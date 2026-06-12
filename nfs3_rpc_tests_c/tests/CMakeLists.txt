add_executable(test_suite_c test_main.c)
target_link_libraries(test_suite_c PRIVATE nfs3_test_core_c)
target_include_directories(test_suite_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_xdr_c test_xdr.c)
target_link_libraries(test_xdr_c PRIVATE nfs3_test_core_c)
target_include_directories(test_xdr_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_xdr_full_c test_nfs3_xdr_full.c)
target_link_libraries(test_nfs3_xdr_full_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_xdr_full_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_getattr_c test_nfs3_getattr.c nfs3_server_test.c)
target_link_libraries(test_nfs3_getattr_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_getattr_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_lookup_c test_nfs3_lookup.c nfs3_server_test.c)
target_link_libraries(test_nfs3_lookup_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_lookup_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_readwrite_c test_nfs3_readwrite.c nfs3_server_test.c)
target_link_libraries(test_nfs3_readwrite_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_readwrite_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_create_c test_nfs3_create.c nfs3_server_test.c)
target_link_libraries(test_nfs3_create_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_create_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_readdir_c test_nfs3_readdir.c nfs3_server_test.c)
target_link_libraries(test_nfs3_readdir_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_readdir_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_dir_c test_nfs3_dir_c.c nfs3_server_test.c)
target_link_libraries(test_nfs3_dir_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_dir_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_meta_c test_nfs3_meta_c.c nfs3_server_test.c)
target_link_libraries(test_nfs3_meta_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_meta_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_namespace_c test_nfs3_namespace_c.c nfs3_server_test.c)
target_link_libraries(test_nfs3_namespace_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_namespace_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_io_c test_nfs3_io_c.c nfs3_server_test.c)
target_link_libraries(test_nfs3_io_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_io_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_nfs3_symlink_special_c test_nfs3_symlink_special_c.c nfs3_server_test.c)
target_link_libraries(test_nfs3_symlink_special_c PRIVATE nfs3_test_core_c)
target_include_directories(test_nfs3_symlink_special_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_rpc_real_c test_rpc_real.c nfs3_server_test.c)
target_link_libraries(test_rpc_real_c PRIVATE nfs3_test_core_c)
target_include_directories(test_rpc_real_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_rpc_robustness_c test_rpc_robustness.c nfs3_server_test.c)
target_link_libraries(test_rpc_robustness_c PRIVATE nfs3_test_core_c)
target_include_directories(test_rpc_robustness_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set(NFS3_SERVER_TEST_LABELS "integration;server;nfs3")

add_test(NAME test_suite_c COMMAND test_suite_c)
add_test(NAME test_xdr_c COMMAND test_xdr_c)
add_test(NAME test_nfs3_xdr_full_c COMMAND test_nfs3_xdr_full_c)
add_test(NAME test_nfs3_getattr_c COMMAND test_nfs3_getattr_c)
add_test(NAME test_nfs3_lookup_c COMMAND test_nfs3_lookup_c)
add_test(NAME test_nfs3_readwrite_c COMMAND test_nfs3_readwrite_c)
add_test(NAME test_nfs3_create_c COMMAND test_nfs3_create_c)
add_test(NAME test_nfs3_readdir_c COMMAND test_nfs3_readdir_c)
add_test(NAME test_nfs3_dir_c COMMAND test_nfs3_dir_c)
add_test(NAME test_nfs3_meta_c COMMAND test_nfs3_meta_c)
add_test(NAME test_nfs3_namespace_c COMMAND test_nfs3_namespace_c)
add_test(NAME test_nfs3_io_c COMMAND test_nfs3_io_c)
add_test(NAME test_nfs3_symlink_special_c COMMAND test_nfs3_symlink_special_c)
add_test(NAME test_rpc_real_c COMMAND test_rpc_real_c)
add_test(NAME test_rpc_robustness_c COMMAND test_rpc_robustness_c)

set_tests_properties(
    test_suite_c
    test_xdr_c
    test_nfs3_xdr_full_c
    PROPERTIES LABELS "unit;xdr"
)

set_tests_properties(
    test_nfs3_getattr_c
    test_nfs3_lookup_c
    test_nfs3_readwrite_c
    test_nfs3_create_c
    test_nfs3_readdir_c
    test_nfs3_dir_c
    test_nfs3_meta_c
    test_nfs3_namespace_c
    test_nfs3_io_c
    test_nfs3_symlink_special_c
    test_rpc_real_c
    test_rpc_robustness_c
    PROPERTIES LABELS "${NFS3_SERVER_TEST_LABELS}"
)
