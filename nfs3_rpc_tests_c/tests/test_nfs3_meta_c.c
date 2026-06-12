#include "test_framework.h"
#include "nfs3_server_test.h"
#include "nfstest/rpc_client.h"
#include "nfs3_c/nfs3_xdr.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ACCESS_READ    0x0001U
#define ACCESS_LOOKUP  0x0002U
#define ACCESS_MODIFY  0x0004U
#define ACCESS_EXTEND  0x0008U
#define ACCESS_DELETE  0x0010U
#define ACCESS_EXECUTE 0x0020U
#define ACCESS_ALL_BITS (ACCESS_READ | ACCESS_LOOKUP | ACCESS_MODIFY | ACCESS_EXTEND | ACCESS_DELETE | ACCESS_EXECUTE)

#define REQUIRE_SERVER_OR_SKIP() do { \
    if (!nfs3_server_is_configured()) { \
        if (nfs3_server_require_server()) { \
            ASSERT_TRUE(0, nfs3_server_skip_message()); \
        } \
        TEST_SKIP(nfs3_server_skip_message()); \
    } \
} while(0)

static const nfsstat3_t permission_statuses[] = {
    NFS3_OK,
    NFS3ERR_PERM,
    NFS3ERR_ACCES,
    NFS3ERR_ROFS,
    NFS3ERR_NOTSUPP
};

static void mount_root(nfs_fh3_t* root_fh) {
    int status = nfs3_mount_root(root_fh);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT root should succeed");
    ASSERT_TRUE(!nfs_fh3_is_empty(root_fh), "root file handle should not be empty");
}

static void make_invalid_fh(nfs_fh3_t* fh) {
    const uint8_t invalid_data[] = { 0xde, 0xad, 0xbe, 0xef };
    nfs_fh3_set(fh, invalid_data, sizeof(invalid_data));
}

static int status_allowed(nfsstat3_t status, const nfsstat3_t* allowed, size_t count) {
    return nfs3_status_is_allowed(status, allowed, count);
}

static int permission_status_allowed(nfsstat3_t status) {
    return status_allowed(status, permission_statuses, sizeof(permission_statuses) / sizeof(permission_statuses[0]));
}

static int call_null(void) {
    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_NULL, NULL, &resp, &resp_len);
    free(resp);
    return status;
}

static int call_getattr(const nfs_fh3_t* fh, GETATTR3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    GETATTR3args_t getattr_args;
    nfs_fh3_init(&getattr_args.object);
    nfs_fh3_set(&getattr_args.object, fh->data, fh->len);
    xdr_pack_GETATTR3args(&args, &getattr_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_GETATTR, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_GETATTR3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&getattr_args.object);
    xdr_buf_destroy(&args);
    return status;
}

static int call_setattr(const nfs_fh3_t* fh, const sattr3_t* attr, SETATTR3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    SETATTR3args_t setattr_args;
    memset(&setattr_args, 0, sizeof(setattr_args));
    nfs_fh3_init(&setattr_args.object);
    nfs_fh3_init(&setattr_args.guard);
    nfs_fh3_set(&setattr_args.object, fh->data, fh->len);
    setattr_args.new_attributes = *attr;
    setattr_args.has_guard = 0;
    xdr_pack_SETATTR3args(&args, &setattr_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_SETATTR, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_SETATTR3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&setattr_args.guard);
    nfs_fh3_destroy(&setattr_args.object);
    xdr_buf_destroy(&args);
    return status;
}

static int call_access(const nfs_fh3_t* fh, uint32_t access, ACCESS3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    ACCESS3args_t access_args;
    nfs_fh3_init(&access_args.object);
    nfs_fh3_set(&access_args.object, fh->data, fh->len);
    access_args.access = access;
    xdr_pack_ACCESS3args(&args, &access_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_ACCESS, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_ACCESS3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&access_args.object);
    xdr_buf_destroy(&args);
    return status;
}

