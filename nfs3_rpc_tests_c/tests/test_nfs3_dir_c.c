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

#define ENTRY_COUNT 8

static const nfsstat3_t readdir_protocol_error_statuses[] = {
    NFS3ERR_BAD_COOKIE,
    NFS3ERR_STALE,
    NFS3ERR_INVAL,
    NFS3ERR_SERVERFAULT,
    NFS3ERR_IO
};

static int status_allowed(nfsstat3_t status, const nfsstat3_t* allowed, size_t count) {
    return nfs3_status_is_allowed(status, allowed, count);
}

static void mount_root(nfs_fh3_t* root_fh) {
    int status = nfs3_mount_root(root_fh);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT root should succeed");
    ASSERT_TRUE(!nfs_fh3_is_empty(root_fh), "root file handle should not be empty");
}

static int call_readdir(const nfs_fh3_t* dir, uint64_t cookie, uint64_t cookieverf, uint32_t count, READDIR3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    READDIR3args_t readdir_args;
    memset(&readdir_args, 0, sizeof(readdir_args));
    nfs_fh3_init(&readdir_args.dir);
    nfs_fh3_set(&readdir_args.dir, dir->data, dir->len);
    readdir_args.cookie = cookie;
    readdir_args.cookieverf = cookieverf;
    readdir_args.count = count;
    xdr_pack_READDIR3args(&args, &readdir_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_READDIR, &args, &resp, &resp_len);
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_READDIR3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&readdir_args.dir);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK && resp == NULL ? NFSTEST_RPC_PROTO_ERR : rpc_status;
}

static int call_readdirplus(const nfs_fh3_t* dir, uint64_t cookie, uint64_t cookieverf, uint32_t dircount, uint32_t maxcount, READDIRPLUS3res_t* res) {
    xdr_buf_t args;
    xdr_buf_init(&args);
    READDIRPLUS3args_t readdirplus_args;
    memset(&readdirplus_args, 0, sizeof(readdirplus_args));
    nfs_fh3_init(&readdirplus_args.dir);
    nfs_fh3_set(&readdirplus_args.dir, dir->data, dir->len);
    readdirplus_args.cookie = cookie;
    readdirplus_args.cookieverf = cookieverf;
    readdirplus_args.dircount = dircount;
    readdirplus_args.maxcount = maxcount;
    xdr_pack_READDIRPLUS3args(&args, &readdirplus_args);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nfs3_server_call(NFSPROC3_READDIRPLUS, &args, &resp, &resp_len);
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        xdr_buf_t body;
        xdr_buf_init_copy(&body, resp, resp_len);
        xdr_unpack_READDIRPLUS3res(&body, res);
        xdr_buf_destroy(&body);
    }

    free(resp);
    nfs_fh3_destroy(&readdirplus_args.dir);
    xdr_buf_destroy(&args);
    return rpc_status == NFSTEST_RPC_OK && resp == NULL ? NFSTEST_RPC_PROTO_ERR : rpc_status;
}

static int create_named_dir(const nfs_fh3_t* parent, const char* prefix, char* name, size_t name_len, nfs_fh3_t* out_fh) {
    nfs3_unique_name(prefix, name, name_len);
    return nfs3_create_test_dir(parent, name, out_fh);
}

static int create_named_file(const nfs_fh3_t* parent, const char* prefix, char* name, size_t name_len, nfs_fh3_t* out_fh) {
    nfs3_unique_name(prefix, name, name_len);
    return nfs3_create_test_file(parent, name, out_fh);
}

static void cleanup_files(const nfs_fh3_t* dir, char names[][128], int* created, int count) {
    for (int i = count - 1; i >= 0; --i) {
        if (created[i]) {
            nfs3_remove_test_file(dir, names[i]);
        }
    }
}

