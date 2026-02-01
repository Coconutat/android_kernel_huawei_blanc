#!/bin/bash
#设置环境

# 交叉编译器路径
export PATH=$PATH:$(pwd)/../Compiler/Google/GCC64/bin
export CLANG_PREBUILTS_PATH=$(pwd)/../Compiler/Google/Clang/clang-r346389c/
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/../Compiler/Google/Clang/clang-r346389c/lib64/
export CROSS_COMPILE=aarch64-linux-android-

export ARCH=arm64
# export DTC_EXT=dtc

make ARCH=arm64 O=out merge_kirin970_ksu_defconfig
make ARCH=arm64 O=out menuconfig