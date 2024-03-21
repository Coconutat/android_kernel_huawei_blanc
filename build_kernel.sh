#!/bin/bash
#设置环境

# 交叉编译器路径
export PATH=$PATH:$(pwd)/../Compiler/Google/GCC64/bin


# Find those flag in Makefile!!!
# 这两个标记请在Makefile内寻找！！！
export CLANG_PREBUILTS_PATH=$(pwd)/../Compiler/Google/Clang/clang-r346389c/
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/../Compiler/Google/Clang/clang-r346389c/lib64/


export CROSS_COMPILE=aarch64-linux-android-
export ARCH=arm64
export SUBARCH=arm64
# export DTC_EXT=dtc

if [ ! -d "out" ]; then
	mkdir out
fi

start_time=$(date +%Y.%m.%d-%I_%M)

start_time_sum=$(date +%s)

make ARCH=arm64 O=out merge_kirin970_ksu_defconfig
# 定义编译线程数
make ARCH=arm64 O=out -j$(nproc --all) 2>&1 | tee kernel_log-${start_time}.txt

end_time_sum=$(date +%s)

end_time=$(date +%Y.%m.%d-%I_%M)

# 计算运行时间（秒）
duration=$((end_time_sum - start_time_sum))

# 将秒数转化为 "小时:分钟:秒" 形式输出
hours=$((duration / 3600))
minutes=$(((duration % 3600) / 60))
seconds=$((duration % 60))

# 打印运行时间
echo "脚本运行时间为：${hours}小时 ${minutes}分钟 ${seconds}秒"

if [ -f out/arch/arm64/boot/Image.gz ]; then

	echo "***Sucessfully built kernel...***"
	cp out/arch/arm64/boot/Image.gz Image.gz
	./tools/mkbootimg --kernel out/arch/arm64/boot/Image.gz --base 0x0 --cmdline "loglevel=4 page_tracker=on unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=enforcing buildvariant=user" --tags_offset 0x37E00000 --kernel_offset 0x00080000 --ramdisk_offset 0x37600000 --header_version 1 --os_version 10 --os_patch_level 2019-11-01 --output Mate10Pro_kernel.img
	./tools/mkbootimg --kernel out/arch/arm64/boot/Image.gz --base 0x0 --cmdline "loglevel=4 page_tracker=on unmovable_isolate1=2:192M,3:224M,4:256M printktimer=0xfff0a000,0x534,0x538 androidboot.selinux=permissive buildvariant=user" --tags_offset 0x37E00000 --kernel_offset 0x00080000 --ramdisk_offset 0x37600000 --header_version 1 --os_version 10 --os_patch_level 2019-11-01  --output Mate10Pro_PM_kernel.img
	echo " "
	exit 0
else
	echo " "
	echo "***Failed!***"
	exit 0
fi
