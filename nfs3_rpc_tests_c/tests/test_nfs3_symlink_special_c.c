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

#define CHECK_STREQ(actual, expected, msg) do { \
    const char* _actual = (actual); \
    const char* _expected = (expected); \
    if (!_actual || !_expected || strcmp(_actual, _expected) != 0) { \
        printf("\n  FAIL  %s: %s - expected '%s', got '%s' (line %d)\n", current_test_name, (msg), _expected ? _expected : "(null)", _actual ? _actual : "(null)", __LINE__); \
        tests_failed++; \
        failed = 1; \
        goto cleanup; \
    } \
} while(0)

static const nfsstat3_t unsupported_statuses[] = {
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

static int unsupported_status(nfsstat3_t status) {
    return nfs3_status_is_allowed(status, unsupported_statuses, sizeof(unsupported_statuses) / sizeof(unsupported_statuses[0]));
}

static int call_lookup(const nfs_fh3_t* dir, const char* name, nfs_fh3_t* out_fh) {
    return nfs3_lookup_name(dir, name, out_fh);
}

static int call_symlink(const nfs_fh3_t* dir, const char* name, const char* target, nfs_fh3_t* out_fh) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    SYMLINK3args_t symlink_args;
    symlink3args_init(&symlink_args);
    nfs_fh3_set(&symlink_args.where_dir, dir->data, dir->len);
    symlink3args_set_name(&symlink_args, name);
    symlink3args_set_data(&symlink_args, target);
    symlink_args.symlink_attributes.mode_set = 1;
    symlink_args.symlink_attributes.mode = 0777;
    xdr_pack_SYMLINK3args(&args, &symlink_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_SYMLINK, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        SYMLINK3res_t res;
        symlink3res_init(&res);
        xdr_unpack_SYMLINK3res(&body, &res);
        nfs_status = (int)res.status;
        if (res.status == NFS3_OK && out_fh && !nfs_fh3_is_empty(&res.resok.object)) {
            nfs_fh3_set(out_fh, res.resok.object.data, res.resok.object.len);
        }
        symlink3res_destroy(&res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    symlink3args_destroy(&symlink_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int call_readlink(const nfs_fh3_t* fh, char** out_data) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    READLINK3args_t readlink_args;
    nfs_fh3_init(&readlink_args.symlink);
    nfs_fh3_set(&readlink_args.symlink, fh->data, fh->len);
    xdr_pack_READLINK3args(&args, &readlink_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_READLINK, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        READLINK3res_t res;
        readlink3res_init(&res);
        xdr_unpack_READLINK3res(&body, &res);
        nfs_status = (int)res.status;
        if (res.status == NFS3_OK && out_data) {
            *out_data = res.resok.data ? strdup(res.resok.data) : NULL;
        }
        readlink3res_destroy(&res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&readlink_args.symlink);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int call_mknod(const nfs_fh3_t* dir, const char* name, ftype4_t type, nfs_fh3_t* out_fh) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    MKNOD3args_t mknod_args;
    mknod3args_init(&mknod_args);
    nfs_fh3_set(&mknod_args.where_dir, dir->data, dir->len);
    mknod3args_set_name(&mknod_args, name);
    mknod_args.what_type = type;
    mknod_args.what_attributes.mode_set = 1;
    mknod_args.what_attributes.mode = 0600;
    xdr_pack_MKNOD3args(&args, &mknod_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_MKNOD, &args, &resp, &resp_len);
    int nfs_status = NFS3ERR_IO;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        MKNOD3res_t res;
        mknod3res_init(&res);
        xdr_unpack_MKNOD3res(&body, &res);
        nfs_status = (int)res.status;
        if (res.status == NFS3_OK && out_fh && !nfs_fh3_is_empty(&res.resok.object)) {
            nfs_fh3_set(out_fh, res.resok.object.data, res.resok.object.len);
        }
        mknod3res_destroy(&res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    mknod3args_destroy(&mknod_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static void test_symlink_readlink_targets(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t rel_fh;
    nfs_fh3_t abs_fh;
    nfs_fh3_t dangling_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&rel_fh);
    nfs_fh3_init(&abs_fh);
    nfs_fh3_init(&dangling_fh);
    char rel_name[128];
    char abs_name[128];
    char dangling_name[128];
    char missing_name[128];
    nfs3_unique_name("c_symlink_rel", rel_name, sizeof(rel_name));
    nfs3_unique_name("c_symlink_abs", abs_name, sizeof(abs_name));
    nfs3_unique_name("c_symlink_dangling", dangling_name, sizeof(dangling_name));
    nfs3_unique_name("c_symlink_missing", missing_name, sizeof(missing_name));
    const char* rel_target = "relative/target";
    const char* abs_target = "/absolute/target";
    char dangling_target[160];
    snprintf(dangling_target, sizeof(dangling_target), "%s_target", missing_name);
    char* read_data = NULL;
    int rel_created = 0;
    int abs_created = 0;
    int dangling_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    int status = call_symlink(&root_fh, rel_name, rel_target, &rel_fh);
    if (unsupported_status((nfsstat3_t)status)) {
        goto cleanup;
    }
    CHECK_STATUS(status, NFS3_OK, "SYMLINK relative target should succeed or return allowed unsupported status");
    rel_created = 1;
    if (nfs_fh3_is_empty(&rel_fh)) {
        CHECK_STATUS(call_lookup(&root_fh, rel_name, &rel_fh), NFS3_OK, "LOOKUP relative symlink should succeed");
    }
    CHECK_STATUS(call_readlink(&rel_fh, &read_data), NFS3_OK, "READLINK relative symlink should succeed");
    CHECK_STREQ(read_data, rel_target, "READLINK relative target should match");
    free(read_data);
    read_data = NULL;

    CHECK_STATUS(call_symlink(&root_fh, abs_name, abs_target, &abs_fh), NFS3_OK, "SYMLINK absolute target should succeed");
    abs_created = 1;
    if (nfs_fh3_is_empty(&abs_fh)) {
        CHECK_STATUS(call_lookup(&root_fh, abs_name, &abs_fh), NFS3_OK, "LOOKUP absolute symlink should succeed");
    }
    CHECK_STATUS(call_readlink(&abs_fh, &read_data), NFS3_OK, "READLINK absolute symlink should succeed");
    CHECK_STREQ(read_data, abs_target, "READLINK absolute target should match");
    free(read_data);
    read_data = NULL;

    CHECK_STATUS(call_symlink(&root_fh, dangling_name, dangling_target, &dangling_fh), NFS3_OK, "SYMLINK dangling target should succeed");
    dangling_created = 1;
    if (nfs_fh3_is_empty(&dangling_fh)) {
        CHECK_STATUS(call_lookup(&root_fh, dangling_name, &dangling_fh), NFS3_OK, "LOOKUP dangling symlink should succeed");
    }
    CHECK_STATUS(call_readlink(&dangling_fh, &read_data), NFS3_OK, "READLINK dangling symlink should succeed");
    CHECK_STREQ(read_data, dangling_target, "READLINK dangling target should match");

cleanup:
    free(read_data);
    if (dangling_created) {
        nfs3_remove_if_test_file(&root_fh, dangling_name);
    }
    if (abs_created) {
        nfs3_remove_if_test_file(&root_fh, abs_name);
    }
    if (rel_created) {
        nfs3_remove_if_test_file(&root_fh, rel_name);
    }
    nfs_fh3_destroy(&dangling_fh);
    nfs_fh3_destroy(&abs_fh);
    nfs_fh3_destroy(&rel_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_symlink_readlink_failures(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t symlink_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&symlink_fh);
    nfs_fh3_init(&file_fh);
    char symlink_name[128];
    char file_name[128];
    nfs3_unique_name("c_symlink_dup", symlink_name, sizeof(symlink_name));
    nfs3_unique_name("c_symlink_regular", file_name, sizeof(file_name));
    char* read_data = NULL;
    int symlink_created = 0;
    int file_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    int status = call_symlink(&root_fh, symlink_name, "target", &symlink_fh);
    if (unsupported_status((nfsstat3_t)status)) {
        goto cleanup;
    }
    CHECK_STATUS(status, NFS3_OK, "SYMLINK setup should succeed or return allowed unsupported status");
    symlink_created = 1;
    CHECK_STATUS(call_symlink(&root_fh, symlink_name, "other-target", NULL), NFS3ERR_EXIST, "SYMLINK duplicate name should return EXIST");
    CHECK_STATUS(nfs3_create_test_file(&root_fh, file_name, &file_fh), NFS3_OK, "CREATE regular file should succeed");
    file_created = 1;
    status = call_readlink(&file_fh, &read_data);
    CHECK_TRUE(status != NFS3_OK && !unsupported_status((nfsstat3_t)status), "READLINK regular file should fail without allowed unsupported status");

cleanup:
    free(read_data);
    if (file_created) {
        nfs3_remove_if_test_file(&root_fh, file_name);
    }
    if (symlink_created) {
        nfs3_remove_if_test_file(&root_fh, symlink_name);
    }
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&symlink_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_mknod_special(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t fifo_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&fifo_fh);
    char fifo_name[128];
    char chr_name[128];
    char blk_name[128];
    nfs3_unique_name("c_mknod_fifo", fifo_name, sizeof(fifo_name));
    nfs3_unique_name("c_mknod_chr", chr_name, sizeof(chr_name));
    nfs3_unique_name("c_mknod_blk", blk_name, sizeof(blk_name));
    int fifo_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    int status = call_mknod(&root_fh, fifo_name, NF4FIFO, &fifo_fh);
    if (unsupported_status((nfsstat3_t)status)) {
        goto cleanup;
    }
    CHECK_STATUS(status, NFS3_OK, "MKNOD FIFO should succeed or return allowed unsupported status");
    fifo_created = 1;
    CHECK_STATUS(call_mknod(&root_fh, fifo_name, NF4FIFO, NULL), NFS3ERR_EXIST, "MKNOD duplicate name should return EXIST");

    status = call_mknod(&root_fh, chr_name, NF4CHR, NULL);
    CHECK_TRUE(status == NFS3_OK || unsupported_status((nfsstat3_t)status), "MKNOD char device should succeed or return allowed unsupported/permission status");
    if (status == NFS3_OK) {
        nfs3_remove_if_test_file(&root_fh, chr_name);
    }

    status = call_mknod(&root_fh, blk_name, NF4BLK, NULL);
    CHECK_TRUE(status == NFS3_OK || unsupported_status((nfsstat3_t)status), "MKNOD block device should succeed or return allowed unsupported/permission status");
    if (status == NFS3_OK) {
        nfs3_remove_if_test_file(&root_fh, blk_name);
    }

cleanup:
    if (fifo_created) {
        nfs3_remove_if_test_file(&root_fh, fifo_name);
    }
    nfs_fh3_destroy(&fifo_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

int main(void) {
    printf("=== NFSv3 Symlink/Special Server Tests (C) ===\n\n");
    RUN_TEST(test_symlink_readlink_targets);
    RUN_TEST(test_symlink_readlink_failures);
    RUN_TEST(test_mknod_special);
    PRINT_SUMMARY();
    return tests_failed;
}
