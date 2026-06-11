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

static int write_file(const nfs_fh3_t* file_fh, const uint8_t* payload, uint32_t payload_len) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    WRITE3args_t write_args;
    write3args_init(&write_args);
    nfs_fh3_set(&write_args.file, file_fh->data, file_fh->len);
    write_args.offset = 0;
    write_args.count = payload_len;
    write_args.stable = DATA_SYNC;
    write_args.data = (uint8_t*)malloc(payload_len);
    memcpy(write_args.data, payload, payload_len);
    write_args.data_len = payload_len;
    xdr_pack_WRITE3args(&args, &write_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_WRITE, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        WRITE3res_t write_res;
        memset(&write_res, 0, sizeof(write_res));
        xdr_unpack_WRITE3res(&body, &write_res);
        nfs_status = (write_res.status == NFS3_OK && write_res.resok.count == payload_len) ? NFS3_OK : (int)write_res.status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    write3args_destroy(&write_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int read_file(const nfs_fh3_t* file_fh, READ3res_t* read_res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    READ3args_t read_args;
    memset(&read_args, 0, sizeof(read_args));
    nfs_fh3_init(&read_args.file);
    nfs_fh3_set(&read_args.file, file_fh->data, file_fh->len);
    read_args.offset = 0;
    read_args.count = 1024;
    xdr_pack_READ3args(&args, &read_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_READ, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_READ3res(&body, read_res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&read_args.file);
    xdr_buf_destroy(&args);
    return status;
}

static void test_create_write_read_content_cleanup(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    char filename[128];
    nfs3_unique_name("c_readwrite", filename, sizeof(filename));
    const uint8_t payload[] = "C NFSv3 read/write payload";
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

    status = write_file(&file_fh, payload, (uint32_t)sizeof(payload));
    if (status != NFS3_OK) {
        printf("\n  FAIL  %s: WRITE should return OK and full count (status %d, line %d)\n", current_test_name, status, __LINE__);
        tests_failed++;
        failed = 1;
        goto cleanup;
    }

    READ3res_t read_res;
    read3res_init(&read_res);
    status = read_file(&file_fh, &read_res);
    if (status != NFSTEST_RPC_OK || read_res.status != NFS3_OK || read_res.resok.data_len != (uint32_t)sizeof(payload) ||
        memcmp(read_res.resok.data, payload, sizeof(payload)) != 0) {
        printf("\n  FAIL  %s: READ should return written content (rpc/status/len %d/%u/%u, line %d)\n",
               current_test_name, status, (uint32_t)read_res.status, read_res.resok.data_len, __LINE__);
        tests_failed++;
        failed = 1;
    }
    read3res_destroy(&read_res);

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
    printf("=== NFSv3 READ/WRITE Server Tests (C) ===\n\n");
    RUN_TEST(test_create_write_read_content_cleanup);
    PRINT_SUMMARY();
    return tests_failed;
}
