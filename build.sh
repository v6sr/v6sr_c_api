#!/bin/bash
PROJECT_NAME=${PWD##*/}
THIS_DIR=`pwd`
CMAKE=cmake3

if [ ! -d "./build" ]; then
    mkdir -p ./build
    cd ./build
    ${CMAKE} -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
    cd ${THIS_DIR}
fi

make -C ./build
