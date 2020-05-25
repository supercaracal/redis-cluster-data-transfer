#!/bin/bash

set -eu

yes yes | \
redis-cli --cluster create \
  $(dig ${HOST_PREFIX}1 +short):6379 \
  $(dig ${HOST_PREFIX}2 +short):6379 \
  $(dig ${HOST_PREFIX}3 +short):6379 \
  $(dig ${HOST_PREFIX}4 +short):6379 \
  $(dig ${HOST_PREFIX}5 +short):6379 \
  $(dig ${HOST_PREFIX}6 +short):6379 \
  --cluster-replicas 1
