Redis Cluster Data Transfer ![](https://github.com/supercaracal/redis-cluster-data-transfer/workflows/Test/badge.svg?branch=master)
=================================

This tool copies all keys from a Redis cluster to another one as able as possible by command.

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
* [PTTL](https://redis.io/commands/pttl)
  * For fetching remained TTL milliseconds of a key
* [RESTORE](https://redis.io/commands/restore)
  * For transferring a key to destination node

## Not supported
* AUTH
* SSL
* RESP3
* Windows

so on and so forth

## TODO
* Use non-blocking IO

## Trial

```
$ git clone https://github.com/supercaracal/redis-cluster-data-transfer.git
$ cd redis-cluster-data-transfer/
$ make
$ docker-compose up -d

$ bin/cli 127.0.0.1:16371
>> set key1 1
OK
>> set key2 2
OK
>> set key3 3
OK
>> quit

$ bin/exe 127.0.0.1:16371 127.0.0.1:16381
3 keys were found
3 keys were copied
0 keys were skipped
0 keys were failed

$ bin/diff 127.0.0.1:16371 127.0.0.1:16381
3 keys were found
3 keys were same
0 keys were different
0 keys were deficient
0 keys were failed
0 keys were skipped

$ bin/cli 127.0.0.1:16381
>> get key1
1
>> get key2
2
>> get key3
3
>> quit
$
```

## Dry run

```
$ bin/exe 127.0.0.1:16371 127.0.0.1:16381 -C
```

## Performance
It takes approximately 3 minutes while 10 million keys are copied. This program is I/O bound.

## See also
* [Netflix/dynomite](https://github.com/Netflix/dynomite)
* [sripathikrishnan/redis-rdb-tools](https://github.com/sripathikrishnan/redis-rdb-tools)
* [alibaba/RedisShake](https://github.com/alibaba/RedisShake)
* [alibaba/RedisFullCheck](https://github.com/alibaba/RedisFullCheck)
* [vipshop/redis-migrate-tool](https://github.com/vipshop/redis-migrate-tool)
* [frsyuki/embulk-plugin-redis](https://github.com/frsyuki/embulk-plugin-redis)
* [opstree/redis-migration](https://github.com/opstree/redis-migration)
