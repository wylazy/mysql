#! /usr/bin/env bash

mkdir -p output
cd output
cmake .. \
    -DCMAKE_INSTALL_PREFIX=/opt/mysql-5.6.21 \
    -DENABLE_DOWNLOADS=ON \
    -DENABLED_LOCAL_INFILE=ON \
    -DCMAKE_C_FLAGS='-g -O0' \
    -DCMAKE_CXX_FLAGS='-g -O0' \
    -DCMAKE_BUILD_TYPE=Debug

make -j4
