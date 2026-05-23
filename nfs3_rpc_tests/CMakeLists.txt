cmake_minimum_required(VERSION 3.14)
project(nfs3_rpc_tests LANGUAGES CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wpedantic")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(PkgConfig)
find_package(Tirpc)

option(BUILD_TESTS "Build unit tests" ON)
option(USE_BUILTIN_RPC "Use built-in RPC implementation instead of libtirpc" OFF)

add_library(nfs3_test_core STATIC
    src/xdr_codec.cpp
    src/detail/xdr_primitive.cpp
)

target_include_directories(nfs3_test_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(nfs3_test_core PUBLIC cxx_std_17)

if(TIRPC_FOUND AND NOT USE_BUILTIN_RPC)
    target_sources(nfs3_test_core PRIVATE
        src/rpc_endpoint.cpp
        src/nfs3_client.cpp
        src/test_context.cpp
        src/mount_client.cpp
        src/nfs3_xdr.cpp
    )

    target_include_directories(nfs3_test_core PUBLIC
        ${TIRPC_INCLUDE_DIRS}
    )

    target_link_libraries(nfs3_test_core PUBLIC
        ${TIRPC_LIBRARIES}
        pthread
    )

    add_definitions(-DHAVE_LIBTIRPC=1)
    message(STATUS "libtirpc found, RPC support enabled")
else()
    add_subdirectory(../shared_rpc ${CMAKE_BINARY_DIR}/shared_rpc_build)

    target_sources(nfs3_test_core PRIVATE
        src/rpc_endpoint.cpp
        src/nfs3_client.cpp
        src/test_context.cpp
        src/mount_client.cpp
    )

    target_include_directories(nfs3_test_core PUBLIC
        ${CMAKE_SOURCE_DIR}/../shared_rpc/include
    )

    target_link_libraries(nfs3_test_core PUBLIC shared_rpc)

    if(TIRPC_FOUND AND USE_BUILTIN_RPC)
        message(STATUS "libtirpc available but forced using built-in RPC")
    elseif(NOT TIRPC_FOUND)
        message(STATUS "libtirpc not found, using built-in RPC (shared_rpc)")
    endif()
endif()

if(BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)

    if(NOT GTest_FOUND)
        message(STATUS "GTest not found, fetching via FetchContent")
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    add_subdirectory(tests)
endif()

option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES AND (TIRPC_FOUND OR USE_BUILTIN_RPC))
    add_executable(example_basic_connect examples/basic_connect.cpp)
    target_link_libraries(example_basic_connect nfs3_test_core)

    add_executable(example_nfs_operations examples/nfs_operations.cpp)
    target_link_libraries(example_nfs_operations nfs3_test_core)

    message(STATUS "Examples will be built")
endif()
