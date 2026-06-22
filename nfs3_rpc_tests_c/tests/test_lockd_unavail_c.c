/*
 * test_lockd_unavail_c.c - 测试 lockd 未实现的 NLM 过程
 */

#define _XOPEN_SOURCE 500
#include "test_framework.h"
#include "nfs3_server_test.h"
#include "nfstest/rpc_client.h"
#include "nfs3_c/xdr_codec.h"
#include <stdlib.h>
#include <unistd.h>

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

/* NLM_PROG 过程号 */
#define NLMPROC4_TEST   1U
#define NLMPROC4_LOCK   2U
#define NLMPROC4_CANCEL 3U
#define NLMPROC4_UNLOCK 4U
#define NLM4_FAILED      9U

/*
 * 构造 NLM4 alock 结构（TEST/LOCK/CANCEL/UNLOCK 共用）：
 *   caller_name(string) + fh(netobj) + oh(netobj) + svid(u32) + l_offset(u64) + l_len(u64)
 * 使用全零 fh，服务器未实现时会在解析前直接拒绝。
 */
static void pack_nlm4_args_with_lock(xdr_buf_t* buf, uint32_t proc) {
    /* cookie: netobj */
    uint32_t cookie_val = proc;
    xdr_pack_opaque(buf, (const uint8_t*)&cookie_val, sizeof(cookie_val));

    if (proc == NLMPROC4_LOCK || proc == NLMPROC4_CANCEL) {
        xdr_pack_bool(buf, 0);  /* block */
    }
    if (proc == NLMPROC4_TEST || proc == NLMPROC4_LOCK || proc == NLMPROC4_CANCEL) {
        xdr_pack_bool(buf, 1);  /* exclusive */
    }

    /* alock: caller_name */
    char hostname[64];
    if (gethostname(hostname, sizeof(hostname)) != 0)
        snprintf(hostname, sizeof(hostname), "nfstest");
    xdr_pack_cstring(buf, hostname);
    /* fh: 32 bytes of zero */
    uint8_t fh_zero[32] = {0};
    xdr_pack_opaque(buf, fh_zero, sizeof(fh_zero));
    /* oh: pid bytes */
    uint32_t pid = (uint32_t)getpid();
    xdr_pack_opaque(buf, (const uint8_t*)&pid, sizeof(pid));
    /* svid */
    xdr_pack_uint32(buf, pid);
    /* l_offset, l_len */
    xdr_pack_uint64(buf, 0);
    xdr_pack_uint64(buf, 0);

    if (proc == NLMPROC4_LOCK) {
        xdr_pack_bool(buf, 0);    /* reclaim */
        xdr_pack_uint32(buf, 1);  /* state */
    }
}


static int unpack_nlm4_status(uint8_t* resp, size_t resp_len, uint32_t* nlm_status) {
    xdr_buf_t body;
    xdr_buf_init_copy(&body, resp, resp_len);
    uint8_t* cookie_data = NULL;
    uint32_t cookie_len = 0;
    int ok = xdr_unpack_opaque(&body, &cookie_data, &cookie_len) &&
             xdr_unpack_uint32(&body, nlm_status);
    free(cookie_data);
    xdr_buf_destroy(&body);
    return ok;
}

static void test_nlm_null_proc(void) {
    REQUIRE_SERVER_OR_SKIP();

    uint16_t nlm_port = get_nlm_port();
    if (nlm_port == 0) {
        TEST_SKIP("NLM service not found via rpcbind");
    }

    const char* host = getenv("NFS_TEST_SERVER");
    nfstest_rpc_client_t* client = nfstest_rpc_connect(host, nlm_port, 5000);
    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int status = NFSTEST_RPC_CONN_ERR;
    if (client) {
        xdr_buf_t args;
        xdr_buf_init(&args);
        status = nfstest_rpc_call(client, NLM_PROG, NLM_VERS, 0 /* NULL */,
                                  xdr_buf_data(&args), xdr_buf_size(&args),
                                  &resp, &resp_len, 5000);
        nfstest_rpc_disconnect(client);
        free(resp);
        xdr_buf_destroy(&args);
    }

    ASSERT_EQ_INT(status, NFSTEST_RPC_OK,
                  "NLM NULL proc (ping) should return RPC_OK");
    TEST_PASS();
}

