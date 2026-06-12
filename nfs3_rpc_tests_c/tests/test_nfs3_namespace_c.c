#include "test_framework.h"
#include "nfs3_server_test.h"
#include "nfstest/rpc_client.h"
#include "nfs3_c/nfs3_xdr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE_SERVER_OR_SKIP() do { \
    if (!nfs3_server_is_configured()) { \
        if (nfs3_server_require_server()) { \
            ASSERT_TRUE(0, nfs3_server_skip_message()); \
        } \
        TEST_SKIP(nfs3_server_skip_message()); \
    } \
} while(0)

#define CHECK_TRUE(cond, msg) do { \
    if (!(cond)) { \
        printf("\n  FAIL  %s: %s (line %d)\n", current_test_name, (msg), __LINE__); \
        tests_failed++; \
        failed = 1; \
        goto cleanup; \
    } \
} while(0)

#define CHECK_STATUS(actual, expected, msg) do { \
    int _actual = (actual); \
    int _expected = (expected); \
    if (_actual != _expected) { \
        printf("\n  FAIL  %s: %s - expected %d, got %d (line %d)\n", current_test_name, (msg), _expected, _actual, __LINE__); \
        tests_failed++; \
        failed = 1; \
        goto cleanup; \
    } \
} while(0)

static const nfsstat3_t link_unsupported_statuses[] = {
    NFS3ERR_NOTSUPP,
    NFS3ERR_PERM,
    NFS3ERR_ACCES,
    NFS3ERR_ROFS
};

static void mount_root(nfs_fh3_t* root_fh) {
    int status = nfs3_mount_root(root_fh);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT root should succeed");
    ASSERT_TRUE(!nfs_fh3_is_empty(root_fh), "root file handle should not be empty");
}

static int status_allowed(nfsstat3_t status, const nfsstat3_t* allowed, size_t count) {
    return nfs3_status_is_allowed(status, allowed, count);
}

static int link_unsupported(nfsstat3_t status) {
    return status_allowed(status, link_unsupported_statuses, sizeof(link_unsupported_statuses) / sizeof(link_unsupported_statuses[0]));
}

static void make_long_name(char* buffer, size_t buffer_len) {
    memset(buffer, 'x', buffer_len - 1);
    buffer[buffer_len - 1] = '\0';
}

static int call_lookup(const nfs_fh3_t* dir, const char* name, nfs_fh3_t* out_fh) {
    return nfs3_lookup_name(dir, name, out_fh);
}

