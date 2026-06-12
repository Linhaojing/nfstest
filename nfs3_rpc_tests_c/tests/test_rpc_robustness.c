#include "test_framework.h"
#include "nfstest/rpc_client.h"
#include "nfs3_c/nfs3_constants.h"
#include "nfs3_server_test.h"
#include <string.h>
#include <stdlib.h>

/* ---- big-endian uint32 helpers ---- */

static void put32(uint8_t* buf, size_t offset, uint32_t val) {
    buf[offset + 0] = (uint8_t)(val >> 24);
    buf[offset + 1] = (uint8_t)(val >> 16);
    buf[offset + 2] = (uint8_t)(val >> 8);
    buf[offset + 3] = (uint8_t)(val);
}

/* build a minimal legal ONC RPC call (AUTH_SYS + AUTH_NONE, no args body) */
static void build_call_header(uint8_t* buf, size_t buf_size, size_t* out_len,
                              uint32_t xid, uint32_t prog, uint32_t vers, uint32_t proc) {
    (void)buf_size;
    *out_len = 0;
    put32(buf, *out_len, xid);            *out_len += 4; /* xid */
    put32(buf, *out_len, 0);             *out_len += 4; /* msg_type CALL */
    put32(buf, *out_len, 2);             *out_len += 4; /* rpcvers */
    put32(buf, *out_len, prog);          *out_len += 4;
    put32(buf, *out_len, vers);          *out_len += 4;
    put32(buf, *out_len, proc);          *out_len += 4;
    /* AUTH_SYS cred: flavor=1, len=24, stamp=0, machinename="localhost" (padded 16 bytes), uid=0, gid=0, gids=0 */
    put32(buf, *out_len, 1);             *out_len += 4;
    put32(buf, *out_len, 24);            *out_len += 4;
    put32(buf, *out_len, 0);             *out_len += 4; /* stamp */
    put32(buf, *out_len, 9);             *out_len += 4; /* hostname len */
    memcpy(buf + *out_len, "localhost", 9); *out_len += 9;
    buf[(*out_len)++] = 0; /* padding to 4-byte boundary */
    buf[(*out_len)++] = 0;
    buf[(*out_len)++] = 0;
    put32(buf, *out_len, 0);             *out_len += 4; /* uid */
    put32(buf, *out_len, 0);             *out_len += 4; /* gid */
    put32(buf, *out_len, 0);             *out_len += 4; /* gids count */
    /* AUTH_NONE verf */
    put32(buf, *out_len, 0);             *out_len += 4;
    put32(buf, *out_len, 0);             *out_len += 4;
}

#define CALL_BUF_SIZE 512

/* inline skip check - TEST_SKIP returns from the enclosing function */
#define REQUIRE_SERVER() do { \
    if (!nfs3_server_is_configured()) { \
        TEST_SKIP(nfs3_server_skip_message()); \
    } \
} while(0)

/*
 * Send a raw ONC RPC call and verify server doesn't crash/hang.
 * Returns: 0 = success (server responded or dropped), -1 = connection error
 */
static int send_raw_and_verify(const uint8_t* call_data, size_t call_len) {
    const char* host = getenv("NFS_TEST_SERVER");
    const char* port_env = getenv("NFS_TEST_PORT");
    uint16_t port = 2049;
    if (port_env && port_env[0] != '\0') {
        unsigned long p = strtoul(port_env, NULL, 10);
        if (p > 0 && p <= 65535) port = (uint16_t)p;
    }

    nfstest_rpc_client_t* client = nfstest_rpc_connect(host, port, 3000);
    if (!client) {
        return -1;
    }

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfstest_rpc_raw_call(client, call_data, call_len, &resp, &resp_len, 3000);

    nfstest_rpc_disconnect(client);
    free(resp);

    /* server should either respond or drop connection - never hang */
    if (status == NFSTEST_RPC_OK || status == NFSTEST_RPC_RECV_ERR ||
        status == NFSTEST_RPC_CONN_ERR || status == NFSTEST_RPC_SEND_ERR) {
        return 0;
    }

    /* unexpected status */
    return -1;
}

static const uint32_t valid_prog  = NFS_PROGRAM;
static const uint32_t valid_vers  = NFS_V3;
static const uint32_t valid_proc  = NFSPROC3_NULL;

/* ==================== AUTH field robustness tests ==================== */

static void test_auth_invalid_flavor(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 24, 999); /* invalid cred flavor */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "invalid auth flavor");
    TEST_PASS();
}

static void test_auth_sys_cred_len_zero_with_body(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 28, 0); /* cred len=0 but body follows */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "cred len 0 with body");
    TEST_PASS();
}

