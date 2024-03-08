#!/bin/bash

SCRIPT_DIR=$(dirname $(readlink -f $0))
TARGET_DIR=$SCRIPT_DIR/build

mkdir -p $TARGET_DIR
cd $TARGET_DIR

cmake ..

if [ $? -ne 0 ]; then
    echo "cmake failed"
    exit 1
fi

make

if [ $? -ne 0 ]; then
    echo "make failed"
    exit 1
fi

sudo ./squash_server 6753