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

static void mount_root(nfs_fh3_t* root_fh) {
    int status = nfs3_mount_root(root_fh);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT root should succeed");
    ASSERT_TRUE(!nfs_fh3_is_empty(root_fh), "root file handle should not be empty");
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

static int create_file(const nfs_fh3_t* root_fh, const char* filename, nfs_fh3_t* file_fh) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    CREATE3args_t create_args;
    create3args_init(&create_args);
    nfs_fh3_set(&create_args.where_dir, root_fh->data, root_fh->len);
    create3args_set_name(&create_args, filename);
    create_args.how_mode = UNCHECKED;
    create_args.how_attributes.mode_set = 1;
    create_args.how_attributes.mode = 0644;
    xdr_pack_CREATE3args(&args, &create_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_CREATE, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        CREATE3res_t create_res;
        create3res_init(&create_res);
        xdr_unpack_CREATE3res(&body, &create_res);
        nfs_status = (int)create_res.status;
        if (create_res.status == NFS3_OK && !nfs_fh3_is_empty(&create_res.resok.object)) {
            nfs_fh3_set(file_fh, create_res.resok.object.data, create_res.resok.object.len);
        }
        create3res_destroy(&create_res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    create3args_destroy(&create_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static void test_getattr_root(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_init(&root_fh);
    GETATTR3res_t res;
    memset(&res, 0, sizeof(res));

    mount_root(&root_fh);
    int status = call_getattr(&root_fh, &res);

    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "GETATTR RPC should succeed");
    ASSERT_EQ_U32((uint32_t)res.status, (uint32_t)NFS3_OK, "GETATTR root status");
    ASSERT_EQ_U32((uint32_t)res.resok.obj_attributes.type, (uint32_t)NF3DIR, "root should be a directory");

    nfs_fh3_destroy(&root_fh);
    TEST_PASS();
}

static void test_create_file_getattr_cleanup(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    char filename[128];
    nfs3_unique_name("c_getattr", filename, sizeof(filename));
    int created = 0;
    int failed = 0;

    mount_root(&root_fh);
    int status = create_file(&root_fh, filename, &file_fh);
    if (status != NFS3_OK || nfs_fh3_is_empty(&file_fh)) {
        printf("\n  FAIL  %s: CREATE should return OK and file handle (status %d, line %d)\n", current_test_name, status, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }
    created = 1;

    GETATTR3res_t res;
    memset(&res, 0, sizeof(res));
    status = call_getattr(&file_fh, &res);
    if (status != NFSTEST_RPC_OK || res.status != NFS3_OK || res.resok.obj_attributes.type != NF3REG) {
        printf("\n  FAIL  %s: GETATTR created file should return regular file (rpc/status/type %d/%u/%u, line %d)\n",
               current_test_name, status, (uint32_t)res.status, (uint32_t)res.resok.obj_attributes.type, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }

cleanup:
    if (created) {
        nfs3_remove_if_test_file(&root_fh, filename);
    }
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

int main(void) {
    printf("=== NFSv3 GETATTR Server Tests (C) ===\n\n");
    RUN_TEST(test_getattr_root);
    RUN_TEST(test_create_file_getattr_cleanup);
    PRINT_SUMMARY();
    return tests_failed;
}
