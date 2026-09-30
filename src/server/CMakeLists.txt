cmake_minimum_required(VERSION 3.10)

#声明工程的名称
project(SimpleTieServer LANGUAGES CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CXX_SRCS main.cpp StunServer.cpp)

add_executable(SimpleTieServer ${CXX_SRCS})
