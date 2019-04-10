#!/bin/bash
set -ex

# Get directory path
BASEDIR="$( cd "$( dirname "$0" )" && pwd )"

# create config directory for automake
if [ ! -d "$BASEDIR/config" ]; then
  mkdir "$BASEDIR/config"
fi

cd $BASEDIR

aclocal -I m4
if [ $? -ne 0 ]; then
  echo "Error: Unable to execute aclocal"
  exit 1
fi

autoheader
if [ $? -ne 0 ]; then
  echo "Error: Unable to execute autoheader"
  exit 1
fi

automake --foreign --add-missing
if [ $? -ne 0 ]; then
  echo "Error: Unable to execute automake"
  exit 1
fi

autoconf
if [ $? -ne 0 ]; then
  echo "Error: Unable to execute autoconf"
  exit 1
fi
