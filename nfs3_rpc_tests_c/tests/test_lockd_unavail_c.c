/*
 * test_lockd_unavail_c.c - 测试 lockd 未实现的 NLM 过程，预期返回 PROC_UNAVAIL
 */

#define _XOPEN_SOURCE 500
#include "test_framework.h"
#include "nfs3_server_test.h"
#include "nfstest/rpc_client.h"
#include "nfs3_c/xdr_codec.h"
#include <stdlib.h>

#define NLM_PROG         100021U
#define NLM_VERS         4U
#define RPCBPROG         100000U
#define RPCBVERS         2U
#define RPCBPROC_GETPORT 3U
#define IPPROTO_TCP_NUM  6U

#define REQUIRE_SERVER_OR_SKIP() do { \
    if (!nfs3_server_is_configured()) { \
        if (nfs3_server_require_server()) { \
            ASSERT_TRUE(0, nfs3_server_skip_message()); \
        } \
        TEST_SKIP(nfs3_server_skip_message()); \
    } \
} while(0)

static uint16_t get_nlm_port(void) {
    const char* host = getenv("NFS_TEST_SERVER");
    if (!host || host[0] == '\0') return 0;

    xdr_buf_t args;
    xdr_buf_init(&args);
    xdr_pack_uint32(&args, NLM_PROG);
    xdr_pack_uint32(&args, NLM_VERS);
    xdr_pack_uint32(&args, IPPROTO_TCP_NUM);
    xdr_pack_uint32(&args, 0);

    nfstest_rpc_client_t* client = nfstest_rpc_connect(host, 111, 3000);
    if (!client) { xdr_buf_destroy(&args); return 0; }

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = nfstest_rpc_call(client, RPCBPROG, RPCBVERS, RPCBPROC_GETPORT,
                                  xdr_buf_data(&args), xdr_buf_size(&args),
                                  &resp, &resp_len, 3000);
    nfstest_rpc_disconnect(client);
    xdr_buf_destroy(&args);

    if (status != NFSTEST_RPC_OK || resp == NULL) { free(resp); return 0; }

    xdr_buf_t body;
    xdr_buf_init_copy(&body, resp, resp_len);
    uint32_t port = 0;
    xdr_unpack_uint32(&body, &port);
    free(resp);
    xdr_buf_destroy(&body);

    return (port > 0 && port <= 65535) ? (uint16_t)port : 0;
}

static void test_nlm_unimplemented_proc(void) {
    REQUIRE_SERVER_OR_SKIP();

    uint16_t nlm_port = get_nlm_port();
    if (nlm_port == 0) {
        TEST_SKIP("NLM service not found via rpcbind");
    }

    xdr_buf_t args;
    xdr_buf_init(&args);
    uint32_t dummy = 0;
    xdr_pack_opaque(&args, (const uint8_t*)&dummy, sizeof(dummy));

    const char* host = getenv("NFS_TEST_SERVER");
    nfstest_rpc_client_t* client = nfstest_rpc_connect(host, nlm_port, 5000);
    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = NFSTEST_RPC_CONN_ERR;
    if (client) {
        status = nfstest_rpc_call(client, NLM_PROG, NLM_VERS, 99 /* 不存在的过程号 */,
                                  xdr_buf_data(&args), xdr_buf_size(&args),
                                  &resp, &resp_len, 5000);
        nfstest_rpc_disconnect(client);
    }
    free(resp);
    xdr_buf_destroy(&args);

    ASSERT_EQ_INT(status, NFSTEST_RPC_PROC_UNAVAIL,
                  "Calling unimplemented NLM proc should return PROC_UNAVAIL");
    TEST_PASS();
}

int main(void) {
    printf("=== NFS lockd PROC_UNAVAIL Test (C) ===\n\n");
    RUN_TEST(test_nlm_unimplemented_proc);
    PRINT_SUMMARY();
    return tests_failed;
}
