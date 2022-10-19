#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

MAX_ATTEMPTS=30
SRC_NODE=src1
DEST_NODE=dest1

for i in {0..9}; do
  cnt=0
  key="key${i}"

  while : ; do
    if [[ ${cnt} -ge ${MAX_ATTEMPTS} ]]; then
      echo "Max attempts exceeded: $cnt times"
      break
    fi

    if docker compose exec $SRC_NODE redis-cli -c --no-raw get $key | grep -q nil; then
      echo "OK: ${key}: ${SRC_NODE}"
    else
      echo "NG: ${key}: ${SRC_NODE}"
      sleep 1
      : $((++cnt))
      continue
    fi

    if docker compose exec $DEST_NODE redis-cli -c --no-raw get $key | grep -q nil; then
      echo "OK: ${key}: ${DEST_NODE}"
    else
      echo "NG: ${key}: ${DEST_NODE}"
      sleep 1
      : $((++cnt))
      continue
    fi

    break
  done
done