static void test_auth_sys_cred_len_too_large(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 28, 0xFFFF); /* cred len oversized beyond packet */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "oversized cred len");
    TEST_PASS();
}

static void test_auth_sys_cred_len_truncated(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 28, 4); /* cred len 4, leaving AUTH_SYS body unaccounted */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "truncated cred");
    TEST_PASS();
}

static void test_auth_sys_machinename_len_overflow(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 36, 0xFFFF); /* machinename len overflow */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "machinename overflow");
    TEST_PASS();
}

static void test_auth_sys_gid_count_mismatch(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 56, 100); /* gids count = 100 but no gids in packet */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "gid count mismatch");
    TEST_PASS();
}

static void test_auth_sys_stamp_extreme(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 32, 0xFFFFFFFF); /* stamp = UINT32_MAX */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "extreme stamp");
    TEST_PASS();
}

static void test_auth_verf_invalid_flavor(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 56, 999); /* invalid verf flavor */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "invalid verf flavor");
    TEST_PASS();
}

static void test_auth_verf_len_mismatch(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 60, 100); /* verf len 100 but no body */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "verf len mismatch");
    TEST_PASS();
}

/* ==================== RPC header robustness tests ==================== */

static void test_rpc_xid_zero(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 0, valid_prog, valid_vers, valid_proc);
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "xid zero");
    TEST_PASS();
}

static void test_rpc_msg_type_invalid(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 4, 7); /* msg_type = 7 (invalid) */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "invalid msg_type");
    TEST_PASS();
}

static void test_rpc_rpcvers_invalid(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, valid_proc);
    put32(buf, 8, 9); /* rpcvers = 9 (invalid) */
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "invalid rpcvers");
    TEST_PASS();
}

static void test_rpc_prog_zero(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, 0, valid_vers, valid_proc);
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "prog zero");
    TEST_PASS();
}

static void test_rpc_proc_out_of_range(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t len;
    build_call_header(buf, sizeof(buf), &len, 1, valid_prog, valid_vers, 99);
    ASSERT_EQ_INT(send_raw_and_verify(buf, len), 0, "proc out of range");
    TEST_PASS();
}

/* ==================== Packet length robustness tests ==================== */

static void test_packet_too_short_4bytes(void) {
    REQUIRE_SERVER();
    uint8_t buf[4];
    put32(buf, 0, 1);
    ASSERT_EQ_INT(send_raw_and_verify(buf, 4), 0, "4-byte packet");
    TEST_PASS();
}

static void test_packet_truncated_in_cred_body(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t full_len;
    build_call_header(buf, sizeof(buf), &full_len, 1, valid_prog, valid_vers, valid_proc);
    ASSERT_EQ_INT(send_raw_and_verify(buf, 30), 0, "cred body truncated");
    TEST_PASS();
}

static void test_packet_truncated_in_rpc_header(void) {
    REQUIRE_SERVER();
    uint8_t buf[CALL_BUF_SIZE];
    size_t full_len;
    build_call_header(buf, sizeof(buf), &full_len, 1, valid_prog, valid_vers, valid_proc);
    ASSERT_EQ_INT(send_raw_and_verify(buf, 14), 0, "rpc header truncated");
    TEST_PASS();
}

static void test_packet_empty(void) {
    REQUIRE_SERVER();
    uint8_t dummy = 0;
    ASSERT_EQ_INT(send_raw_and_verify(&dummy, 0), 0, "empty packet");
    TEST_PASS();
}

int main(void) {
    printf("=== RPC Robustness / Security Tests (C) ===\n\n");

    RUN_TEST(test_auth_invalid_flavor);
    RUN_TEST(test_auth_sys_cred_len_zero_with_body);
    RUN_TEST(test_auth_sys_cred_len_too_large);
    RUN_TEST(test_auth_sys_cred_len_truncated);
    RUN_TEST(test_auth_sys_machinename_len_overflow);
    RUN_TEST(test_auth_sys_gid_count_mismatch);
    RUN_TEST(test_auth_sys_stamp_extreme);
    RUN_TEST(test_auth_verf_invalid_flavor);
    RUN_TEST(test_auth_verf_len_mismatch);

    RUN_TEST(test_rpc_xid_zero);
    RUN_TEST(test_rpc_msg_type_invalid);
    RUN_TEST(test_rpc_rpcvers_invalid);
    RUN_TEST(test_rpc_prog_zero);
    RUN_TEST(test_rpc_proc_out_of_range);

    RUN_TEST(test_packet_too_short_4bytes);
    RUN_TEST(test_packet_truncated_in_cred_body);
    RUN_TEST(test_packet_truncated_in_rpc_header);
    RUN_TEST(test_packet_empty);

    PRINT_SUMMARY();
    return tests_failed;
}
