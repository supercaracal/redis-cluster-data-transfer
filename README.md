Redis Cluster Data Transfer ![](https://github.com/supercaracal/redis-cluster-data-transfer/workflows/Test/badge.svg?branch=master)
=================================

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
  * For counting keys each slot
* [CLUSTER GETKEYSINSLOT](https://redis.io/commands/cluster-getkeysinslot)
  * For listing keys in slot
* [MIGRATE](https://redis.io/commands/migrate)
  * For copying a key from source to destination

## Not supported
* AUTH
* SSL
* RESP3

so on and so forth

## TODO
* multi thread

## Trial

```
$ docker-compose up -d
$ make
$ bin/exe 127.0.0.1:16371 127.0.0.1:16381
```

## Dry run

```
$ bin/exe 127.0.0.1:16371 127.0.0.1:16381 -C
```

## See also
* [alibaba/RedisShake](https://github.com/alibaba/RedisShake)
* [frsyuki/embulk-plugin-redis](https://github.com/frsyuki/embulk-plugin-redis)
* [Netflix/dynomite](https://github.com/Netflix/dynomite)
* [sripathikrishnan/redis-rdb-tools](https://github.com/sripathikrishnan/redis-rdb-tools)
