# XMRig Proxy without donate

Extremely high performance Monero (XMR) Stratum protocol proxy, can easily handle over 100K connections on cheap $5 (1024 MB) virtual machine. Reduce number of pool connections up to 256 times, 100K workers become just 391 worker on pool side. Written on C++/libuv same as [XMRig](https://github.com/xmrig/xmrig) miner.

## Compatibility
:warning: :warning: :warning: **Nicehash support must be enabled on miner side, it mandatory.** :warning: :warning: :warning:

* Compatible with any Monero, Electroneum, Sumokoin and AEON pools, except **nicehash.com**.
* Any miner with nicehash support, `--nicehash` option for [XMRig](https://github.com/xmrig/xmrig), `"nicehash_nonce": true,` for xmr-stak-cpu.
* [Comparison](https://github.com/xmrig/xmrig-proxy/wiki/Comparison) with other proxies.

## Why?
This proxy designed and created for handle donation traffic from XMRig. No one other solution works fine with high connection/disconnection rate.

## Download
* Binary releases: https://github.com/xoste49/xmrig-proxy/releases
* Git tree: https://github.com/xmrig/xmrig-proxy.git
  * Clone with `git clone https://github.com/xoste49/xmrig-proxy.git` :hammer: [Build instructions](https://github.com/xmrig/xmrig-proxy/wiki/Build).
  
## Usage
:boom: If you use Linux and want handle more than **1000 connections**, you need [increase limits of open files](https://github.com/xmrig/xmrig-proxy/wiki/Ubuntu-setup).
### Basic example
```
xmrig-proxy.exe -o pool.minemonero.pro:5555 -u YOUR_WALLET -p x --bind 0.0.0.0:3333 --bind 0.0.0.0:5555 
```

### Failover
```
xmrig-proxy.exe -o pool.minemonero.pro:5555 -u YOUR_WALLET1 -o pool.supportxmr.com:5555 -u YOUR_WALLET2 -p x --bind 0.0.0.0:5555 
```
For failover you can add multiple pools, maximum count not limited.
  
### Options
```
  -b, --bind=ADDR          bind to specified address, example "0.0.0.0:3333"
  -o, --url=URL            URL of mining server
  -O, --userpass=U:P       username:password pair for mining server
  -u, --user=USERNAME      username for mining server
  -p, --pass=PASSWORD      password for mining server
  -r, --retries=N          number of times to retry before switch to backup server (default: 5)
  -R, --retry-pause=N      time to pause between retries (default: 5)
      --custom-diff=N      override pool diff
      --verbose            verbose output
      --user-agent=AGENT   set custom user-agent string for pool
      --coin=COIN          xmr for all cryptonight coins or aeon
      --no-color           disable colored output
      --no-workers         disable per worker statistics
  -B, --background         run the miner in the background
  -c, --config=FILE        load a JSON-format configuration file
  -l, --log-file=FILE      log all output to a file
  -S, --syslog             use system log for output messages
  -A  --access-log-file=N  log all workers access to a file
      --api-port=N         port for the miner API
      --api-access-token=T access token for API
      --api-worker-id=ID   custom worker-id for API
  -h, --help               display this help and exit
  -V, --version            output version information and exit
```

## Contacts
* support@xmrig.com
* [reddit](https://www.reddit.com/user/XMRig/)