static int call_fsstat(const nfs_fh3_t* fh, FSSTAT3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    FSSTAT3args_t fsstat_args;
    nfs_fh3_init(&fsstat_args.fsroot);
    nfs_fh3_set(&fsstat_args.fsroot, fh->data, fh->len);
    xdr_pack_FSSTAT3args(&args, &fsstat_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_FSSTAT, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_FSSTAT3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&fsstat_args.fsroot);
    xdr_buf_destroy(&args);
    return status;
}

static int call_fsinfo(const nfs_fh3_t* fh, FSINFO3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    FSINFO3args_t fsinfo_args;
    nfs_fh3_init(&fsinfo_args.fsroot);
    nfs_fh3_set(&fsinfo_args.fsroot, fh->data, fh->len);
    xdr_pack_FSINFO3args(&args, &fsinfo_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_FSINFO, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_FSINFO3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&fsinfo_args.fsroot);
    xdr_buf_destroy(&args);
    return status;
}

static int call_pathconf(const nfs_fh3_t* fh, PATHCONF3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    PATHCONF3args_t pathconf_args;
    nfs_fh3_init(&pathconf_args.object);
    nfs_fh3_set(&pathconf_args.object, fh->data, fh->len);
    xdr_pack_PATHCONF3args(&args, &pathconf_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_PATHCONF, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_PATHCONF3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&pathconf_args.object);
    xdr_buf_destroy(&args);
    return status;
}

static void assert_getattr_ok(const nfs_fh3_t* fh, ftype3_t expected_type, const char* message) {
    GETATTR3res_t res;
    memset(&res, 0, sizeof(res));
    int status = call_getattr(fh, &res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, message);
    ASSERT_EQ_U32((uint32_t)res.status, (uint32_t)NFS3_OK, message);
    ASSERT_EQ_U32((uint32_t)res.resok.obj_attributes.type, (uint32_t)expected_type, message);
}

static void assert_invalid_getattr(const nfs_fh3_t* fh) {
    GETATTR3res_t res;
    memset(&res, 0, sizeof(res));
    int status = call_getattr(fh, &res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "GETATTR invalid RPC should complete");
    ASSERT_TRUE(res.status != NFS3_OK, "GETATTR invalid handle should fail");
}

static int setup_objects(nfs_fh3_t* root_fh, nfs_fh3_t* file_fh, nfs_fh3_t* dir_fh, char* file_name, size_t file_name_len, char* dir_name, size_t dir_name_len) {
    mount_root(root_fh);
    nfs3_unique_name("c_meta_file", file_name, file_name_len);
    nfs3_unique_name("c_meta_dir", dir_name, dir_name_len);

    int status = nfs3_create_test_file(root_fh, file_name, file_fh);
    if (status != NFS3_OK || nfs_fh3_is_empty(file_fh)) {
        printf("\n  FAIL  %s: CREATE test file should succeed (status %d, line %d)\n", current_test_name, status, __LINE__);
        tests_failed++;
        return 0;
    }

    status = nfs3_create_test_dir(root_fh, dir_name, dir_fh);
    if (status != NFS3_OK || nfs_fh3_is_empty(dir_fh)) {
        printf("\n  FAIL  %s: MKDIR test dir should succeed (status %d, line %d)\n", current_test_name, status, __LINE__);
        tests_failed++;
        nfs3_remove_if_test_file(root_fh, file_name);
        return 0;
    }
    return 1;
}

static void cleanup_objects(nfs_fh3_t* root_fh, const char* file_name, const char* dir_name) {
    nfs3_remove_if_test_file(root_fh, file_name);
    nfs3_rmdir_test_dir(root_fh, dir_name);
}

static void test_null_proc(void) {
    REQUIRE_SERVER_OR_SKIP();
    int status = call_null();
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "NULL RPC should succeed");
    TEST_PASS();
}

