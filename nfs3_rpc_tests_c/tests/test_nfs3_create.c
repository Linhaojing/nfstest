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

static int access_file(const nfs_fh3_t* file_fh, uint32_t mask, ACCESS3res_t* access_res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    ACCESS3args_t access_args;
    memset(&access_args, 0, sizeof(access_args));
    nfs_fh3_init(&access_args.object);
    nfs_fh3_set(&access_args.object, file_fh->data, file_fh->len);
    access_args.access = mask;
    xdr_pack_ACCESS3args(&args, &access_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_ACCESS, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_ACCESS3res(&body, access_res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&access_args.object);
    xdr_buf_destroy(&args);
    return status;
}

static int remove_file(const nfs_fh3_t* root_fh, const char* filename) {
    return nfs3_remove_if_test_file(root_fh, filename);
}

static void test_create_access_remove_file(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    char filename[128];
    nfs3_unique_name("c_create", filename, sizeof(filename));
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

    ACCESS3res_t access_res;
    memset(&access_res, 0, sizeof(access_res));
    status = access_file(&file_fh, NFS3_ACCESS_READ | NFS3_ACCESS_MODIFY | NFS3_ACCESS_EXTEND, &access_res);
    if (status != NFSTEST_RPC_OK || access_res.status != NFS3_OK || (access_res.resok.access & NFS3_ACCESS_READ) == 0) {
        printf("\n  FAIL  %s: ACCESS should allow reading created file (rpc/status/access %d/%u/%u, line %d)\n",
               current_test_name, status, (uint32_t)access_res.status, access_res.resok.access, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }

    status = remove_file(&root_fh, filename);
    if (status != NFS3_OK) {
        printf("\n  FAIL  %s: REMOVE should return OK (status %d, line %d)\n", current_test_name, status, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }
    created = 0;

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
    printf("=== NFSv3 CREATE/ACCESS/REMOVE Server Tests (C) ===\n\n");
    RUN_TEST(test_create_access_remove_file);
    PRINT_SUMMARY();
    return tests_failed;
}
