Redis Cluster Data Transfer
=================================

I want to migrate my redis cluster from 3 to 5.  
Official Redis client has migration feature but source data is deleted.  

[Redis cluster tutorial](https://redis.io/topics/cluster-tutorial)

> There is an alternative way to import data from external instances to a Redis Cluster, which is to use the `redis-cli --cluster import` command.
> The command moves all the keys of a running instance (deleting the keys from the source instance) to the specified pre-existing Redis Cluster.

I need a copy tool.

### Using commands

* [CLUSTER SLOTS](https://redis.io/commands/cluster-slots)
  * For mapping nodes and slots
* [CLUSTER COUNTKEYSINSLOT](https://redis.io/commands/cluster-countkeysinslot)
  * For counting keys each slot
* [CLUSTER GETKEYSINSLOT](https://redis.io/commands/cluster-getkeysinslot)
  * For listing keys in slot
* [MIGRATE](https://redis.io/commands/migrate)
  * For copying a key from source to destination

### Not supported
* AUTH
* SSL
* RESP3
so on and so forth

### Trial

```
$ docker-compose up -d
$ make
$ bin/exe 127.0.0.1:16371 127.0.0.1:16381
```
