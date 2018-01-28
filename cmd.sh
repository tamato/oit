#!/bin/bash

build_dir=~/programming/oit_build

if [[ $1 = "rebuild" ]]; then
    cd $build_dir
    rm -rf ./* && cmake ../oit && make && ./oit
fi

if [[ $1 = "shaders" ]]; then
    cp `ls ./data/ | egrep -i "\.vert|\.frag"` $(build_dir/data/)
fi

if [[ $1 = "make" ]]; then
    cd $build_dir
    make && ./oit
fi

if [[ $1 = "run" ]]; then
    cd $build_dir
    ./oit
fi

