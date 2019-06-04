#!/bin/bash

[ ! -d build ] && mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
#cpack .. 
make
#make install