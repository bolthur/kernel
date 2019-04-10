#!/bin/bash
set -ex

# Get base directory
BASEDIR="$( cd "$( dirname "$0" )" && pwd )"

# build pi
sh "$BASEDIR/build/pi.sh"
