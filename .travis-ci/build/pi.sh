#!/bin/bash
set -ex

# Get base directory
BASEDIR="$( cd "$( dirname "$0" )" && pwd )"

# build pi versions
sh "$BASEDIR/pi/2b_rev1.sh"
