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

#define CHECK_NFS_OK_OR_NOTSUPP(actual, msg) do { \
    int _actual = (actual); \
    if (_actual != NFS3_OK && _actual != NFS3ERR_NOTSUPP) { \
        printf("\n  FAIL  %s: %s - expected OK or NOTSUPP, got %d (line %d)\n", current_test_name, (msg), _actual, __LINE__); \
        tests_failed++; \
        failed = 1; \
        goto cleanup; \
    } \
} while(0)

static void mount_root(nfs_fh3_t* root_fh) {
    int status = nfs3_mount_root(root_fh);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT root should succeed");
    ASSERT_TRUE(!nfs_fh3_is_empty(root_fh), "root file handle should not be empty");
}

static void make_invalid_handle(nfs_fh3_t* fh) {
    const uint8_t bad_handle[] = "not-a-valid-nfs-file-handle";
    nfs_fh3_set(fh, bad_handle, (uint32_t)(sizeof(bad_handle) - 1));
}

static int call_read(const nfs_fh3_t* file, uint64_t offset, uint32_t count, READ3res_t* out) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    READ3args_t read_args;
    memset(&read_args, 0, sizeof(read_args));
    nfs_fh3_init(&read_args.file);
    nfs_fh3_set(&read_args.file, file->data, file->len);
    read_args.offset = offset;
    read_args.count = count;
    xdr_pack_READ3args(&args, &read_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_READ, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        read3res_init(out);
        xdr_unpack_READ3res(&body, out);
        nfs_status = (int)out->status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&read_args.file);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? (resp == NULL ? NFSTEST_RPC_PROTO_ERR : nfs_status) : rpc_status;
}

static int call_write(const nfs_fh3_t* file, uint64_t offset, stable_how_t stable, const uint8_t* data, uint32_t count, WRITE3res_t* out) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    WRITE3args_t write_args;
    write3args_init(&write_args);
    nfs_fh3_set(&write_args.file, file->data, file->len);
    write_args.offset = offset;
    write_args.count = count;
    write_args.stable = stable;
    write_args.data = count > 0 ? (uint8_t*)malloc(count) : NULL;
    if (count > 0) {
        memcpy(write_args.data, data, count);
    }
    write_args.data_len = count;
    xdr_pack_WRITE3args(&args, &write_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_WRITE, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        memset(out, 0, sizeof(*out));
        xdr_unpack_WRITE3res(&body, out);
        nfs_status = (int)out->status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    write3args_destroy(&write_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? (resp == NULL ? NFSTEST_RPC_PROTO_ERR : nfs_status) : rpc_status;
}

static int call_commit(const nfs_fh3_t* file, uint64_t offset, uint32_t count, COMMIT3res_t* out) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    COMMIT3args_t commit_args;
    memset(&commit_args, 0, sizeof(commit_args));
    nfs_fh3_init(&commit_args.file);
    nfs_fh3_set(&commit_args.file, file->data, file->len);
    commit_args.offset = offset;
    commit_args.count = count;
    xdr_pack_COMMIT3args(&args, &commit_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_COMMIT, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        memset(out, 0, sizeof(*out));
        xdr_unpack_COMMIT3res(&body, out);
        nfs_status = (int)out->status;
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&commit_args.file);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? (resp == NULL ? NFSTEST_RPC_PROTO_ERR : nfs_status) : rpc_status;
}

static void expect_read_data(const nfs_fh3_t* file, uint64_t offset, uint32_t count, const uint8_t* expected, uint32_t expected_len, int expect_eof, int* failed) {
    READ3res_t res;
    read3res_init(&res);
    int status = call_read(file, offset, count, &res);
    if (status != NFS3_OK || res.resok.data_len != expected_len || res.resok.eof != expect_eof || (expected_len > 0 && memcmp(res.resok.data, expected, expected_len) != 0)) {
        printf("\n  FAIL  %s: READ offset %lu count %u returned status/len/eof %d/%u/%u (line %d)\n",
               current_test_name, (unsigned long)offset, count, status, res.resok.data_len, (uint32_t)res.resok.eof, __LINE__);
        tests_failed++;
        *failed = 1;
    }
    read3res_destroy(&res);
}

