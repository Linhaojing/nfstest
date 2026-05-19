#include "test_framework.h"
#include "nfs3_c/nfs3_xdr.h"

static void test_fattr3_roundtrip(void) {
    fattr3_t attr;
    fattr3_init(&attr);
    attr.type = NF3REG;
    attr.mode = 0755;
    attr.nlink = 3;
    attr.uid = 65534;
    attr.gid = 65534;
    attr.size = 102400;
    attr.used = 102400;
    attr.fsid = 99;
    attr.fileid = 999;
    attr.atime.seconds = 1700000000;
    attr.atime.nseconds = 500000000;
    attr.mtime.seconds = 1700000001;
    attr.ctime.seconds = 1700000002;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_fattr3(&buf, &attr);
    xdr_buf_reset_read(&buf);

    fattr3_t restored;
    fattr3_init(&restored);
    xdr_unpack_fattr3(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.type, (uint32_t)attr.type, "ftype");
    ASSERT_EQ_U32(restored.mode, attr.mode, "mode");
    ASSERT_EQ_U32(restored.nlink, attr.nlink, "nlink");
    ASSERT_EQ_U32(restored.uid, attr.uid, "uid");
    ASSERT_EQ_U32(restored.gid, attr.gid, "gid");
    ASSERT_EQ_U64(restored.size, attr.size, "size");
    ASSERT_EQ_U64(restored.fileid, attr.fileid, "fileid");

    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_getattr_args_roundtrip(void) {
    GETATTR3args_t args;
    memset(&args, 0, sizeof(args));
    nfs_fh3_init(&args.object);
    uint8_t fh_data[] = {0x01, 0x02, 0x03, 0x04};
    nfs_fh3_set(&args.object, fh_data, 4);

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_GETATTR3args(&buf, &args);
    xdr_buf_reset_read(&buf);

    GETATTR3args_t restored;
    memset(&restored, 0, sizeof(restored));
    nfs_fh3_init(&restored.object);
    xdr_unpack_GETATTR3args(&buf, &restored);

    ASSERT_EQ_U32(restored.object.len, args.object.len, "fh length");
    ASSERT_MEMEQ(restored.object.data, args.object.data, 4, "fh data");

    nfs_fh3_destroy(&args.object);
    nfs_fh3_destroy(&restored.object);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_getattr_res_ok_roundtrip(void) {
    GETATTR3res_t res;
    memset(&res, 0, sizeof(res));
    res.status = NFS3_OK;
    res.has_resok = 1;
    res.resok.obj_attributes.type = NF3REG;
    res.resok.obj_attributes.mode = 0644;
    res.resok.obj_attributes.size = 4096;
    res.resok.obj_attributes.fileid = 12345;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_GETATTR3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    GETATTR3res_t restored;
    memset(&restored, 0, sizeof(restored));
    xdr_unpack_GETATTR3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)res.status, "status");
    ASSERT_EQ_INT(restored.has_resok, 1, "has_resok");
    ASSERT_EQ_U32(restored.resok.obj_attributes.mode, res.resok.obj_attributes.mode, "mode");

    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_getattr_res_error_roundtrip(void) {
    GETATTR3res_t res;
    memset(&res, 0, sizeof(res));
    res.status = NFS3ERR_NOENT;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_GETATTR3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    GETATTR3res_t restored;
    memset(&restored, 0, sizeof(restored));
    xdr_unpack_GETATTR3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3ERR_NOENT, "status");
    ASSERT_EQ_INT(restored.has_resok, 0, "no resok");

    xdr_buf_destroy(&buf);
    TEST_PASS();
}

int main(void) {
    printf("=== NFSv3 GETATTR Tests (C) ===\n\n");
    RUN_TEST(test_fattr3_roundtrip);
    RUN_TEST(test_getattr_args_roundtrip);
    RUN_TEST(test_getattr_res_ok_roundtrip);
    RUN_TEST(test_getattr_res_error_roundtrip);
    PRINT_SUMMARY();
    return tests_failed;
}
