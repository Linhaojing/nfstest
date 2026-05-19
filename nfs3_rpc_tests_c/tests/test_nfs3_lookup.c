#include "test_framework.h"
#include "nfs3_c/nfs3_xdr.h"

static void test_lookup_args_roundtrip(void) {
    LOOKUP3args_t args;
    lookup3args_init(&args);
    uint8_t fh[] = {0x01, 0x02, 0x03};
    nfs_fh3_set(&args.what_dir, fh, 3);
    lookup3args_set_name(&args, "testfile.txt");

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_LOOKUP3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    LOOKUP3args_t restored;
    lookup3args_init(&restored);
    xdr_unpack_LOOKUP3args(&buf, &restored);

    ASSERT_EQ_U32(restored.what_dir.len, 3, "fh length");
    ASSERT_STREQ(restored.what_name, "testfile.txt", "name");

    lookup3args_destroy(&args);
    lookup3args_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_lookup_args_empty_name(void) {
    LOOKUP3args_t args;
    lookup3args_init(&args);
    uint8_t fh[] = {0xFF, 0xFE, 0xFD};
    nfs_fh3_set(&args.what_dir, fh, 3);
    lookup3args_set_name(&args, "");

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_LOOKUP3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    LOOKUP3args_t restored;
    lookup3args_init(&restored);
    xdr_unpack_LOOKUP3args(&buf, &restored);

    ASSERT_STREQ(restored.what_name, "", "empty name");

    lookup3args_destroy(&args);
    lookup3args_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_lookup_args_long_path(void) {
    LOOKUP3args_t args;
    lookup3args_init(&args);
    uint8_t fh[] = {0x01};
    nfs_fh3_set(&args.what_dir, fh, 1);
    lookup3args_set_name(&args, "this/is/a/very/deep/nested/path/file.txt");

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_LOOKUP3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    LOOKUP3args_t restored;
    lookup3args_init(&restored);
    xdr_unpack_LOOKUP3args(&buf, &restored);

    ASSERT_STREQ(restored.what_name, "this/is/a/very/deep/nested/path/file.txt", "long name");

    lookup3args_destroy(&args);
    lookup3args_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_lookup_res_ok_roundtrip(void) {
    LOOKUP3res_t res;
    lookup3res_init(&res);
    res.status = NFS3_OK;
    res.has_resok = 1;
    uint8_t fh[] = {0xAA, 0xBB, 0xCC};
    nfs_fh3_set(&res.resok.object, fh, 3);
    res.resok.obj_attributes.follow = 1;
    res.resok.obj_attributes.attributes.type = NF3REG;
    res.resok.obj_attributes.attributes.size = 8192;
    res.resok.dir_attributes.follow = 1;
    res.resok.dir_attributes.attributes.type = NF3DIR;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_LOOKUP3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    LOOKUP3res_t restored;
    lookup3res_init(&restored);
    xdr_unpack_LOOKUP3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3_OK, "status");
    ASSERT_EQ_INT(restored.has_resok, 1, "has_resok");
    ASSERT_EQ_U32(restored.resok.object.len, 3, "object fh len");

    lookup3res_destroy(&res);
    lookup3res_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_lookup_res_noent(void) {
    LOOKUP3res_t res;
    lookup3res_init(&res);
    res.status = NFS3ERR_NOENT;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_LOOKUP3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    LOOKUP3res_t restored;
    lookup3res_init(&restored);
    xdr_unpack_LOOKUP3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3ERR_NOENT, "status");
    ASSERT_EQ_INT(restored.has_resok, 0, "no resok");

    lookup3res_destroy(&res);
    lookup3res_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

int main(void) {
    printf("=== NFSv3 LOOKUP Tests (C) ===\n\n");
    RUN_TEST(test_lookup_args_roundtrip);
    RUN_TEST(test_lookup_args_empty_name);
    RUN_TEST(test_lookup_args_long_path);
    RUN_TEST(test_lookup_res_ok_roundtrip);
    RUN_TEST(test_lookup_res_noent);
    PRINT_SUMMARY();
    return tests_failed;
}