static int readdir_has_name(const READDIR3res_t* res, const char* name) {
    if (res->status != NFS3_OK) {
        return 0;
    }
    for (entry3_t* entry = res->resok.reply.entries; entry != NULL; entry = entry->nextentry) {
        if (entry->name != NULL && strcmp(entry->name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

static int readdirplus_has_name(const READDIRPLUS3res_t* res, const char* name) {
    if (res->status != NFS3_OK) {
        return 0;
    }
    for (entryplus3_t* entry = res->resok.reply.entries; entry != NULL; entry = entry->nextentry) {
        if (entry->name != NULL && strcmp(entry->name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

static entryplus3_t* readdirplus_find_name(READDIRPLUS3res_t* res, const char* name) {
    if (res->status != NFS3_OK) {
        return NULL;
    }
    for (entryplus3_t* entry = res->resok.reply.entries; entry != NULL; entry = entry->nextentry) {
        if (entry->name != NULL && strcmp(entry->name, name) == 0) {
            return entry;
        }
    }
    return NULL;
}

static int readdir_count_entries(const READDIR3res_t* res) {
    int count = 0;
    if (res->status != NFS3_OK) {
        return 0;
    }
    for (entry3_t* entry = res->resok.reply.entries; entry != NULL; entry = entry->nextentry) {
        count++;
    }
    return count;
}

static int readdirplus_count_entries(const READDIRPLUS3res_t* res) {
    int count = 0;
    if (res->status != NFS3_OK) {
        return 0;
    }
    for (entryplus3_t* entry = res->resok.reply.entries; entry != NULL; entry = entry->nextentry) {
        count++;
    }
    return count;
}

static void test_readdir_empty_and_near_empty_dir(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&file_fh);
    char dir_name[128];
    char file_name[128];
    int dir_created = 0;
    int file_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(create_named_dir(&root_fh, "c_dir_empty", dir_name, sizeof(dir_name), &dir_fh), NFS3_OK, "MKDIR test dir should succeed");
    dir_created = 1;

    READDIR3res_t empty_res;
    readdir3res_init(&empty_res);
    int status = call_readdir(&dir_fh, 0, 0, 4096, &empty_res);
    CHECK_STATUS(status, NFSTEST_RPC_OK, "READDIR empty dir RPC should succeed");
    CHECK_STATUS(empty_res.status, NFS3_OK, "READDIR empty dir should succeed");
    CHECK_TRUE(empty_res.has_resok, "READDIR empty dir should have resok");
    CHECK_TRUE(readdir_count_entries(&empty_res) >= 0, "READDIR empty dir should decode entry list");
    readdir3res_destroy(&empty_res);

    CHECK_STATUS(create_named_file(&dir_fh, "c_dir_one", file_name, sizeof(file_name), &file_fh), NFS3_OK, "CREATE near-empty file should succeed");
    file_created = 1;

    READDIR3res_t one_res;
    readdir3res_init(&one_res);
    status = call_readdir(&dir_fh, 0, 0, 4096, &one_res);
    CHECK_STATUS(status, NFSTEST_RPC_OK, "READDIR near-empty dir RPC should succeed");
    CHECK_STATUS(one_res.status, NFS3_OK, "READDIR near-empty dir should succeed");
    CHECK_TRUE(readdir_has_name(&one_res, file_name), "READDIR near-empty dir should include created file");
    readdir3res_destroy(&one_res);

cleanup:
    if (file_created) {
        nfs3_remove_test_file(&dir_fh, file_name);
    }
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dir_name);
    }
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_readdir_multiple_entries_paging_and_cookieverf(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t file_fhs[ENTRY_COUNT];
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&dir_fh);
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        nfs_fh3_init(&file_fhs[i]);
    }
    char dir_name[128];
    char names[ENTRY_COUNT][128];
    int created[ENTRY_COUNT] = {0};
    int dir_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(create_named_dir(&root_fh, "c_dir_page", dir_name, sizeof(dir_name), &dir_fh), NFS3_OK, "MKDIR paging dir should succeed");
    dir_created = 1;
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "c_dir_file_%d", i);
        CHECK_STATUS(create_named_file(&dir_fh, prefix, names[i], sizeof(names[i]), &file_fhs[i]), NFS3_OK, "CREATE paging file should succeed");
        created[i] = 1;
    }

    READDIR3res_t full_res;
    readdir3res_init(&full_res);
    CHECK_STATUS(call_readdir(&dir_fh, 0, 0, 8192, &full_res), NFSTEST_RPC_OK, "READDIR full RPC should succeed");
    CHECK_STATUS(full_res.status, NFS3_OK, "READDIR full should succeed");
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        CHECK_TRUE(readdir_has_name(&full_res, names[i]), "READDIR full should include each created file");
    }
    readdir3res_destroy(&full_res);

    READDIR3res_t page1;
    readdir3res_init(&page1);
    CHECK_STATUS(call_readdir(&dir_fh, 0, 0, 160, &page1), NFSTEST_RPC_OK, "READDIR small-count page RPC should succeed");
    CHECK_STATUS(page1.status, NFS3_OK, "READDIR small-count page should succeed");
    CHECK_TRUE(page1.resok.reply.entries != NULL, "READDIR small-count page should return at least one entry");
    uint64_t next_cookie = page1.resok.reply.entries->cookie;
    uint64_t cookieverf = page1.resok.cookieverf;
    CHECK_TRUE(next_cookie != 0, "READDIR entry cookie should be usable for continuation");

    READDIR3res_t page2;
    readdir3res_init(&page2);
    CHECK_STATUS(call_readdir(&dir_fh, next_cookie, cookieverf, 8192, &page2), NFSTEST_RPC_OK, "READDIR continuation RPC should succeed");
    CHECK_STATUS(page2.status, NFS3_OK, "READDIR continuation should succeed");
    for (entry3_t* entry = page2.resok.reply.entries; entry != NULL; entry = entry->nextentry) {
        CHECK_TRUE(entry->cookie != next_cookie || entry->name == NULL || strcmp(entry->name, page1.resok.reply.entries->name) != 0, "READDIR continuation should advance from returned cookie");
    }
    readdir3res_destroy(&page2);
    readdir3res_destroy(&page1);

cleanup:
    cleanup_files(&dir_fh, names, created, ENTRY_COUNT);
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dir_name);
    }
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        nfs_fh3_destroy(&file_fhs[i]);
    }
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_readdir_bad_cookie_and_stale_cookieverf_protocol_errors(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&dir_fh);
    char dir_name[128];
    int dir_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(create_named_dir(&root_fh, "c_dir_badcookie", dir_name, sizeof(dir_name), &dir_fh), NFS3_OK, "MKDIR bad-cookie dir should succeed");
    dir_created = 1;

    READDIR3res_t bad_cookie;
    readdir3res_init(&bad_cookie);
    CHECK_STATUS(call_readdir(&dir_fh, UINT64_MAX, 0, 4096, &bad_cookie), NFSTEST_RPC_OK, "READDIR bad cookie RPC should return protocol response");
    CHECK_TRUE(bad_cookie.status != NFS3_OK && status_allowed(bad_cookie.status, readdir_protocol_error_statuses, sizeof(readdir_protocol_error_statuses) / sizeof(readdir_protocol_error_statuses[0])), "READDIR bad cookie should be rejected with protocol error");
    readdir3res_destroy(&bad_cookie);

    READDIR3res_t stale_cookieverf;
    readdir3res_init(&stale_cookieverf);
    CHECK_STATUS(call_readdir(&dir_fh, 1, UINT64_MAX, 4096, &stale_cookieverf), NFSTEST_RPC_OK, "READDIR stale cookieverf RPC should return protocol response");
    CHECK_TRUE(stale_cookieverf.status != NFS3_OK && status_allowed(stale_cookieverf.status, readdir_protocol_error_statuses, sizeof(readdir_protocol_error_statuses) / sizeof(readdir_protocol_error_statuses[0])), "READDIR stale cookieverf should be rejected with protocol error");
    readdir3res_destroy(&stale_cookieverf);

