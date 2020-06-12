Redis Cluster Data Transfer ![](https://github.com/supercaracal/redis-cluster-data-transfer/workflows/Test/badge.svg?branch=master)
=================================

This tool copies all keys from a Redis cluster to another cluster as able as possible by command.

I want to migrate my redis cluster from 3 to 5.  
Official Redis client has migration feature but source data is deleted.  

[Redis cluster tutorial](https://redis.io/topics/cluster-tutorial)

> There is an alternative way to import data from external instances to a Redis Cluster, which is to use the `redis-cli --cluster import` command.
> The command moves all the keys of a running instance (deleting the keys from the source instance) to the specified pre-existing Redis Cluster.

I need a copy tool.

## Using commands

* [CLUSTER SLOTS](https://redis.io/commands/cluster-slots)
  * For mapping nodes and slots
* [CLUSTER COUNTKEYSINSLOT](https://redis.io/commands/cluster-countkeysinslot)
  * For counting keys in a slot
* [CLUSTER GETKEYSINSLOT](https://redis.io/commands/cluster-getkeysinslot)
  * For listing keys in a slot
* [DUMP](https://redis.io/commands/dump)
  * For fetching a key from source node
* [RESTORE](https://redis.io/commands/restore)
  * For transferring a key to destination node

## Not supported
* AUTH
* SSL
* RESP3
* Windows

so on and so forth

## Trial

```
$ git clone https://github.com/supercaracal/redis-cluster-data-transfer.git
$ cd redis-cluster-data-transfer/
$ make
$ docker-compose up -d
$ bin/cli 127.0.0.1:16371
cli> set key1 1
OK
cli> set key2 1
OK
cli> set key3 1
OK
cli> quit
$ bin/exe 127.0.0.1:16371 127.0.0.1:16381
01: 11654 <140098610599680>: 00000 - 02047
02: 11654 <140098602206976>: 02048 - 04095
07: 11654 <140098560243456>: 12288 - 14335
05: 11654 <140098577028864>: 08192 - 10239
03: 11654 <140098593814272>: 04096 - 06143
08: 11654 <140098477750016>: 14336 - 16383
04: 11654 <140098585421568>: 06144 - 08191
06: 11654 <140098568636160>: 10240 - 12287
3 keys were copied
0 keys were skipped
0 keys were failed
$ bin/cli 127.0.0.1:16381
cli> get key1
1
cli> get key2
1
cli> get key3
1
cli> quit
$
```

## Dry run

```
$ bin/exe 127.0.0.1:16371 127.0.0.1:16381 -C
```

## Performance
It takes approximately 5 minutes while 10 million keys are copied.

## See also
* [Netflix/dynomite](https://github.com/Netflix/dynomite)
* [sripathikrishnan/redis-rdb-tools](https://github.com/sripathikrishnan/redis-rdb-tools)
* [alibaba/RedisShake](https://github.com/alibaba/RedisShake)
* [vipshop/redis-migrate-tool](https://github.com/vipshop/redis-migrate-tool)
* [frsyuki/embulk-plugin-redis](https://github.com/frsyuki/embulk-plugin-redis)
