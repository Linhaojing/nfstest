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

static int create_file(const nfs_fh3_t* root_fh, const char* filename) {
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
        create3res_destroy(&create_res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    create3args_destroy(&create_args);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK ? nfs_status : rpc_status;
}

static int read_dir(const nfs_fh3_t* root_fh, READDIR3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    READDIR3args_t readdir_args;
    memset(&readdir_args, 0, sizeof(readdir_args));
    nfs_fh3_init(&readdir_args.dir);
    nfs_fh3_set(&readdir_args.dir, root_fh->data, root_fh->len);
    readdir_args.cookie = 0;
    readdir_args.cookieverf = 0;
    readdir_args.count = 8192;
    xdr_pack_READDIR3args(&args, &readdir_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfs3_server_call(NFSPROC3_READDIR, &args, &resp, &resp_len);
    if (status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_READDIR3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&readdir_args.dir);
    xdr_buf_destroy(&args);
    return status;
}

static int has_entry(const READDIR3res_t* res, const char* name) {
    for (entry3_t* entry = res->resok.reply.entries; entry != NULL; entry = entry->nextentry) {
        if (entry->name != NULL && strcmp(entry->name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

static void test_readdir_lists_created_files_cleanup(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_init(&root_fh);
    char names[3][128];
    int created[3] = {0, 0, 0};
    int failed = 0;

    mount_root(&root_fh);
    for (int i = 0; i < 3; ++i) {
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "c_readdir_%d", i);
        nfs3_unique_name(prefix, names[i], sizeof(names[i]));
        int status = create_file(&root_fh, names[i]);
        if (status != NFS3_OK) {
            printf("\n  FAIL  %s: CREATE %s should return OK (status %d, line %d)\n", current_test_name, names[i], status, __LINE__);
            tests_failed++;
            failed = 1;
            goto cleanup;
        }
        created[i] = 1;
    }

    READDIR3res_t readdir_res;
    readdir3res_init(&readdir_res);
    int status = read_dir(&root_fh, &readdir_res);
    if (status != NFSTEST_RPC_OK || readdir_res.status != NFS3_OK ||
        !has_entry(&readdir_res, names[0]) || !has_entry(&readdir_res, names[1]) || !has_entry(&readdir_res, names[2])) {
        printf("\n  FAIL  %s: READDIR should include created entries (rpc/status %d/%u, line %d)\n",
               current_test_name, status, (uint32_t)readdir_res.status, __LINE__);
        tests_failed++;
        failed = 1;
    }
    readdir3res_destroy(&readdir_res);

cleanup:
    for (int i = 0; i < 3; ++i) {
        if (created[i]) {
            nfs3_remove_if_test_file(&root_fh, names[i]);
        }
    }
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

int main(void) {
    printf("=== NFSv3 READDIR Server Tests (C) ===\n\n");
    RUN_TEST(test_readdir_lists_created_files_cleanup);
    PRINT_SUMMARY();
    return tests_failed;
}
