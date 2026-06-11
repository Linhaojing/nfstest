cmake_minimum_required(VERSION 3.14)
project(nfs3_rpc_tests_c LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

add_subdirectory(../shared_rpc ${CMAKE_BINARY_DIR}/shared_rpc_build)

add_library(nfs3_test_core_c STATIC
    src/xdr_codec.c
    src/nfs3_types.c
    src/nfs3_xdr.c
)

target_include_directories(nfs3_test_core_c PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../shared_rpc/include
)

target_link_libraries(nfs3_test_core_c PUBLIC shared_rpc)

target_compile_features(nfs3_test_core_c PUBLIC c_std_11)

enable_testing()
add_subdirectory(tests)
