#include "test_framework.h"
#include "nfs3_c/xdr_codec.h"
#include "nfs3_c/nfs3_xdr.h"
#include <string.h>

#define RUN(name) do { \
    current_test_name = #name; \
    printf("  RUN   %s ... ", #name); \
    fflush(stdout); \
    name(); \
} while(0)

/* ---- XDR Codec tests ---- */

static void test_xdr_int32(void) {
    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_int32(&buf, 42);
    xdr_buf_reset_read(&buf);
    int32_t v;
    int ok = xdr_unpack_int32(&buf, &v);
    ASSERT_TRUE(ok, "unpack int32");
    ASSERT_EQ_INT(v, 42, "int32 value");
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_xdr_uint32(void) {
    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_uint32(&buf, 0x12345678U);
    xdr_buf_reset_read(&buf);
    uint32_t v;
    int ok = xdr_unpack_uint32(&buf, &v);
    ASSERT_TRUE(ok, "unpack uint32");
    ASSERT_EQ_U32(v, 0x12345678U, "uint32 value");
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_xdr_uint64(void) {
    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_uint64(&buf, 0x123456789ABCDEF0ULL);
    xdr_buf_reset_read(&buf);
    uint64_t v;
    int ok = xdr_unpack_uint64(&buf, &v);
    ASSERT_TRUE(ok, "unpack uint64");
    ASSERT_EQ_U64(v, 0x123456789ABCDEF0ULL, "uint64 value");
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_xdr_bool(void) {
    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_bool(&buf, 1);
    xdr_pack_bool(&buf, 0);
    xdr_buf_reset_read(&buf);
    int v1, v2;
    xdr_unpack_bool(&buf, &v1);
    xdr_unpack_bool(&buf, &v2);
    ASSERT_EQ_INT(v1, 1, "bool true");
    ASSERT_EQ_INT(v2, 0, "bool false");
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_xdr_string(void) {
    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_cstring(&buf, "Hello NFSv3!");
    xdr_buf_reset_read(&buf);
    char* s = NULL;
    xdr_unpack_cstring(&buf, &s);
    ASSERT_STREQ(s, "Hello NFSv3!", "string roundtrip");
    free(s);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_xdr_opaque(void) {
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_opaque(&buf, data, 4);
    xdr_buf_reset_read(&buf);
    uint8_t* d = NULL;
    uint32_t len = 0;
    xdr_unpack_opaque(&buf, &d, &len);
    ASSERT_EQ_U32(len, 4, "opaque length");
    ASSERT_MEMEQ(d, data, 4, "opaque data");
    free(d);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

/* ---- NFSv3 GETATTR tests ---- */

static void test_fattr3_roundtrip(void) {
    fattr3_t attr;
    fattr3_init(&attr);
    attr.type = NF3REG;
    attr.mode = 0755;
    attr.nlink = 3;
    attr.uid = 65534;
    attr.gid = 65534;
    attr.size = 102400;
    attr.fileid = 999;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_fattr3(&buf, &attr);
    xdr_buf_reset_read(&buf);

    fattr3_t restored;
    fattr3_init(&restored);
    xdr_unpack_fattr3(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.type, (uint32_t)attr.type, "ftype");
    ASSERT_EQ_U32(restored.mode, attr.mode, "mode");
    ASSERT_EQ_U64(restored.size, attr.size, "size");

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
    res.resok.obj_attributes.mode = 0644;
    res.resok.obj_attributes.size = 4096;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_GETATTR3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    GETATTR3res_t restored;
    memset(&restored, 0, sizeof(restored));
    xdr_unpack_GETATTR3res(&buf, &restored);

    ASSERT_EQ_U32((uint32_t)restored.status, (uint32_t)NFS3_OK, "status");
    ASSERT_EQ_INT(restored.has_resok, 1, "has_resok");
    ASSERT_EQ_U32(restored.resok.obj_attributes.mode, res.resok.obj_attributes.mode, "mode");

    xdr_buf_destroy(&buf);
    TEST_PASS();
}

static void test_getattr_res_error(void) {
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

/* ---- NFSv3 LOOKUP tests ---- */

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

    ASSERT_STREQ(restored.what_name, "testfile.txt", "name");

    lookup3args_destroy(&args);
    lookup3args_destroy(&restored);
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

    lookup3res_destroy(&res);
    lookup3res_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

/* ---- NFSv3 READ/WRITE tests ---- */

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

    ASSERT_EQ_U64(restored.offset, 1024, "offset");
    ASSERT_EQ_U32(restored.count, 4096, "count");

    nfs_fh3_destroy(&args.file);
    nfs_fh3_destroy(&restored.file);
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

    ASSERT_EQ_U64(restored.offset, 2048, "offset");
    ASSERT_EQ_U32((uint32_t)restored.stable, (uint32_t)UNSTABLE, "stable");
    ASSERT_EQ_U32(restored.data_len, 5, "data length");
    ASSERT_MEMEQ(restored.data, d, 5, "data");

    write3args_destroy(&args);
    write3args_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

/* ---- NFSv3 CREATE/REMOVE tests ---- */

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

    ASSERT_STREQ(restored.where_name, "newfile.txt", "name");
    ASSERT_EQ_U32((uint32_t)restored.how_mode, (uint32_t)GUARDED, "mode");
    ASSERT_EQ_U32(restored.how_attributes.mode, 0644, "mode val");

    create3args_destroy(&args);
    create3args_destroy(&restored);
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

    ASSERT_STREQ(restored.object_name, "file_to_remove.txt", "name");

    remove3args_destroy(&args);
    remove3args_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

/* ---- NFSv3 READDIR tests ---- */

static void test_readdir_args_roundtrip(void) {
    READDIR3args_t args;
    memset(&args, 0, sizeof(args));
    nfs_fh3_init(&args.dir);
    uint8_t fh[] = {0x01, 0x02};
    nfs_fh3_set(&args.dir, fh, 2);
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
    res.resok.cookieverf = 0xDEADBEEF;
    res.resok.reply.eof = 0;

    entry3_t* e1 = (entry3_t*)calloc(1, sizeof(entry3_t));
    e1->fileid = 1;
    entry3_set_name(e1, "file1.txt");
    e1->cookie = 1;
    res.resok.reply.entries = e1;

    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_READDIR3res(&buf, &res);
    xdr_buf_reset_read(&buf);

    READDIR3res_t restored;
    readdir3res_init(&restored);
    xdr_unpack_READDIR3res(&buf, &restored);

    ASSERT_EQ_U64(restored.resok.cookieverf, 0xDEADBEEF, "cookieverf");
    ASSERT_TRUE(restored.resok.reply.entries != NULL, "has entries");
    ASSERT_STREQ(restored.resok.reply.entries->name, "file1.txt", "entry name");

    readdir3res_destroy(&res);
    readdir3res_destroy(&restored);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

/* ---- Main ---- */

int main(void) {
    printf("========================================\n");
    printf("  NFSv3 XDR Unit Tests (C Language)\n");
    printf("========================================\n\n");

    printf("--- XDR Codec Tests ---\n");
    RUN(test_xdr_int32);
    RUN(test_xdr_uint32);
    RUN(test_xdr_uint64);
    RUN(test_xdr_bool);
    RUN(test_xdr_string);
    RUN(test_xdr_opaque);

    printf("\n--- NFSv3 GETATTR Tests ---\n");
    RUN(test_fattr3_roundtrip);
    RUN(test_getattr_args_roundtrip);
    RUN(test_getattr_res_ok_roundtrip);
    RUN(test_getattr_res_error);

    printf("\n--- NFSv3 LOOKUP Tests ---\n");
    RUN(test_lookup_args_roundtrip);
    RUN(test_lookup_res_noent);

    printf("\n--- NFSv3 READ/WRITE Tests ---\n");
    RUN(test_read_args_roundtrip);
    RUN(test_write_args_roundtrip);

    printf("\n--- NFSv3 CREATE/REMOVE Tests ---\n");
    RUN(test_create_args_roundtrip);
    RUN(test_remove_args_roundtrip);

    printf("\n--- NFSv3 READDIR Tests ---\n");
    RUN(test_readdir_args_roundtrip);
    RUN(test_readdir_res_ok_roundtrip);

    PRINT_SUMMARY();
    return tests_failed;
}
