/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>


#include "core/Config.h"
#include "core/Controller.h"
#include "log/Log.h"
#include "net/Url.h"
#include "proxy/Counters.h"
#include "proxy/events/CloseEvent.h"
#include "proxy/events/LoginEvent.h"
#include "proxy/events/SubmitEvent.h"
#include "proxy/Miner.h"
#include "proxy/splitters/nicehash/NonceMapper.h"
#include "proxy/splitters/nicehash/NonceSplitter.h"


#define LABEL(x) " \x1B[01;30m" x ":\x1B[0m "


static bool compare(Url *i, Url *j) {
  return *i == *j;
}


NonceSplitter::NonceSplitter(xmrig::Controller *controller) : Splitter(controller)
{
}


NonceSplitter::~NonceSplitter()
{
}


uint64_t NonceSplitter::activeUpstreams() const
{
    uint64_t active = 0;

    for (const NonceMapper *mapper : m_upstreams) {
        if (mapper->isActive()) {
            active++;
        }
    }

    return active;
}


void NonceSplitter::connect()
{
    auto upstream = new NonceMapper(m_upstreams.size(), m_controller);
    m_upstreams.push_back(upstream);

    upstream->start();
}


void NonceSplitter::gc()
{
    for (NonceMapper *mapper : m_upstreams) {
        mapper->gc();
    }

    while (m_upstreams.back()->suspended() >= 2) {
        delete m_upstreams.back();

        m_upstreams.pop_back();
    }
}


void NonceSplitter::printConnections()
{
    int active    = 0;
    int suspended = 0;

    for (const NonceMapper *mapper : m_upstreams) {
        if (mapper->isActive()) {
            active++;
            continue;
        }

        if (mapper->isSuspended()) {
            suspended++;
            continue;
        }
    }

    const int error    = (int) m_upstreams.size() - active - suspended;
    const double ratio = active > 0 ? ((double) Counters::miners() / active) : 0;

    if (m_controller->config()->colors()) {
        LOG_INFO("\x1B[01;32m* \x1B[01;37mupstreams\x1B[0m" LABEL("active") "%s%d\x1B[0m" LABEL("sleep") "\x1B[01;37m%d\x1B[0m" LABEL("error") "%s%d\x1B[0m" LABEL("total") "\x1B[01;37m%d",
                 active ? "\x1B[01;32m" : "\x1B[01;31m", active, suspended, error ? "\x1B[01;31m" : "\x1B[01;37m", error, m_upstreams.size());

        LOG_INFO("\x1B[01;32m* \x1B[01;37mminers   \x1B[0m" LABEL("active") "%s%" PRIu64 "\x1B[0m" LABEL("max") "\x1B[01;37m%" PRIu64 "\x1B[0m" LABEL("ratio") "%s1:%3.1f",
                 Counters::miners() ? "\x1B[01;32m" : "\x1B[01;31m", Counters::miners(), Counters::maxMiners(), (ratio > 200 ? "\x1B[01;32m" : "\x1B[01;33m"), ratio);
    }
    else {
        LOG_INFO("* upstreams: active %d sleep %d error %d total %d",
                 active, suspended, error, m_upstreams.size());

        LOG_INFO("* miners:    active %" PRIu64 " max %" PRIu64 " ratio 1:%3.1f",
                 Counters::miners(), Counters::maxMiners(), ratio);
    }
}


void NonceSplitter::tick(uint64_t ticks)
{
    const uint64_t now = uv_now(uv_default_loop());

    for (NonceMapper *mapper : m_upstreams) {
        mapper->tick(ticks, now);
    }
}


#ifdef APP_DEVEL
void NonceSplitter::printState()
{
    for (NonceMapper *mapper : m_upstreams) {
        mapper->printState();
    }
}
#endif


void NonceSplitter::onConfigChanged(xmrig::Config *config, xmrig::Config *previousConfig)
{
    const std::vector<Url*> &pools         = config->pools();
    const std::vector<Url*> &previousPools = previousConfig->pools();

    if (pools.size() != previousPools.size() || !std::equal(pools.begin(), pools.end(), previousPools.begin(), compare)) {
        for (NonceMapper *mapper : m_upstreams) {
            mapper->reload(pools);
        }
    }
}


void NonceSplitter::onEvent(IEvent *event)
{
    switch (event->type())
    {
    case IEvent::CloseType:
        remove(static_cast<CloseEvent*>(event)->miner());
        break;

    case IEvent::LoginType:
        login(static_cast<LoginEvent*>(event));
        break;

    case IEvent::SubmitType:
        submit(static_cast<SubmitEvent*>(event));
        break;

    default:
        break;
    }
}


void NonceSplitter::login(LoginEvent *event)
{
    // try reuse active upstreams.
    for (NonceMapper *mapper : m_upstreams) {
        if (!mapper->isSuspended() && mapper->add(event->miner(), event->request)) {
            return;
        }
    }

    // try reuse suspended upstreams.
    for (NonceMapper *mapper : m_upstreams) {
        if (mapper->isSuspended() && mapper->add(event->miner(), event->request)) {
            return;
        }
    }

    connect();
    login(event);
}


void NonceSplitter::remove(Miner *miner)
{
    if (miner->mapperId() < 0) {
        return;
    }

    m_upstreams[miner->mapperId()]->remove(miner);
}


void NonceSplitter::submit(SubmitEvent *event)
{
    m_upstreams[event->miner()->mapperId()]->submit(event);
}
