#include "test_framework.h"
#include "nfs3_c/xdr_codec.h"
#include <stdint.h>

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

static void test_xdr_empty_string(void) {
    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_cstring(&buf, "");
    xdr_buf_reset_read(&buf);
    char* s = NULL;
    xdr_unpack_cstring(&buf, &s);
    ASSERT_STREQ(s, "", "empty string");
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

static void test_xdr_multiple(void) {
    xdr_buf_t buf;
    xdr_buf_init(&buf);
    xdr_pack_int32(&buf, -1);
    xdr_pack_uint32(&buf, 0xDEADBEEF);
    xdr_pack_uint64(&buf, 0xCAFEBABE);
    xdr_pack_cstring(&buf, "test");
    xdr_pack_bool(&buf, 1);
    xdr_buf_reset_read(&buf);
    int32_t i32;
    uint32_t u32;
    uint64_t u64;
    char* s;
    int b;
    xdr_unpack_int32(&buf, &i32);
    xdr_unpack_uint32(&buf, &u32);
    xdr_unpack_uint64(&buf, &u64);
    xdr_unpack_cstring(&buf, &s);
    xdr_unpack_bool(&buf, &b);
    ASSERT_EQ_INT(i32, -1, "int32");
    ASSERT_EQ_U32(u32, 0xDEADBEEF, "uint32");
    ASSERT_EQ_U64(u64, 0xCAFEBABE, "uint64");
    ASSERT_STREQ(s, "test", "string");
    ASSERT_EQ_INT(b, 1, "bool");
    free(s);
    xdr_buf_destroy(&buf);
    TEST_PASS();
}

int main(void) {
    printf("=== XDR Codec Unit Tests (C) ===\n\n");
    RUN_TEST(test_xdr_int32);
    RUN_TEST(test_xdr_uint32);
    RUN_TEST(test_xdr_uint64);
    RUN_TEST(test_xdr_bool);
    RUN_TEST(test_xdr_string);
    RUN_TEST(test_xdr_empty_string);
    RUN_TEST(test_xdr_opaque);
    RUN_TEST(test_xdr_multiple);
    PRINT_SUMMARY();
    return tests_failed;
}
