#!/usr/bin/env bash
set -e

DEPS_OUTPUT_DIRECTORY=$(pwd)/build_3rdparty

cd 3rdparty

cd openssl
./Configure --prefix=${DEPS_OUTPUT_DIRECTORY} --openssldir=${DEPS_OUTPUT_DIRECTORY} '-Wl,-rpath,$(LIBRPATH)'
make
make install
cd ..

cd libsrtp
mkdir -p build
cd build
cmake -GNinja -DCMAKE_INSTALL_PREFIX:PATH=${DEPS_OUTPUT_DIRECTORY} -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENSSL=ON ..
cmake --build . --target install -j 4
rm -r *