static void test_read_offsets_counts_and_failures(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t invalid_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&invalid_fh);
    char filename[128];
    char dirname[128];
    nfs3_unique_name("c_io_read_file", filename, sizeof(filename));
    nfs3_unique_name("c_io_read_dir", dirname, sizeof(dirname));
    const uint8_t payload[] = "0123456789abcdef";
    int file_created = 0;
    int dir_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(nfs3_create_test_file(&root_fh, filename, &file_fh), NFS3_OK, "CREATE test file should succeed");
    file_created = 1;
    CHECK_STATUS(nfs3_write_data(&file_fh, 0, FILE_SYNC, payload, sizeof(payload) - 1), NFS3_OK, "WRITE initial payload should succeed");
    CHECK_STATUS(nfs3_create_test_dir(&root_fh, dirname, &dir_fh), NFS3_OK, "MKDIR test dir should succeed");
    dir_created = 1;

    expect_read_data(&file_fh, 0, 16, payload, 16, 1, &failed);
    if (failed) goto cleanup;
    expect_read_data(&file_fh, 5, 6, payload + 5, 6, 0, &failed);
    if (failed) goto cleanup;
    expect_read_data(&file_fh, 16, 8, payload + 16, 0, 1, &failed);
    if (failed) goto cleanup;
    expect_read_data(&file_fh, 32, 8, payload + 16, 0, 1, &failed);
    if (failed) goto cleanup;
    expect_read_data(&file_fh, 0, 0, payload, 0, 0, &failed);
    if (failed) goto cleanup;
    expect_read_data(&file_fh, 0, 4, payload, 4, 0, &failed);
    if (failed) goto cleanup;
    expect_read_data(&file_fh, 0, 64, payload, 16, 1, &failed);
    if (failed) goto cleanup;

    make_invalid_handle(&invalid_fh);
    READ3res_t read_res;
    read3res_init(&read_res);
    int status = call_read(&invalid_fh, 0, 8, &read_res);
    CHECK_TRUE(status != NFS3_OK, "READ with invalid handle should fail");
    read3res_destroy(&read_res);

    read3res_init(&read_res);
    status = call_read(&dir_fh, 0, 8, &read_res);
    CHECK_TRUE(status != NFS3_OK, "READ with directory handle should fail");
    read3res_destroy(&read_res);

