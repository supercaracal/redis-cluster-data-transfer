#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

MAX_ATTEMPTS=20

for i in {0..9}; do
  cnt=0

  while : ; do
    if [[ ${cnt} -gt ${MAX_ATTEMPTS} ]]; then
      echo "Max attempts exceeded: $cnt times"
      break
    fi

    if docker compose exec src1 redis-cli -c get "key${i}" | grep -q 'null'; then
      echo "key${i}: src: OK"
    else
      echo "key${i}: src: NG"
      sleep 1
      : $((++cnt))
      continue
    fi

    if docker compose exec dest1 redis-cli -c get "key${i}" | grep -q 'null'; then
      echo "key${i}: dest: OK"
    else
      echo "key${i}: dest: NG"
      sleep 1
      : $((++cnt))
      continue
    fi

    break
  done
done
