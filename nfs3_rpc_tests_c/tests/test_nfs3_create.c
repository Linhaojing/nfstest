#include "test_framework.h"
#include "nfs3_c/nfs3_xdr.h"

static void test_create_args_roundtrip(void) {
    CREATE3args_t args;
    create3args_init(&args);
    uint8_t fh[] = {0x01, 0x02, 0x03};
    nfs_fh3_set(&args.where_dir, fh, 3);
    create3args_set_name(&args, "newfile.txt");
    args.how_mode = GUARDED;
    args.how_attributes.mode_set = 1;
    args.how_attributes.mode = 0644;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_CREATE3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    CREATE3args_t restored;
    create3args_init(&restored);
    xdr_unpack_CREATE3args(&buf, &restored);

    ASSERT_EQ_U32(restored.where_dir.len, 3, "fh length");
    ASSERT_STREQ(restored.where_name, "newfile.txt", "name");
    ASSERT_EQ_U32((uint32_t)restored.how_mode, (uint32_t)GUARDED, "mode");
    ASSERT_EQ_INT(restored.how_attributes.mode_set, 1, "mode_set");
    ASSERT_EQ_U32(restored.how_attributes.mode, 0644, "mode val");

    create3args_destroy(&args);
    create3args_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_create_res_ok_roundtrip(void) {
    CREATE3res_t res;
    create3res_init(&res);
    res.status = NFS3_OK;
    res.has_resok = 1;
    res.resok.obj_attributes.follow = 1;
    res.resok.obj_attributes.attributes.type = NF3REG;
    uint8_t fh[] = {0xAA, 0xBB};
    nfs_fh3_set(&res.resok.object, fh, 2);

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_CREATE3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    CREATE3res_t restored;
    create3res_init(&restored);
    xdr_unpack_CREATE3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3_OK, "status");
    ASSERT_EQ_U32(restored.resok.object.len, 2, "object fh len");

    create3res_destroy(&res);
    create3res_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_remove_args_roundtrip(void) {
    REMOVE3args_t args;
    remove3args_init(&args);
    uint8_t fh[] = {0x01, 0x02};
    nfs_fh3_set(&args.object_dir, fh, 2);
    remove3args_set_name(&args, "file_to_remove.txt");

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_REMOVE3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    REMOVE3args_t restored;
    remove3args_init(&restored);
    xdr_unpack_REMOVE3args(&buf, &restored);

    ASSERT_EQ_U32(restored.object_dir.len, 2, "fh length");
    ASSERT_STREQ(restored.object_name, "file_to_remove.txt", "name");

    remove3args_destroy(&args);
    remove3args_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_remove_res_error(void) {
    REMOVE3res_t res;
    memset(&res, 0, sizeof(res));
    res.status = NFS3ERR_NOENT;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_REMOVE3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    REMOVE3res_t restored;
    memset(&restored, 0, sizeof(restored));
    xdr_unpack_REMOVE3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3ERR_NOENT, "status");

    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_access_args_roundtrip(void) {
    ACCESS3args_t args;
    memset(&args, 0, sizeof(args));
    nfs_fh3_init(&args.object);
    uint8_t fh[] = {0x01};
    nfs_fh3_set(&args.object, fh, 1);
    args.access = NFS3_ACCESS_READ | NFS3_ACCESS_LOOKUP;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_ACCESS3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    ACCESS3args_t restored;
    memset(&restored, 0, sizeof(restored));
    nfs_fh3_init(&restored.object);
    xdr_unpack_ACCESS3args(&buf, &restored);

    ASSERT_EQ_U32(restored.object.len, 1, "fh length");
    ASSERT_EQ_U32(restored.access, args.access, "access mask");

    nfs_fh3_destroy(&args.object);
    nfs_fh3_destroy(&restored.object);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

int main(void) {
    printf("=== NFSv3 CREATE/REMOVE/ACCESS Tests (C) ===\n\n");
    RUN_TEST(test_create_args_roundtrip);
    RUN_TEST(test_create_res_ok_roundtrip);
    RUN_TEST(test_remove_args_roundtrip);
    RUN_TEST(test_remove_res_error);
    RUN_TEST(test_access_args_roundtrip);
    PRINT_SUMMARY();
    return tests_failed;
}
