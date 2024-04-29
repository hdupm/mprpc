#设置cmake的最低版本和项目名称
cmake_minimum_required(VERSION 3.0)
project(mprpc) #工程名字

#生成debug版本 可以进行gbd调试
set(CMAKE_BUILD_TYPE "Debug")

#配置编译选项 可执行文件可调试
#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#设置可执行文件最终存储路径 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)#全局变量 PROJECT_SOURCE_DIR是工程根目录

#设置项目库文件输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置项目编译头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/example)

#设置项目库文件搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

#加载子目录 src包含了mprpc框架所有的相关代码
add_subdirectory(src)
#example里面是业务代码 example包含了mprpc框架使用的示例代码
add_subdirectory(example) 