static void test_getattr_root_file_dir_invalid(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t invalid_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&invalid_fh);
    char file_name[128];
    char dir_name[128];

    if (!setup_objects(&root_fh, &file_fh, &dir_fh, file_name, sizeof(file_name), dir_name, sizeof(dir_name))) {
        nfs_fh3_destroy(&dir_fh);
        nfs_fh3_destroy(&file_fh);
        nfs_fh3_destroy(&root_fh);
        return;
    }
    make_invalid_fh(&invalid_fh);

    assert_getattr_ok(&root_fh, NF3DIR, "GETATTR root should return directory");
    assert_getattr_ok(&file_fh, NF3REG, "GETATTR file should return regular file");
    assert_getattr_ok(&dir_fh, NF3DIR, "GETATTR dir should return directory");
    assert_invalid_getattr(&invalid_fh);

    cleanup_objects(&root_fh, file_name, dir_name);
    nfs_fh3_destroy(&invalid_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

static void test_access_masks_file_dir_invalid(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t invalid_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&invalid_fh);
    char file_name[128];
    char dir_name[128];
    const uint32_t masks[] = { 0, ACCESS_READ, ACCESS_LOOKUP, ACCESS_MODIFY, ACCESS_EXTEND, ACCESS_DELETE, ACCESS_EXECUTE, ACCESS_ALL_BITS };

    if (!setup_objects(&root_fh, &file_fh, &dir_fh, file_name, sizeof(file_name), dir_name, sizeof(dir_name))) {
        nfs_fh3_destroy(&invalid_fh);
        nfs_fh3_destroy(&dir_fh);
        nfs_fh3_destroy(&file_fh);
        nfs_fh3_destroy(&root_fh);
        return;
    }
    make_invalid_fh(&invalid_fh);

    for (size_t i = 0; i < sizeof(masks) / sizeof(masks[0]); i++) {
        ACCESS3res_t file_res;
        ACCESS3res_t dir_res;
        memset(&file_res, 0, sizeof(file_res));
        memset(&dir_res, 0, sizeof(dir_res));
        int status = call_access(&file_fh, masks[i], &file_res);
        ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "ACCESS file RPC should complete");
        ASSERT_TRUE(permission_status_allowed(file_res.status), "ACCESS file status should be allowed");
        if (file_res.status == NFS3_OK) {
            ASSERT_TRUE((file_res.resok.access & ~masks[i]) == 0, "ACCESS file should not grant unrequested bits");
        }

        status = call_access(&dir_fh, masks[i], &dir_res);
        ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "ACCESS dir RPC should complete");
        ASSERT_TRUE(permission_status_allowed(dir_res.status), "ACCESS dir status should be allowed");
        if (dir_res.status == NFS3_OK) {
            ASSERT_TRUE((dir_res.resok.access & ~masks[i]) == 0, "ACCESS dir should not grant unrequested bits");
        }
    }

    ACCESS3res_t invalid_res;
    memset(&invalid_res, 0, sizeof(invalid_res));
    int status = call_access(&invalid_fh, ACCESS_ALL_BITS, &invalid_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "ACCESS invalid RPC should complete");
    ASSERT_TRUE(invalid_res.status != NFS3_OK, "ACCESS invalid handle should fail");

    cleanup_objects(&root_fh, file_name, dir_name);
    nfs_fh3_destroy(&invalid_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

static void test_setattr_mode_size_time_uid_gid(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    char file_name[128];
    char dir_name[128];
    const char data[] = "abcdef";

    if (!setup_objects(&root_fh, &file_fh, &dir_fh, file_name, sizeof(file_name), dir_name, sizeof(dir_name))) {
        nfs_fh3_destroy(&dir_fh);
        nfs_fh3_destroy(&file_fh);
        nfs_fh3_destroy(&root_fh);
        return;
    }
    int write_status = nfs3_write_data(&file_fh, 0, FILE_SYNC, (const uint8_t*)data, sizeof(data) - 1);
    ASSERT_TRUE(permission_status_allowed((nfsstat3_t)write_status), "WRITE setup status should be allowed");

    sattr3_t attrs[5];
    memset(attrs, 0, sizeof(attrs));
    sattr3_init(&attrs[0]);
    attrs[0].mode_set = 1;
    attrs[0].mode = 0600;
    sattr3_init(&attrs[1]);
    attrs[1].size_set = 1;
    attrs[1].size = 2;
    sattr3_init(&attrs[2]);
    attrs[2].size_set = 1;
    attrs[2].size = 16;
    sattr3_init(&attrs[3]);
    attrs[3].atime_set = 1;
    attrs[3].mtime_set = 1;
    attrs[3].atime.seconds = 1;
    attrs[3].mtime.seconds = 1;
    sattr3_init(&attrs[4]);
    attrs[4].uid_set = 1;
    attrs[4].gid_set = 1;
    attrs[4].uid = 0;
    attrs[4].gid = 0;

    for (size_t i = 0; i < sizeof(attrs) / sizeof(attrs[0]); i++) {
        SETATTR3res_t res;
        memset(&res, 0, sizeof(res));
        int status = call_setattr(&file_fh, &attrs[i], &res);
        ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "SETATTR RPC should complete");
        ASSERT_TRUE(permission_status_allowed(res.status), "SETATTR status should be allowed");
    }

    cleanup_objects(&root_fh, file_name, dir_name);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

static void test_fsstat_root_dir_invalid(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t invalid_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&invalid_fh);
    char file_name[128];
    char dir_name[128];

    if (!setup_objects(&root_fh, &file_fh, &dir_fh, file_name, sizeof(file_name), dir_name, sizeof(dir_name))) {
        nfs_fh3_destroy(&invalid_fh);
        nfs_fh3_destroy(&dir_fh);
        nfs_fh3_destroy(&file_fh);
        nfs_fh3_destroy(&root_fh);
        return;
    }
    make_invalid_fh(&invalid_fh);

    FSSTAT3res_t root_res;
    FSSTAT3res_t dir_res;
    FSSTAT3res_t invalid_res;
    memset(&root_res, 0, sizeof(root_res));
    memset(&dir_res, 0, sizeof(dir_res));
    memset(&invalid_res, 0, sizeof(invalid_res));

    int status = call_fsstat(&root_fh, &root_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "FSSTAT root RPC should complete");
    ASSERT_EQ_U32((uint32_t)root_res.status, (uint32_t)NFS3_OK, "FSSTAT root status");
    ASSERT_TRUE(root_res.resok.tbytes >= root_res.resok.fbytes, "FSSTAT free bytes should not exceed total bytes");
    ASSERT_TRUE(root_res.resok.fbytes >= root_res.resok.abytes, "FSSTAT available bytes should not exceed free bytes");

    status = call_fsstat(&dir_fh, &dir_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "FSSTAT dir RPC should complete");
    ASSERT_EQ_U32((uint32_t)dir_res.status, (uint32_t)NFS3_OK, "FSSTAT dir status");
    ASSERT_TRUE(dir_res.resok.tbytes >= dir_res.resok.fbytes, "FSSTAT dir free bytes should not exceed total bytes");

    status = call_fsstat(&invalid_fh, &invalid_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "FSSTAT invalid RPC should complete");
    ASSERT_TRUE(invalid_res.status != NFS3_OK, "FSSTAT invalid handle should fail");

    cleanup_objects(&root_fh, file_name, dir_name);
    nfs_fh3_destroy(&invalid_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

static void test_fsinfo_root_file_invalid(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t invalid_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&invalid_fh);
    char file_name[128];
    char dir_name[128];

    if (!setup_objects(&root_fh, &file_fh, &dir_fh, file_name, sizeof(file_name), dir_name, sizeof(dir_name))) {
        nfs_fh3_destroy(&invalid_fh);
        nfs_fh3_destroy(&dir_fh);
        nfs_fh3_destroy(&file_fh);
        nfs_fh3_destroy(&root_fh);
        return;
    }
    make_invalid_fh(&invalid_fh);

    FSINFO3res_t root_res;
    FSINFO3res_t file_res;
    FSINFO3res_t invalid_res;
    memset(&root_res, 0, sizeof(root_res));
    memset(&file_res, 0, sizeof(file_res));
    memset(&invalid_res, 0, sizeof(invalid_res));

    int status = call_fsinfo(&root_fh, &root_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "FSINFO root RPC should complete");
    ASSERT_EQ_U32((uint32_t)root_res.status, (uint32_t)NFS3_OK, "FSINFO root status");
    ASSERT_TRUE(root_res.resok.rtmax > 0 && root_res.resok.rtpref > 0, "FSINFO read sizes should be positive");
    ASSERT_TRUE(root_res.resok.wtmax > 0 && root_res.resok.wtpref > 0, "FSINFO write sizes should be positive");
    ASSERT_TRUE(root_res.resok.rtpref <= root_res.resok.rtmax, "FSINFO rtpref should not exceed rtmax");
    ASSERT_TRUE(root_res.resok.wtpref <= root_res.resok.wtmax, "FSINFO wtpref should not exceed wtmax");
    ASSERT_TRUE(root_res.resok.time_delta_nseconds < 1000000000U, "FSINFO time delta nsec should be valid");

    status = call_fsinfo(&file_fh, &file_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "FSINFO file RPC should complete");
    ASSERT_EQ_U32((uint32_t)file_res.status, (uint32_t)NFS3_OK, "FSINFO file status");
    ASSERT_TRUE(file_res.resok.rtmax > 0, "FSINFO file rtmax should be positive");

    status = call_fsinfo(&invalid_fh, &invalid_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "FSINFO invalid RPC should complete");
    ASSERT_TRUE(invalid_res.status != NFS3_OK, "FSINFO invalid handle should fail");

    cleanup_objects(&root_fh, file_name, dir_name);
    nfs_fh3_destroy(&invalid_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

static void test_pathconf_root_dir_invalid(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t invalid_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&invalid_fh);
    char file_name[128];
    char dir_name[128];

    if (!setup_objects(&root_fh, &file_fh, &dir_fh, file_name, sizeof(file_name), dir_name, sizeof(dir_name))) {
        nfs_fh3_destroy(&invalid_fh);
        nfs_fh3_destroy(&dir_fh);
        nfs_fh3_destroy(&file_fh);
        nfs_fh3_destroy(&root_fh);
        return;
    }
    make_invalid_fh(&invalid_fh);

    PATHCONF3res_t root_res;
    PATHCONF3res_t dir_res;
    PATHCONF3res_t invalid_res;
    memset(&root_res, 0, sizeof(root_res));
    memset(&dir_res, 0, sizeof(dir_res));
    memset(&invalid_res, 0, sizeof(invalid_res));

    int status = call_pathconf(&root_fh, &root_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "PATHCONF root RPC should complete");
    ASSERT_EQ_U32((uint32_t)root_res.status, (uint32_t)NFS3_OK, "PATHCONF root status");
    ASSERT_TRUE(root_res.resok.info.linkmax >= 0, "PATHCONF linkmax should be non-negative");
    ASSERT_TRUE(root_res.resok.info.name_max > 0, "PATHCONF name_max should be positive");

    status = call_pathconf(&dir_fh, &dir_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "PATHCONF dir RPC should complete");
    ASSERT_EQ_U32((uint32_t)dir_res.status, (uint32_t)NFS3_OK, "PATHCONF dir status");
    ASSERT_TRUE(dir_res.resok.info.name_max > 0, "PATHCONF dir name_max should be positive");

    status = call_pathconf(&invalid_fh, &invalid_res);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "PATHCONF invalid RPC should complete");
    ASSERT_TRUE(invalid_res.status != NFS3_OK, "PATHCONF invalid handle should fail");

    cleanup_objects(&root_fh, file_name, dir_name);
    nfs_fh3_destroy(&invalid_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

int main(void) {
    printf("=== NFSv3 Meta Server Tests (C) ===\n\n");
    RUN_TEST(test_null_proc);
    RUN_TEST(test_getattr_root_file_dir_invalid);
    RUN_TEST(test_access_masks_file_dir_invalid);
    RUN_TEST(test_setattr_mode_size_time_uid_gid);
    RUN_TEST(test_fsstat_root_dir_invalid);
    RUN_TEST(test_fsinfo_root_file_invalid);
    RUN_TEST(test_pathconf_root_dir_invalid);
    PRINT_SUMMARY();
    return tests_failed;
}
