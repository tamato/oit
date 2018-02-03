#!/bin/bash

build_dir=~/programming/build_oit
out_data=$build_dir/data
target=$build_dir/oit
local_target=./oit


if [[ $1 = "rebuild" ]]; then
    cd $build_dir
    rm -rf ./* && cmake -G "Sublime Text 2 - Unix Makefiles" ../oit && make && ./oit
fi

if [[ $1 = "make" ]]; then
    cp $(find ~/programming/oit/data -iname "*.vert") $out_data
    cp $(find ~/programming/oit/data -iname "*.frag") $out_data

    cd $build_dir
    make && $local_target
fi

if [[ $1 = "run" ]]; then
    cp $(find ~/programming/oit/data -iname "*.vert") $out_data
    cp $(find ~/programming/oit/data -iname "*.frag") $out_data

    cd $build_dir
    $local_target
fi

if [[ $1 = "debug" ]]; then
    cd $build_dir
    gdb $local_target
fi