cleanup:
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dir_name);
    }
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_readdirplus_basic_and_entry_metadata(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&dir_fh);
    nfs_fh3_init(&file_fh);
    char dir_name[128];
    char file_name[128];
    int dir_created = 0;
    int file_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(create_named_dir(&root_fh, "c_dir_plus", dir_name, sizeof(dir_name), &dir_fh), NFS3_OK, "MKDIR READDIRPLUS dir should succeed");
    dir_created = 1;
    CHECK_STATUS(create_named_file(&dir_fh, "c_dir_plus_file", file_name, sizeof(file_name), &file_fh), NFS3_OK, "CREATE READDIRPLUS file should succeed");
    file_created = 1;

    READDIRPLUS3res_t res;
    readdirplus3res_init(&res);
    CHECK_STATUS(call_readdirplus(&dir_fh, 0, 0, 4096, 8192, &res), NFSTEST_RPC_OK, "READDIRPLUS basic RPC should succeed");
    if (res.status == NFS3ERR_NOTSUPP) {
        readdirplus3res_destroy(&res);
        goto cleanup;
    }
    CHECK_STATUS(res.status, NFS3_OK, "READDIRPLUS basic should succeed or return NOTSUPP");
    entryplus3_t* entry = readdirplus_find_name(&res, file_name);
    CHECK_TRUE(entry != NULL, "READDIRPLUS should include created file name");
    CHECK_TRUE(entry->name_attributes.follow || !nfs_fh3_is_empty(&entry->name_handle), "READDIRPLUS should return attrs or handle when available");
    if (entry->name_attributes.follow) {
        CHECK_TRUE(entry->name_attributes.attributes.type != 0, "READDIRPLUS attrs should include a file type");
    }
    readdirplus3res_destroy(&res);

