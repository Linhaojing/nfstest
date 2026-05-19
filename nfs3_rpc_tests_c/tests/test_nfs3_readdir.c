#include "test_framework.h"
#include "nfs3_c/nfs3_xdr.h"

static void test_readdir_args_roundtrip(void) {
    READDIR3args_t args;
    memset(&args, 0, sizeof(args));
    nfs_fh3_init(&args.dir);
    uint8_t fh[] = {0x01, 0x02};
    nfs_fh3_set(&args.dir, fh, 2);
    args.cookie = 0;
    args.cookieverf = 0;
    args.count = 4096;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_READDIR3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    READDIR3args_t restored;
    memset(&restored, 0, sizeof(restored));
    nfs_fh3_init(&restored.dir);
    xdr_unpack_READDIR3args(&buf, &restored);

    ASSERT_EQ_U32(restored.dir.len, 2, "fh length");
    ASSERT_EQ_U64(restored.cookie, 0, "cookie");
    ASSERT_EQ_U32(restored.count, 4096, "count");

    nfs_fh3_destroy(&args.dir);
    nfs_fh3_destroy(&restored.dir);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_readdir_res_ok_roundtrip(void) {
    READDIR3res_t res;
    readdir3res_init(&res);
    res.status = NFS3_OK;
    res.has_resok = 1;
    res.resok.dir_attributes.follow = 1;
    res.resok.dir_attributes.attributes.type = NF3DIR;
    res.resok.cookieverf = 0xDEADBEEF;
    res.resok.reply.eof = 0;

    entry3_t* e1 = (entry3_t*)calloc(1, sizeof(entry3_t));
    e1->fileid = 1;
    entry3_set_name(e1, "file1.txt");
    e1->cookie = 1;

    entry3_t* e2 = (entry3_t*)calloc(1, sizeof(entry3_t));
    e2->fileid = 2;
    entry3_set_name(e2, "file2.txt");
    e2->cookie = 2;
    e1->nextentry = e2;

    res.resok.reply.entries = e1;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_READDIR3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    READDIR3res_t restored;
    readdir3res_init(&restored);
    xdr_unpack_READDIR3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3_OK, "status");
    ASSERT_EQ_U64(restored.resok.cookieverf, 0xDEADBEEF, "cookieverf");
    ASSERT_TRUE(restored.resok.reply.entries != NULL, "has entries");
    ASSERT_STREQ(restored.resok.reply.entries->name, "file1.txt", "first entry");
    ASSERT_TRUE(restored.resok.reply.entries->nextentry != NULL, "has next");
    ASSERT_STREQ(restored.resok.reply.entries->nextentry->name, "file2.txt", "second entry");

    readdir3res_destroy(&res);
    readdir3res_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_fsstat_args_roundtrip(void) {
    FSSTAT3args_t args;
    memset(&args, 0, sizeof(args));
    nfs_fh3_init(&args.fsroot);
    uint8_t fh[] = {0x01};
    nfs_fh3_set(&args.fsroot, fh, 1);

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_FSSTAT3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    FSSTAT3args_t restored;
    memset(&restored, 0, sizeof(restored));
    nfs_fh3_init(&restored.fsroot);
    xdr_unpack_FSSTAT3args(&buf, &restored);

    ASSERT_EQ_U32(restored.fsroot.len, 1, "fh length");

    nfs_fh3_destroy(&args.fsroot);
    nfs_fh3_destroy(&restored.fsroot);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_commit_args_roundtrip(void) {
    COMMIT3args_t args;
    memset(&args, 0, sizeof(args));
    nfs_fh3_init(&args.file);
    uint8_t fh[] = {0x01, 0x02};
    nfs_fh3_set(&args.file, fh, 2);
    args.offset = 4096;
    args.count = 1024;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_COMMIT3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    COMMIT3args_t restored;
    memset(&restored, 0, sizeof(restored));
    nfs_fh3_init(&restored.file);
    xdr_unpack_COMMIT3args(&buf, &restored);

    ASSERT_EQ_U32(restored.file.len, 2, "fh length");
    ASSERT_EQ_U64(restored.offset, 4096, "offset");
    ASSERT_EQ_U32(restored.count, 1024, "count");

    nfs_fh3_destroy(&args.file);
    nfs_fh3_destroy(&restored.file);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

int main(void) {
    printf("=== NFSv3 READDIR/FSSTAT/COMMIT Tests (C) ===\n\n");
    RUN_TEST(test_readdir_args_roundtrip);
    RUN_TEST(test_readdir_res_ok_roundtrip);
    RUN_TEST(test_fsstat_args_roundtrip);
    RUN_TEST(test_commit_args_roundtrip);
    PRINT_SUMMARY();
    return tests_failed;
}