static int call_create(const nfs_fh3_t* dir, const char* name, createmode3_t mode, uint32_t file_mode, const uint8_t* verf, nfs_fh3_t* out_fh) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    CREATE3args_t create_args;
    create3args_init(&create_args);
    nfs_fh3_set(&create_args.where_dir, dir->data, dir->len);
    create3args_set_name(&create_args, name);
    create_args.how_mode = mode;
    if (mode == EXCLUSIVE) {
        if (verf) {
            memcpy(create_args.how_verf.data, verf, sizeof(create_args.how_verf.data));
        }
    } else {
        create_args.how_attributes.mode_set = 1;
        create_args.how_attributes.mode = file_mode;
    }
    xdr_pack_CREATE3args(&args, &create_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_CREATE, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        CREATE3res_t res;
        create3res_init(&res);
        xdr_unpack_CREATE3res(&body, &res);
        nfs_status = (int)res.status;
        if (res.status == NFS3_OK && out_fh) {
            nfs_fh3_set(out_fh, res.resok.object.data, res.resok.object.len);
        }
        create3res_destroy(&res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    create3args_destroy(&create_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int call_mkdir(const nfs_fh3_t* dir, const char* name, uint32_t mode, nfs_fh3_t* out_fh) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    MKDIR3args_t mkdir_args;
    mkdir3args_init(&mkdir_args);
    nfs_fh3_set(&mkdir_args.where_dir, dir->data, dir->len);
    mkdir3args_set_name(&mkdir_args, name);
    mkdir_args.attributes.mode_set = 1;
    mkdir_args.attributes.mode = mode;
    xdr_pack_MKDIR3args(&args, &mkdir_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_MKDIR, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        MKDIR3res_t res;
        mkdir3res_init(&res);
        xdr_unpack_MKDIR3res(&body, &res);
        nfs_status = (int)res.status;
        if (res.status == NFS3_OK && out_fh) {
            nfs_fh3_set(out_fh, res.resok.object.data, res.resok.object.len);
        }
        mkdir3res_destroy(&res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    mkdir3args_destroy(&mkdir_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int call_remove(const nfs_fh3_t* dir, const char* name) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    REMOVE3args_t remove_args;
    remove3args_init(&remove_args);
    nfs_fh3_set(&remove_args.object_dir, dir->data, dir->len);
    remove3args_set_name(&remove_args, name);
    xdr_pack_REMOVE3args(&args, &remove_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_REMOVE, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        REMOVE3res_t res;
        memset(&res, 0, sizeof(res));
        xdr_unpack_REMOVE3res(&body, &res);
        nfs_status = (int)res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    remove3args_destroy(&remove_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int call_rmdir(const nfs_fh3_t* dir, const char* name) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    RMDIR3args_t rmdir_args;
    rmdir3args_init(&rmdir_args);
    nfs_fh3_set(&rmdir_args.object_dir, dir->data, dir->len);
    rmdir3args_set_name(&rmdir_args, name);
    xdr_pack_RMDIR3args(&args, &rmdir_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_RMDIR, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        RMDIR3res_t res;
        memset(&res, 0, sizeof(res));
        xdr_unpack_RMDIR3res(&body, &res);
        nfs_status = (int)res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    rmdir3args_destroy(&rmdir_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int call_rename(const nfs_fh3_t* from_dir, const char* from_name, const nfs_fh3_t* to_dir, const char* to_name) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    RENAME3args_t rename_args;
    rename3args_init(&rename_args);
    nfs_fh3_set(&rename_args.from_dir, from_dir->data, from_dir->len);
    rename3args_set_from_name(&rename_args, from_name);
    nfs_fh3_set(&rename_args.to_dir, to_dir->data, to_dir->len);
    rename3args_set_to_name(&rename_args, to_name);
    xdr_pack_RENAME3args(&args, &rename_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_RENAME, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        RENAME3res_t res;
        memset(&res, 0, sizeof(res));
        xdr_unpack_RENAME3res(&body, &res);
        nfs_status = (int)res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    rename3args_destroy(&rename_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int call_link(const nfs_fh3_t* file, const nfs_fh3_t* link_dir, const char* link_name) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    LINK3args_t link_args;
    link3args_init(&link_args);
    nfs_fh3_set(&link_args.file, file->data, file->len);
    nfs_fh3_set(&link_args.link_dir, link_dir->data, link_dir->len);
    link3args_set_name(&link_args, link_name);
    xdr_pack_LINK3args(&args, &link_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_LINK, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        LINK3res_t res;
        memset(&res, 0, sizeof(res));
        xdr_unpack_LINK3res(&body, &res);
        nfs_status = (int)res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    link3args_destroy(&link_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static void test_lookup_namespace(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t lookup_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&lookup_fh);
    char file_name[128];
    char dir_name[128];
    char missing_name[128];
    nfs3_unique_name("c_ns_lookup_file", file_name, sizeof(file_name));
    nfs3_unique_name("c_ns_lookup_dir", dir_name, sizeof(dir_name));
    nfs3_unique_name("c_ns_lookup_missing", missing_name, sizeof(missing_name));
    int file_created = 0;
    int dir_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(nfs3_create_test_file(&root_fh, file_name, &file_fh), NFS3_OK, "CREATE setup file should succeed");
    file_created = 1;
    CHECK_STATUS(nfs3_create_test_dir(&root_fh, dir_name, &dir_fh), NFS3_OK, "MKDIR setup dir should succeed");
    dir_created = 1;

    CHECK_STATUS(call_lookup(&root_fh, file_name, &lookup_fh), NFS3_OK, "LOOKUP existing file should succeed");
    CHECK_TRUE(!nfs_fh3_is_empty(&lookup_fh), "LOOKUP existing file should return handle");
    nfs_fh3_destroy(&lookup_fh);
    nfs_fh3_init(&lookup_fh);
    CHECK_STATUS(call_lookup(&root_fh, dir_name, &lookup_fh), NFS3_OK, "LOOKUP existing dir should succeed");
    CHECK_TRUE(!nfs_fh3_is_empty(&lookup_fh), "LOOKUP existing dir should return handle");
    CHECK_STATUS(call_lookup(&root_fh, missing_name, NULL), NFS3ERR_NOENT, "LOOKUP nonexistent should return NOENT");
    CHECK_STATUS(call_lookup(&file_fh, missing_name, NULL), NFS3ERR_NOTDIR, "LOOKUP with non-directory handle should return NOTDIR");

cleanup:
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dir_name);
    }
    if (file_created) {
        nfs3_remove_if_test_file(&root_fh, file_name);
    }
    nfs_fh3_destroy(&lookup_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_create_namespace(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t unchecked_fh;
    nfs_fh3_t guarded_fh;
    nfs_fh3_t exclusive_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&unchecked_fh);
    nfs_fh3_init(&guarded_fh);
    nfs_fh3_init(&exclusive_fh);
    char unchecked_name[128];
    char guarded_name[128];
    char exclusive_name[128];
    char child_name[128];
    char long_name[300];
    nfs3_unique_name("c_ns_create_unchecked", unchecked_name, sizeof(unchecked_name));
    nfs3_unique_name("c_ns_create_guarded", guarded_name, sizeof(guarded_name));
    nfs3_unique_name("c_ns_create_exclusive", exclusive_name, sizeof(exclusive_name));
    nfs3_unique_name("c_ns_create_child", child_name, sizeof(child_name));
    make_long_name(long_name, sizeof(long_name));
    const uint8_t verf[8] = { 'n', 'f', 's', '3', 't', 'e', 's', 't' };
    int unchecked_created = 0;
    int guarded_created = 0;
    int exclusive_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(call_create(&root_fh, unchecked_name, UNCHECKED, 0644, NULL, &unchecked_fh), NFS3_OK, "CREATE UNCHECKED should succeed");
    unchecked_created = 1;
    CHECK_STATUS(call_create(&root_fh, guarded_name, GUARDED, 0600, NULL, &guarded_fh), NFS3_OK, "CREATE GUARDED should succeed");
    guarded_created = 1;
    CHECK_STATUS(call_create(&root_fh, exclusive_name, EXCLUSIVE, 0, verf, &exclusive_fh), NFS3_OK, "CREATE EXCLUSIVE should succeed");
    exclusive_created = 1;
    CHECK_STATUS(call_create(&root_fh, guarded_name, GUARDED, 0600, NULL, NULL), NFS3ERR_EXIST, "CREATE duplicate GUARDED should return EXIST");
    CHECK_STATUS(call_create(&unchecked_fh, child_name, UNCHECKED, 0600, NULL, NULL), NFS3ERR_NOTDIR, "CREATE with non-directory parent should return NOTDIR");
    CHECK_STATUS(call_create(&root_fh, long_name, UNCHECKED, 0600, NULL, NULL), NFS3ERR_NAMETOOLONG, "CREATE long name should return NAMETOOLONG");

cleanup:
    if (exclusive_created) {
        nfs3_remove_if_test_file(&root_fh, exclusive_name);
    }
    if (guarded_created) {
        nfs3_remove_if_test_file(&root_fh, guarded_name);
    }
    if (unchecked_created) {
        nfs3_remove_if_test_file(&root_fh, unchecked_name);
    }
    nfs_fh3_destroy(&exclusive_fh);
    nfs_fh3_destroy(&guarded_fh);
    nfs_fh3_destroy(&unchecked_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_mkdir_namespace(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t dir0700_fh;
    nfs_fh3_t dir0755_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&dir0700_fh);
    nfs_fh3_init(&dir0755_fh);
    nfs_fh3_init(&file_fh);
    char dir0700_name[128];
    char dir0755_name[128];
    char file_name[128];
    char child_name[128];
    nfs3_unique_name("c_ns_mkdir_0700", dir0700_name, sizeof(dir0700_name));
    nfs3_unique_name("c_ns_mkdir_0755", dir0755_name, sizeof(dir0755_name));
    nfs3_unique_name("c_ns_mkdir_file", file_name, sizeof(file_name));
    nfs3_unique_name("c_ns_mkdir_child", child_name, sizeof(child_name));
    int dir0700_created = 0;
    int dir0755_created = 0;
    int file_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(call_mkdir(&root_fh, dir0700_name, 0700, &dir0700_fh), NFS3_OK, "MKDIR mode 0700 should succeed");
    dir0700_created = 1;
    CHECK_STATUS(call_mkdir(&root_fh, dir0755_name, 0755, &dir0755_fh), NFS3_OK, "MKDIR mode 0755 should succeed");
    dir0755_created = 1;
    CHECK_STATUS(call_mkdir(&root_fh, dir0700_name, 0700, NULL), NFS3ERR_EXIST, "MKDIR duplicate should return EXIST");
    CHECK_STATUS(nfs3_create_test_file(&root_fh, file_name, &file_fh), NFS3_OK, "CREATE setup file should succeed");
    file_created = 1;
    CHECK_STATUS(call_mkdir(&file_fh, child_name, 0700, NULL), NFS3ERR_NOTDIR, "MKDIR with non-directory parent should return NOTDIR");

cleanup:
    if (file_created) {
        nfs3_remove_if_test_file(&root_fh, file_name);
    }
    if (dir0755_created) {
        nfs3_rmdir_test_dir(&root_fh, dir0755_name);
    }
    if (dir0700_created) {
        nfs3_rmdir_test_dir(&root_fh, dir0700_name);
    }
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&dir0755_fh);
    nfs_fh3_destroy(&dir0700_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_remove_namespace(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    char file_name[128];
    char dir_name[128];
    char missing_name[128];
    nfs3_unique_name("c_ns_remove_file", file_name, sizeof(file_name));
    nfs3_unique_name("c_ns_remove_dir", dir_name, sizeof(dir_name));
    nfs3_unique_name("c_ns_remove_missing", missing_name, sizeof(missing_name));
    int file_created = 0;
    int dir_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(nfs3_create_test_file(&root_fh, file_name, &file_fh), NFS3_OK, "CREATE setup file should succeed");
    file_created = 1;
    CHECK_STATUS(call_remove(&root_fh, file_name), NFS3_OK, "REMOVE existing file should succeed");
    file_created = 0;
    CHECK_STATUS(call_remove(&root_fh, missing_name), NFS3ERR_NOENT, "REMOVE nonexistent should return NOENT");
    CHECK_STATUS(nfs3_create_test_dir(&root_fh, dir_name, &dir_fh), NFS3_OK, "MKDIR setup dir should succeed");
    dir_created = 1;
    CHECK_STATUS(call_remove(&root_fh, dir_name), NFS3ERR_ISDIR, "REMOVE directory should fail with ISDIR");

cleanup:
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dir_name);
    }
    if (file_created) {
        nfs3_remove_if_test_file(&root_fh, file_name);
    }
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_rmdir_namespace(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t empty_dir_fh;
    nfs_fh3_t nonempty_dir_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t child_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&empty_dir_fh);
    nfs_fh3_init(&nonempty_dir_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&child_fh);
    char empty_dir_name[128];
    char nonempty_dir_name[128];
    char file_name[128];
    char child_name[128];
    char missing_name[128];
    nfs3_unique_name("c_ns_rmdir_empty", empty_dir_name, sizeof(empty_dir_name));
    nfs3_unique_name("c_ns_rmdir_nonempty", nonempty_dir_name, sizeof(nonempty_dir_name));
    nfs3_unique_name("c_ns_rmdir_file", file_name, sizeof(file_name));
    nfs3_unique_name("c_ns_rmdir_child", child_name, sizeof(child_name));
    nfs3_unique_name("c_ns_rmdir_missing", missing_name, sizeof(missing_name));
    int empty_dir_created = 0;
    int nonempty_dir_created = 0;
    int file_created = 0;
    int child_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(nfs3_create_test_dir(&root_fh, empty_dir_name, &empty_dir_fh), NFS3_OK, "MKDIR empty setup dir should succeed");
    empty_dir_created = 1;
    CHECK_STATUS(call_rmdir(&root_fh, empty_dir_name), NFS3_OK, "RMDIR empty should succeed");
    empty_dir_created = 0;
    CHECK_STATUS(call_rmdir(&root_fh, missing_name), NFS3ERR_NOENT, "RMDIR nonexistent should return NOENT");
    CHECK_STATUS(nfs3_create_test_dir(&root_fh, nonempty_dir_name, &nonempty_dir_fh), NFS3_OK, "MKDIR non-empty setup dir should succeed");
    nonempty_dir_created = 1;
    CHECK_STATUS(nfs3_create_test_file(&nonempty_dir_fh, child_name, &child_fh), NFS3_OK, "CREATE child file should succeed");
    child_created = 1;
    CHECK_STATUS(call_rmdir(&root_fh, nonempty_dir_name), NFS3ERR_NOTEMPTY, "RMDIR non-empty should return NOTEMPTY");
    CHECK_STATUS(nfs3_create_test_file(&root_fh, file_name, &file_fh), NFS3_OK, "CREATE setup file should succeed");
    file_created = 1;
    CHECK_STATUS(call_rmdir(&root_fh, file_name), NFS3ERR_NOTDIR, "RMDIR file name should fail with NOTDIR");

cleanup:
    if (child_created) {
        nfs3_remove_if_test_file(&nonempty_dir_fh, child_name);
    }
    if (file_created) {
        nfs3_remove_if_test_file(&root_fh, file_name);
    }
    if (nonempty_dir_created) {
        nfs3_rmdir_test_dir(&root_fh, nonempty_dir_name);
    }
    if (empty_dir_created) {
        nfs3_rmdir_test_dir(&root_fh, empty_dir_name);
    }
    nfs_fh3_destroy(&child_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&nonempty_dir_fh);
    nfs_fh3_destroy(&empty_dir_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_rename_namespace(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t overwrite_fh;
    nfs_fh3_t lookup_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&overwrite_fh);
    nfs_fh3_init(&lookup_fh);
    char file_from[128];
    char file_to[128];
    char dir_from[128];
    char dir_to[128];
    char overwrite_from[128];
    char overwrite_to[128];
    char missing_name[128];
    char same_name[128];
    nfs3_unique_name("c_ns_rename_file_from", file_from, sizeof(file_from));
    nfs3_unique_name("c_ns_rename_file_to", file_to, sizeof(file_to));
    nfs3_unique_name("c_ns_rename_dir_from", dir_from, sizeof(dir_from));
    nfs3_unique_name("c_ns_rename_dir_to", dir_to, sizeof(dir_to));
    nfs3_unique_name("c_ns_rename_overwrite_from", overwrite_from, sizeof(overwrite_from));
    nfs3_unique_name("c_ns_rename_overwrite_to", overwrite_to, sizeof(overwrite_to));
    nfs3_unique_name("c_ns_rename_missing", missing_name, sizeof(missing_name));
    nfs3_unique_name("c_ns_rename_same", same_name, sizeof(same_name));
    int file_to_exists = 0;
    int dir_to_exists = 0;
    int overwrite_to_exists = 0;
    int same_exists = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(nfs3_create_test_file(&root_fh, file_from, &file_fh), NFS3_OK, "CREATE file rename source should succeed");
    CHECK_STATUS(call_rename(&root_fh, file_from, &root_fh, file_to), NFS3_OK, "RENAME file should succeed");
    file_to_exists = 1;
    CHECK_STATUS(call_lookup(&root_fh, file_to, &lookup_fh), NFS3_OK, "LOOKUP renamed file should succeed");
    CHECK_STATUS(nfs3_create_test_dir(&root_fh, dir_from, &dir_fh), NFS3_OK, "MKDIR dir rename source should succeed");
    CHECK_STATUS(call_rename(&root_fh, dir_from, &root_fh, dir_to), NFS3_OK, "RENAME dir should succeed");
    dir_to_exists = 1;
    CHECK_STATUS(nfs3_create_test_file(&root_fh, overwrite_from, &overwrite_fh), NFS3_OK, "CREATE overwrite source should succeed");
    CHECK_STATUS(nfs3_create_test_file(&root_fh, overwrite_to, NULL), NFS3_OK, "CREATE overwrite target should succeed");
    overwrite_to_exists = 1;
    CHECK_STATUS(call_rename(&root_fh, overwrite_from, &root_fh, overwrite_to), NFS3_OK, "RENAME overwrite file should succeed");
    CHECK_STATUS(call_rename(&root_fh, missing_name, &root_fh, missing_name), NFS3ERR_NOENT, "RENAME nonexistent source should return NOENT");
    CHECK_STATUS(nfs3_create_test_file(&root_fh, same_name, NULL), NFS3_OK, "CREATE same-name source should succeed");
    same_exists = 1;
    CHECK_STATUS(call_rename(&root_fh, same_name, &root_fh, same_name), NFS3_OK, "RENAME same name should succeed");

cleanup:
    if (same_exists) {
        nfs3_remove_if_test_file(&root_fh, same_name);
    }
    if (overwrite_to_exists) {
        nfs3_remove_if_test_file(&root_fh, overwrite_to);
    }
    if (dir_to_exists) {
        nfs3_rmdir_test_dir(&root_fh, dir_to);
    }
    if (file_to_exists) {
        nfs3_remove_if_test_file(&root_fh, file_to);
    }
    nfs3_remove_if_test_file(&root_fh, overwrite_from);
    nfs3_rmdir_test_dir(&root_fh, dir_from);
    nfs3_remove_if_test_file(&root_fh, file_from);
    nfs_fh3_destroy(&lookup_fh);
    nfs_fh3_destroy(&overwrite_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_link_namespace(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t lookup_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&lookup_fh);
    char file_name[128];
    char link_name[128];
    char dir_name[128];
    nfs3_unique_name("c_ns_link_file", file_name, sizeof(file_name));
    nfs3_unique_name("c_ns_link_name", link_name, sizeof(link_name));
    nfs3_unique_name("c_ns_link_dir", dir_name, sizeof(dir_name));
    int file_created = 0;
    int link_created = 0;
    int dir_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(nfs3_create_test_file(&root_fh, file_name, &file_fh), NFS3_OK, "CREATE link source should succeed");
    file_created = 1;
    int status = call_link(&file_fh, &root_fh, link_name);
    if (link_unsupported((nfsstat3_t)status)) {
        goto cleanup;
    }
    CHECK_STATUS(status, NFS3_OK, "LINK regular file should succeed or return allowed unsupported status");
    link_created = 1;
    CHECK_STATUS(call_lookup(&root_fh, link_name, &lookup_fh), NFS3_OK, "LOOKUP hardlink should succeed");
    CHECK_STATUS(call_remove(&root_fh, file_name), NFS3_OK, "REMOVE hardlink original name should succeed");
    file_created = 0;
    CHECK_STATUS(call_lookup(&root_fh, link_name, &lookup_fh), NFS3_OK, "Hardlink should remain after removing original name");
    CHECK_STATUS(call_link(&lookup_fh, &root_fh, link_name), NFS3ERR_EXIST, "LINK duplicate target should return EXIST");
    CHECK_STATUS(nfs3_create_test_dir(&root_fh, dir_name, &dir_fh), NFS3_OK, "MKDIR link dir source should succeed");
    dir_created = 1;
    status = call_link(&dir_fh, &root_fh, file_name);
    CHECK_TRUE(status != NFS3_OK || link_unsupported((nfsstat3_t)status), "LINK directory should fail or return allowed unsupported status");

cleanup:
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dir_name);
    }
    if (link_created) {
        nfs3_remove_if_test_file(&root_fh, link_name);
    }
    if (file_created) {
        nfs3_remove_if_test_file(&root_fh, file_name);
    }
    nfs_fh3_destroy(&lookup_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

int main(void) {
    printf("=== NFSv3 Namespace Server Tests (C) ===\n\n");
    RUN_TEST(test_lookup_namespace);
    RUN_TEST(test_create_namespace);
    RUN_TEST(test_mkdir_namespace);
    RUN_TEST(test_remove_namespace);
    RUN_TEST(test_rmdir_namespace);
    RUN_TEST(test_rename_namespace);
    RUN_TEST(test_link_namespace);
    PRINT_SUMMARY();
    return tests_failed;
}
