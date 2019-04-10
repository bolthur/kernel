#!/bin/bash
set -ex

# get necessary scripts for building the toolchain
wget https://raw.githubusercontent.com/bolthur/workspace/develop/workspace/ci/toolchain.sh

# build toolchain
sh toolchain.sh
