#include "test_framework.h"
#include "nfs3_c/nfs3_xdr.h"
#include <string.h>

static void test_read_args_roundtrip(void) {
    READ3args_t args;
    memset(&args, 0, sizeof(args));
    nfs_fh3_init(&args.file);
    uint8_t fh[] = {0x01, 0x02, 0x03};
    nfs_fh3_set(&args.file, fh, 3);
    args.offset = 1024;
    args.count = 4096;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_READ3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    READ3args_t restored;
    memset(&restored, 0, sizeof(restored));
    nfs_fh3_init(&restored.file);
    xdr_unpack_READ3args(&buf, &restored);

    ASSERT_EQ_U32(restored.file.len, 3, "fh length");
    ASSERT_EQ_U64(restored.offset, 1024, "offset");
    ASSERT_EQ_U32(restored.count, 4096, "count");

    nfs_fh3_destroy(&args.file);
    nfs_fh3_destroy(&restored.file);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_read_args_max_count(void) {
    READ3args_t args;
    memset(&args, 0, sizeof(args));
    nfs_fh3_init(&args.file);
    uint8_t fh[] = {0x01};
    nfs_fh3_set(&args.file, fh, 1);
    args.offset = 0;
    args.count = 1048576;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_READ3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    READ3args_t restored;
    memset(&restored, 0, sizeof(restored));
    nfs_fh3_init(&restored.file);
    xdr_unpack_READ3args(&buf, &restored);

    ASSERT_EQ_U32(restored.count, 1048576, "max count");

    nfs_fh3_destroy(&args.file);
    nfs_fh3_destroy(&restored.file);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_read_res_ok_roundtrip(void) {
    READ3res_t res;
    read3res_init(&res);
    res.status = NFS3_OK;
    res.has_resok = 1;
    res.resok.file_attributes.follow = 1;
    res.resok.file_attributes.attributes.type = NF3REG;
    res.resok.file_attributes.attributes.size = 8192;
    res.resok.count = 256;
    res.resok.eof = 0;
    uint8_t d[] = {0xDE, 0xAD, 0xBE, 0xEF};
    res.resok.data = (uint8_t*)malloc(4);
    memcpy(res.resok.data, d, 4);
    res.resok.data_len = 4;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_READ3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    READ3res_t restored;
    read3res_init(&restored);
    xdr_unpack_READ3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3_OK, "status");
    ASSERT_EQ_U64(restored.resok.count, 256, "count");
    ASSERT_EQ_INT(restored.resok.eof, 0, "eof");
    ASSERT_EQ_U32(restored.resok.data_len, 4, "data length");
    ASSERT_MEMEQ(restored.resok.data, d, 4, "data");

    read3res_destroy(&res);
    read3res_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_write_args_roundtrip(void) {
    WRITE3args_t args;
    write3args_init(&args);
    uint8_t fh[] = {0x01, 0x02, 0x03};
    nfs_fh3_set(&args.file, fh, 3);
    args.offset = 2048;
    args.stable = UNSTABLE;
    uint8_t d[] = {'h', 'e', 'l', 'l', 'o'};
    args.data = (uint8_t*)malloc(5);
    memcpy(args.data, d, 5);
    args.data_len = 5;
    args.count = 5;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_WRITE3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    WRITE3args_t restored;
    write3args_init(&restored);
    xdr_unpack_WRITE3args(&buf, &restored);

    ASSERT_EQ_U32(restored.file.len, 3, "fh length");
    ASSERT_EQ_U64(restored.offset, 2048, "offset");
    ASSERT_EQ_U32((uint32_t)restored.stable, (uint32_t)UNSTABLE, "stable");
    ASSERT_EQ_U32(restored.data_len, 5, "data length");
    ASSERT_MEMEQ(restored.data, d, 5, "data");

    write3args_destroy(&args);
    write3args_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_write_res_ok_roundtrip(void) {
    WRITE3res_t res;
    memset(&res, 0, sizeof(res));
    res.status = NFS3_OK;
    res.has_resok = 1;
    res.resok.file_attributes.follow = 1;
    res.resok.file_attributes.attributes.size = 8192;
    res.resok.count = 1024;
    res.resok.committed = FILE_SYNC;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_WRITE3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    WRITE3res_t restored;
    memset(&restored, 0, sizeof(restored));
    xdr_unpack_WRITE3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3_OK, "status");
    ASSERT_EQ_U32(restored.resok.count, 1024, "count");
    ASSERT_EQ_U32((uint32_t)restored.resok.committed, (uint32_t)FILE_SYNC, "committed");

    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_write_res_error_roundtrip(void) {
    WRITE3res_t res;
    memset(&res, 0, sizeof(res));
    res.status = NFS3ERR_ROFS;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_WRITE3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    WRITE3res_t restored;
    memset(&restored, 0, sizeof(restored));
    xdr_unpack_WRITE3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3ERR_ROFS, "status");

    xdr_buf_destroy(&buf);
    TEST_PASS();
}

int main(void) {
    printf("=== NFSv3 READ/WRITE Tests (C) ===\n\n");
    RUN_TEST(test_read_args_roundtrip);
    RUN_TEST(test_read_args_max_count);
    RUN_TEST(test_read_res_ok_roundtrip);
    RUN_TEST(test_write_args_roundtrip);
    RUN_TEST(test_write_res_ok_roundtrip);
    RUN_TEST(test_write_res_error_roundtrip);
    PRINT_SUMMARY();
    return tests_failed;
}
