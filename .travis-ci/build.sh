#!/bin/bash
set -ex

# Get base directory
BASEDIR="$( cd "$( dirname "$0" )" && pwd )"

# Configure
cd "$BASEDIR/../"
autoreconf -if
cd "$BASEDIR"

# Build pi
sh "$BASEDIR/build/pi.sh"