cleanup:
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dirname);
    }
    if (file_created) {
        nfs3_remove_test_file(&root_fh, filename);
    }
    nfs_fh3_destroy(&invalid_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_write_stability_offsets_counts_and_failures(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t invalid_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&invalid_fh);
    char filename[128];
    char dirname[128];
    nfs3_unique_name("c_io_write_file", filename, sizeof(filename));
    nfs3_unique_name("c_io_write_dir", dirname, sizeof(dirname));
    const uint8_t unstable_payload[] = "unstable";
    const uint8_t data_sync_payload[] = "DATA";
    const uint8_t file_sync_payload[] = "file-sync";
    const uint8_t overwrite_payload[] = "MID";
    const uint8_t append_payload[] = "APPEND";
    const uint8_t large_payload[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int file_created = 0;
    int dir_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(nfs3_create_test_file(&root_fh, filename, &file_fh), NFS3_OK, "CREATE test file should succeed");
    file_created = 1;
    CHECK_STATUS(nfs3_create_test_dir(&root_fh, dirname, &dir_fh), NFS3_OK, "MKDIR test dir should succeed");
    dir_created = 1;

    WRITE3res_t write_res;
    memset(&write_res, 0, sizeof(write_res));
    CHECK_STATUS(call_write(&file_fh, 0, UNSTABLE, unstable_payload, (uint32_t)(sizeof(unstable_payload) - 1), &write_res), NFS3_OK, "WRITE UNSTABLE at offset 0 should succeed");
    CHECK_STATUS((int)write_res.resok.count, (int)(sizeof(unstable_payload) - 1), "WRITE UNSTABLE should report full count");
    CHECK_STATUS(call_write(&file_fh, 8, DATA_SYNC, data_sync_payload, (uint32_t)(sizeof(data_sync_payload) - 1), &write_res), NFS3_OK, "WRITE DATA_SYNC at non-zero offset should succeed");
    CHECK_STATUS((int)write_res.resok.count, (int)(sizeof(data_sync_payload) - 1), "WRITE DATA_SYNC should report full count");
    CHECK_STATUS(call_write(&file_fh, 12, FILE_SYNC, file_sync_payload, (uint32_t)(sizeof(file_sync_payload) - 1), &write_res), NFS3_OK, "WRITE FILE_SYNC should succeed");
    CHECK_STATUS((int)write_res.resok.count, (int)(sizeof(file_sync_payload) - 1), "WRITE FILE_SYNC should report full count");
    CHECK_STATUS(call_write(&file_fh, 2, FILE_SYNC, overwrite_payload, (uint32_t)(sizeof(overwrite_payload) - 1), &write_res), NFS3_OK, "WRITE overwrite middle should succeed");
    CHECK_STATUS(call_write(&file_fh, 21, FILE_SYNC, append_payload, (uint32_t)(sizeof(append_payload) - 1), &write_res), NFS3_OK, "WRITE append should succeed");
    CHECK_STATUS(call_write(&file_fh, 0, FILE_SYNC, NULL, 0, &write_res), NFS3_OK, "WRITE count 0 should succeed");
    CHECK_STATUS((int)write_res.resok.count, 0, "WRITE count 0 should report zero count");
    CHECK_STATUS(call_write(&file_fh, 27, DATA_SYNC, large_payload, (uint32_t)(sizeof(large_payload) - 1), &write_res), NFS3_OK, "WRITE larger payload should succeed");
    CHECK_STATUS((int)write_res.resok.count, (int)(sizeof(large_payload) - 1), "WRITE larger payload should report full count");

    make_invalid_handle(&invalid_fh);
    CHECK_TRUE(call_write(&invalid_fh, 0, FILE_SYNC, unstable_payload, (uint32_t)(sizeof(unstable_payload) - 1), &write_res) != NFS3_OK, "WRITE with invalid handle should fail");
    CHECK_TRUE(call_write(&dir_fh, 0, FILE_SYNC, unstable_payload, (uint32_t)(sizeof(unstable_payload) - 1), &write_res) != NFS3_OK, "WRITE with directory handle should fail");

cleanup:
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dirname);
    }
    if (file_created) {
        nfs3_remove_test_file(&root_fh, filename);
    }
    nfs_fh3_destroy(&invalid_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_commit_success_variants_and_invalid_handle(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_t invalid_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    nfs_fh3_init(&invalid_fh);
    char filename[128];
    nfs3_unique_name("c_io_commit_file", filename, sizeof(filename));
    const uint8_t payload[] = "unstable-commit-payload";
    int file_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(nfs3_create_test_file(&root_fh, filename, &file_fh), NFS3_OK, "CREATE test file should succeed");
    file_created = 1;
    CHECK_STATUS(nfs3_write_data(&file_fh, 0, UNSTABLE, payload, sizeof(payload) - 1), NFS3_OK, "UNSTABLE WRITE before COMMIT should succeed");

    COMMIT3res_t commit_res;
    CHECK_NFS_OK_OR_NOTSUPP(call_commit(&file_fh, 0, (uint32_t)(sizeof(payload) - 1), &commit_res), "COMMIT after UNSTABLE write should succeed or be unsupported");
    CHECK_NFS_OK_OR_NOTSUPP(call_commit(&file_fh, 0, 0, &commit_res), "COMMIT count 0 should succeed or be unsupported");
    CHECK_NFS_OK_OR_NOTSUPP(call_commit(&file_fh, 1024, 16, &commit_res), "COMMIT beyond file size should succeed or be unsupported");
    CHECK_NFS_OK_OR_NOTSUPP(call_commit(&file_fh, 0, 16, &commit_res), "COMMIT without pending unstable write should succeed or be unsupported");

    make_invalid_handle(&invalid_fh);
    CHECK_TRUE(call_commit(&invalid_fh, 0, 0, &commit_res) != NFS3_OK, "COMMIT with invalid handle should fail");

cleanup:
    if (file_created) {
        nfs3_remove_test_file(&root_fh, filename);
    }
    nfs_fh3_destroy(&invalid_fh);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

int main(void) {
    printf("=== NFSv3 IO Server Tests (C) ===\n\n");
    RUN_TEST(test_read_offsets_counts_and_failures);
    RUN_TEST(test_write_stability_offsets_counts_and_failures);
    RUN_TEST(test_commit_success_variants_and_invalid_handle);
    PRINT_SUMMARY();
    return tests_failed;
}
