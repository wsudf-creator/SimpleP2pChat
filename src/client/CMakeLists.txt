cmake_minimum_required(VERSION 3.10)

#声明工程的名称
project(SimpleTieClient LANGUAGES C CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(C_SRCS ikcp.c)
set(CXX_SRCS main.cpp Punch.cpp StunClient.cpp UdpChat.cpp)

add_executable(SimpleTieClient ${C_SRCS} ${CXX_SRCS})

if(MSVC)
    target_compile_options(SimpleTieClient PRIVATE /utf-8)
    find_package(Boost REQUIRED)
    target_include_directories(SimpleTieClient PRIVATE ${Boost_INCLUDE_DIRS})
endif()