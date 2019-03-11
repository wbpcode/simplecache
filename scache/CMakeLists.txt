# CMakeList.txt: scache 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

if(WIN32)
set(BOOST_ROOT "C:/boost_1_69_0/")
endif()

find_package(Boost REQUIRED COMPONENTS system)

if(Boost_FOUND)
include_directories(${Boost_INCLUDE_DIRS})
# 将源代码添加到此项目的可执行文件。
add_executable (scache 
    "simple-cache.cpp" 
    "cache-object.h" "cache-object.cpp" 
    "cache-dict.h" "cache-dict.cpp" 
    "cache-list.h" "cache-list.cpp"
    "cache-server.h" "cache-server.cpp"
    "request-buffer.h" "request-buffer.cpp"
    "cache-session.h" "cache-session.cpp")

target_link_libraries(scache ${Boost_LIBRARIES})
else()
message("No Boost!!!")
endif()

# TODO: 如有需要，请添加测试并安装目标。
