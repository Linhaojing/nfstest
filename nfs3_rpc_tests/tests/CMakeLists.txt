add_executable(test_nfs3_suite
    test_main.cpp
    test_rpc_null.cpp
    test_rpc_errors.cpp
    test_nfs3_getattr.cpp
    test_nfs3_lookup.cpp
    test_nfs3_readwrite.cpp
    test_nfs3_create.cpp
    test_nfs3_remove.cpp
    test_nfs3_readdir.cpp
    test_nfs3_other.cpp
    test_nfs3_stress.cpp
    test_real_rpc.cpp
)

target_link_libraries(test_nfs3_suite PRIVATE
    nfs3_test_core
    GTest::gtest
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_nfs3_suite)

add_executable(test_xdr test_xdr.cpp)
target_link_libraries(test_xdr PRIVATE nfs3_test_core)
