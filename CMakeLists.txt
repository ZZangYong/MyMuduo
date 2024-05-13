cmake_minimum_required(VERSION 3.22)
project(MyMuduo)

# MyMuduo最终编译成so动态库，设置动态库的路径，放在根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 设置调试信息（编译选项），启动c++11语言标准
set(CMaKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")
# 定义参与编译的源代码文件,把当前根目录下源文件名字组合起来放在SRC_LIST变量里
aux_source_directory(. SRC_LIST)
# 用SRC_LIST变量下的文件编译动态库mymuduo
add_library(mymuduo SHARED ${SRC_LIST})