#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

MAX_ATTEMPTS=30

for i in {0..9}; do
  cnt=0

  while : ; do
    if [[ ${cnt} -ge ${MAX_ATTEMPTS} ]]; then
      echo "Max attempts exceeded: $cnt times"
      break
    fi

    if docker compose exec src1 redis-cli -c --no-raw get "key${i}" | grep -q nil; then
      echo "OK: key${i}: src"
    else
      echo "NG: key${i}: src"
      sleep 1
      : $((++cnt))
      continue
    fi

    if docker compose exec dest1 redis-cli -c --no-raw get "key${i}" | grep -q nil; then
      echo "OK: key${i}: dest"
    else
      echo "NG: key${i}: dest"
      sleep 1
      : $((++cnt))
      continue
    fi

    break
  done
done
