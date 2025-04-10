#!/usr/bin/env bash
set -e

DEPS_OUTPUT_DIRECTORY=$(pwd)/build_3rdparty

cd 3rdparty

cd openssl
./Configure --prefix=${DEPS_OUTPUT_DIRECTORY} --openssldir=${DEPS_OUTPUT_DIRECTORY} '-Wl,-rpath,$(LIBRPATH)'
make
make install
