#!/bin/bash
set -ex

cd "$(mktemp -dt plumed.XXXXXX)"

git clone https://github.com/danmar/cppcheck.git
cd cppcheck
make install MATCHCOMPILER=yes FILESDIR="$HOME/.bolthur/opt/share/cppcheck" HAVE_RULES=yes CXXFLAGS="-O2 -DNDEBUG -Wall -Wno-sign-compare -Wno-unused-function" PREFIX="$HOME/.bolthur/opt"
cd ..
