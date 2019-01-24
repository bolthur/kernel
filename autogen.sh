#!/bin/bash

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
