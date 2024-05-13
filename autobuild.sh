#!/bin/bash


set -e

# ���û��buildĿ¼��������Ŀ¼
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

# ��֮ǰ�������ı����ļ���ɾ����
rm -rf `pwd`/build/*

# ����build�ļ����£��ҵ�build�ļ����CMakeLists.txt�ļ�
# cmake����makefile�ļ���Ȼ��make
cd `pwd`/build &&
    cmake .. &&
    make

# �ص���Ŀ��Ŀ¼
cd ..

# ��ͷ�ļ������� /usr/include/mymuduo  so�⿽���� /usr/lib    PATH
if [ ! -d /usr/include/mymuduo ]; then 
    mkdir /usr/include/mymuduo
fi

for header in `ls *.h`
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

# ˢ��һ�¶�̬�⻺��
ldconfig