/* 该服务器实现了 TEST/LOCK/CANCEL/UNLOCK 入口，但返回 NLM4_FAILED */
static void test_nlm_core_procs_failed(void) {
    REQUIRE_SERVER_OR_SKIP();

    uint16_t nlm_port = get_nlm_port();
    if (nlm_port == 0) {
        TEST_SKIP("NLM service not found via rpcbind");
    }

    const char* host = getenv("NFS_TEST_SERVER");
    int failed = 0;

    for (uint32_t proc = NLMPROC4_TEST; proc <= NLMPROC4_UNLOCK; proc++) {
        nfstest_rpc_client_t* client = nfstest_rpc_connect(host, nlm_port, 5000);
        if (!client) {
            printf("\n  FAIL  %s: cannot connect for proc %u\n", current_test_name, proc);
            tests_failed++; return;
        }
        xdr_buf_t args;
        xdr_buf_init(&args);
        pack_nlm4_args_with_lock(&args, proc);
        uint8_t* resp = NULL;
        size_t resp_len = 0;
        int status = nfstest_rpc_call(client, NLM_PROG, NLM_VERS, proc,
                                      xdr_buf_data(&args), xdr_buf_size(&args),
                                      &resp, &resp_len, 5000);
        nfstest_rpc_disconnect(client);
        xdr_buf_destroy(&args);

        uint32_t nlm_status = 0;
        if (status != NFSTEST_RPC_OK) {
            printf("\n  FAIL  %s: proc %u expected RPC_OK, got %d\n",
                   current_test_name, proc, status);
            failed = 1;
        } else if (!unpack_nlm4_status(resp, resp_len, &nlm_status)) {
            printf("\n  FAIL  %s: proc %u failed to unpack NLM status\n",
                   current_test_name, proc);
            failed = 1;
        } else if (nlm_status != NLM4_FAILED) {
            printf("\n  FAIL  %s: proc %u expected NLM4_FAILED(%u), got %u\n",
                   current_test_name, proc, NLM4_FAILED, nlm_status);
            failed = 1;
        }
        free(resp);
    }

    if (failed) { tests_failed++; return; }
    TEST_PASS();
}

/* 其余 NLM4 过程号不支持，RPC 层返回 PROC_UNAVAIL */
static void test_nlm_remaining_procs_unavail(void) {
    REQUIRE_SERVER_OR_SKIP();

    uint16_t nlm_port = get_nlm_port();
    if (nlm_port == 0) {
        TEST_SKIP("NLM service not found via rpcbind");
    }

    const char* host = getenv("NFS_TEST_SERVER");
    int failed = 0;

    for (uint32_t proc = 5; proc <= 23; proc++) {
        nfstest_rpc_client_t* client = nfstest_rpc_connect(host, nlm_port, 5000);
        if (!client) {
            printf("\n  FAIL  %s: cannot connect for proc %u\n", current_test_name, proc);
            tests_failed++; return;
        }
        xdr_buf_t args;
        xdr_buf_init(&args);
        uint8_t* resp = NULL;
        size_t resp_len = 0;
        int status = nfstest_rpc_call(client, NLM_PROG, NLM_VERS, proc,
                                      xdr_buf_data(&args), xdr_buf_size(&args),
                                      &resp, &resp_len, 5000);
        nfstest_rpc_disconnect(client);
        free(resp);
        xdr_buf_destroy(&args);

        if (status != NFSTEST_RPC_PROC_UNAVAIL) {
            printf("\n  FAIL  %s: proc %u expected PROC_UNAVAIL(-5), got %d\n",
                   current_test_name, proc, status);
            failed = 1;
        }
    }

    if (failed) { tests_failed++; return; }
    TEST_PASS();
}

int main(void) {
    printf("=== NFS lockd PROC_UNAVAIL Test (C) ===\n\n");
    RUN_TEST(test_nlm_null_proc);
    RUN_TEST(test_nlm_core_procs_failed);
    RUN_TEST(test_nlm_remaining_procs_unavail);
    PRINT_SUMMARY();
    return tests_failed;
}
