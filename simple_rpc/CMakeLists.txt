cmake_minimum_required(VERSION 3.14)
project(simple_rpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
endif()

add_library(simple_rpc STATIC
    src/rpc_transport.cpp
    src/rpc_client.cpp
)

target_include_directories(simple_rpc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(simple_rpc PUBLIC cxx_std_17)
