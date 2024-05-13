#!/bin/bash


set -e

# 如果没有build目录，创建该目录
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# 把之前产生过的编译文件都删除掉
rm -rf `pwd`/build/*

# 进入build文件夹下，找到build文件外的CMakeLists.txt文件
# cmake生成makefile文件，然后make
cd `pwd`/build &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo  so库拷贝到 /usr/lib    PATH
if [ ! -d /usr/include/mymuduo ]; then 
    mkdir /usr/include/mymuduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

# 刷新一下动态库缓存
ldconfig