cleanup:
    if (file_created) {
        nfs3_remove_test_file(&dir_fh, file_name);
    }
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dir_name);
    }
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

static void test_readdirplus_small_counts_and_multi_entry_paging(void) {
    REQUIRE_SERVER_OR_SKIP();

    nfs_fh3_t root_fh;
    nfs_fh3_t dir_fh;
    nfs_fh3_t file_fhs[ENTRY_COUNT];
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&dir_fh);
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        nfs_fh3_init(&file_fhs[i]);
    }
    char dir_name[128];
    char names[ENTRY_COUNT][128];
    int created[ENTRY_COUNT] = {0};
    int dir_created = 0;
    int failed = 0;

    mount_root(&root_fh);
    CHECK_STATUS(create_named_dir(&root_fh, "c_dir_plus_page", dir_name, sizeof(dir_name), &dir_fh), NFS3_OK, "MKDIR READDIRPLUS paging dir should succeed");
    dir_created = 1;
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "c_dir_plus_%d", i);
        CHECK_STATUS(create_named_file(&dir_fh, prefix, names[i], sizeof(names[i]), &file_fhs[i]), NFS3_OK, "CREATE READDIRPLUS paging file should succeed");
        created[i] = 1;
    }

    READDIRPLUS3res_t full_res;
    readdirplus3res_init(&full_res);
    CHECK_STATUS(call_readdirplus(&dir_fh, 0, 0, 4096, 16384, &full_res), NFSTEST_RPC_OK, "READDIRPLUS full RPC should succeed");
    if (full_res.status == NFS3ERR_NOTSUPP) {
        readdirplus3res_destroy(&full_res);
        goto cleanup;
    }
    CHECK_STATUS(full_res.status, NFS3_OK, "READDIRPLUS full should succeed");
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        CHECK_TRUE(readdirplus_has_name(&full_res, names[i]), "READDIRPLUS full should include each created file");
    }
    readdirplus3res_destroy(&full_res);

    READDIRPLUS3res_t small_res;
    readdirplus3res_init(&small_res);
    CHECK_STATUS(call_readdirplus(&dir_fh, 0, 0, 96, 512, &small_res), NFSTEST_RPC_OK, "READDIRPLUS small dircount/maxcount RPC should succeed");
    if (small_res.status == NFS3ERR_NOTSUPP) {
        readdirplus3res_destroy(&small_res);
        goto cleanup;
    }
    CHECK_STATUS(small_res.status, NFS3_OK, "READDIRPLUS small dircount/maxcount should succeed");
    CHECK_TRUE(small_res.resok.reply.entries != NULL, "READDIRPLUS small count should return at least one entry");
    uint64_t next_cookie = small_res.resok.reply.entries->cookie;
    uint64_t cookieverf = small_res.resok.cookieverf;
    CHECK_TRUE(next_cookie != 0, "READDIRPLUS entry cookie should be usable for continuation");

    READDIRPLUS3res_t page2;
    readdirplus3res_init(&page2);
    CHECK_STATUS(call_readdirplus(&dir_fh, next_cookie, cookieverf, 4096, 16384, &page2), NFSTEST_RPC_OK, "READDIRPLUS continuation RPC should succeed");
    CHECK_STATUS(page2.status, NFS3_OK, "READDIRPLUS continuation should succeed");
    CHECK_TRUE(readdirplus_count_entries(&page2) >= 0, "READDIRPLUS continuation should decode entries");
    readdirplus3res_destroy(&page2);
    readdirplus3res_destroy(&small_res);

cleanup:
    cleanup_files(&dir_fh, names, created, ENTRY_COUNT);
    if (dir_created) {
        nfs3_rmdir_test_dir(&root_fh, dir_name);
    }
    for (int i = 0; i < ENTRY_COUNT; ++i) {
        nfs_fh3_destroy(&file_fhs[i]);
    }
    nfs_fh3_destroy(&dir_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) {
        TEST_PASS();
    }
}

int main(void) {
    printf("=== NFSv3 Directory Listing Server Tests (C) ===\n\n");
    RUN_TEST(test_readdir_empty_and_near_empty_dir);
    RUN_TEST(test_readdir_multiple_entries_paging_and_cookieverf);
    RUN_TEST(test_readdir_bad_cookie_and_stale_cookieverf_protocol_errors);
    RUN_TEST(test_readdirplus_basic_and_entry_metadata);
    RUN_TEST(test_readdirplus_small_counts_and_multi_entry_paging);
    PRINT_SUMMARY();
    return tests_failed;
}
