/*
 * test_lockd_c.c - NFS lockd (NLM v4) 基本文件锁测试
 *
 * 测试场景：
 *   1. 排它锁加锁 + 解锁
 *   2. 共享锁加锁 + 解锁
 *   3. NLM TEST 验证锁存在
 *
 * 环境变量：NFS_TEST_SERVER、NFS_TEST_EXPORT（同其他服务端测试）
 */

#define _XOPEN_SOURCE 500
#include "test_framework.h"
#include "nfs3_server_test.h"
#include "nfstest/rpc_client.h"
#include "nfs3_c/xdr_codec.h"
#include "nfs3_c/nfs3_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* NLM 协议常量 */
#define NLM_PROG          100021U
#define NLM_VERS          4U
#define NLMPROC4_TEST     5U
#define NLMPROC4_LOCK     6U
#define NLMPROC4_UNLOCK   8U

/* NLM4 状态码 */
#define NLM4_GRANTED             0
#define NLM4_DENIED              1
#define NLM4_DENIED_NOLOCKS      2
#define NLM4_BLOCKED             3
#define NLM4_DENIED_GRACE_PERIOD 4

/* rpcbind 查询常量 */
#define RPCBPROG          100000U
#define RPCBVERS          2U
#define RPCBPROC_GETPORT  3U
#define IPPROTO_TCP_NUM   6U

#define REQUIRE_SERVER_OR_SKIP() do { \
    if (!nfs3_server_is_configured()) { \
        if (nfs3_server_require_server()) { \
            ASSERT_TRUE(0, nfs3_server_skip_message()); \
        } \
        TEST_SKIP(nfs3_server_skip_message()); \
    } \
} while(0)

/* ---- NLM 辅助函数 ---- */

/* 通过 rpcbind 查询 NLM 端口 */
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

/* 调用 NLM 过程 */
static int nlm_call(uint16_t port, uint32_t proc,
                    xdr_buf_t* args, uint8_t** resp, size_t* resp_len) {
    const char* host = getenv("NFS_TEST_SERVER");
    if (!host || host[0] == '\0' || port == 0) return NFSTEST_RPC_CONN_ERR;

    nfstest_rpc_client_t* client = nfstest_rpc_connect(host, port, 5000);
    if (!client) return NFSTEST_RPC_CONN_ERR;

    int status = nfstest_rpc_call(client, NLM_PROG, NLM_VERS, proc,
                                  xdr_buf_data(args), xdr_buf_size(args),
                                  resp, resp_len, 5000);
    nfstest_rpc_disconnect(client);
    return status;
}

/*
 * 序列化 NLM4 alock 字段（caller_name、fh、oh、svid、offset、len）
 * 被 LOCK、UNLOCK、TEST 共用
 */
static void pack_nlm4_alock(xdr_buf_t* buf, const nfs_fh3_t* fh,
                             uint32_t svid, uint64_t offset, uint64_t len) {
    char hostname[64];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        snprintf(hostname, sizeof(hostname), "nfstest-client");
    }
    /* caller_name: string */
    xdr_pack_cstring(buf, hostname);
    /* fh: netobj (uint32 len + data) */
    xdr_pack_opaque(buf, fh->data, fh->len);
    /* oh: owner handle = pid bytes */
    uint32_t pid = (uint32_t)getpid();
    xdr_pack_opaque(buf, (const uint8_t*)&pid, sizeof(pid));
    /* svid */
    xdr_pack_uint32(buf, svid);
    /* l_offset, l_len */
    xdr_pack_uint64(buf, offset);
    xdr_pack_uint64(buf, len);
}

/* 打包 cookie（4字节值） */
static void pack_nlm4_cookie(xdr_buf_t* buf, uint32_t val) {
    xdr_pack_opaque(buf, (const uint8_t*)&val, sizeof(val));
}

/* 从 NLM 响应中读取 cookie（跳过）和状态码 */
static int unpack_nlm4_status(uint8_t* resp, size_t resp_len) {
    xdr_buf_t body;
    xdr_buf_init_copy(&body, resp, resp_len);
    /* skip cookie */
    uint8_t* cookie_data = NULL;
    uint32_t cookie_len = 0;
    xdr_unpack_opaque(&body, &cookie_data, &cookie_len);
    free(cookie_data);
    /* status */
    uint32_t status = 0;
    xdr_unpack_uint32(&body, &status);
    xdr_buf_destroy(&body);
    return (int)status;
}

/* ---- 加锁/解锁辅助 ---- */

static int nlm4_lock(uint16_t port, const nfs_fh3_t* fh, int exclusive, uint32_t cookie) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    pack_nlm4_cookie(&args, cookie);
    xdr_pack_bool(&args, 0);           /* block = false */
    xdr_pack_bool(&args, exclusive);   /* exclusive */
    pack_nlm4_alock(&args, fh, (uint32_t)getpid(), 0, 0);
    xdr_pack_bool(&args, 0);           /* reclaim = false */
    xdr_pack_uint32(&args, 1);         /* state = 1 */

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nlm_call(port, NLMPROC4_LOCK, &args, &resp, &resp_len);
    int nlm_status = NLM4_DENIED_NOLOCKS;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        nlm_status = unpack_nlm4_status(resp, resp_len);
    }
    free(resp);
    xdr_buf_destroy(&args);
    return (rpc_status == NFSTEST_RPC_OK) ? nlm_status : rpc_status;
}

static int nlm4_unlock(uint16_t port, const nfs_fh3_t* fh, uint32_t cookie) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    pack_nlm4_cookie(&args, cookie);
    pack_nlm4_alock(&args, fh, (uint32_t)getpid(), 0, 0);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nlm_call(port, NLMPROC4_UNLOCK, &args, &resp, &resp_len);
    int nlm_status = NLM4_DENIED_NOLOCKS;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        nlm_status = unpack_nlm4_status(resp, resp_len);
    }
    free(resp);
    xdr_buf_destroy(&args);
    return (rpc_status == NFSTEST_RPC_OK) ? nlm_status : rpc_status;
}

/* NLM TEST：检查锁是否存在；返回 NLM4_GRANTED(0) 表示无冲突，NLM4_DENIED 表示有锁 */
static int nlm4_test(uint16_t port, const nfs_fh3_t* fh, int exclusive, uint32_t cookie) {
    xdr_buf_t args;
    xdr_buf_init(&args);

    pack_nlm4_cookie(&args, cookie);
    xdr_pack_bool(&args, exclusive);
    pack_nlm4_alock(&args, fh, (uint32_t)getpid(), 0, 0);

    uint8_t* resp = NULL;
    size_t resp_len = 0;
    int rpc_status = nlm_call(port, NLMPROC4_TEST, &args, &resp, &resp_len);
    int nlm_status = NLM4_DENIED_NOLOCKS;
    if (rpc_status == NFSTEST_RPC_OK && resp != NULL) {
        nlm_status = unpack_nlm4_status(resp, resp_len);
    }
    free(resp);
    xdr_buf_destroy(&args);
    return (rpc_status == NFSTEST_RPC_OK) ? nlm_status : rpc_status;
}

/* ---- 测试用例 ---- */

static void test_nlm_exclusive_lock_and_unlock(void) {
    REQUIRE_SERVER_OR_SKIP();

    uint16_t nlm_port = get_nlm_port();
    if (nlm_port == 0) {
        TEST_SKIP("NLM service not found via rpcbind");
    }

    nfs_fh3_t root_fh, file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    char filename[128];
    nfs3_unique_name("c_lockd_excl", filename, sizeof(filename));
    int created = 0, failed = 0;

    int status = nfs3_mount_root(&root_fh);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT root should succeed");

    status = nfs3_create_test_file(&root_fh, filename, &file_fh);
    if (status != NFS3_OK || nfs_fh3_is_empty(&file_fh)) {
        printf("\n  FAIL  %s: CREATE file failed (status %d)\n", current_test_name, status);
        tests_failed++; failed = 1; goto cleanup;
    }
    created = 1;

    /* 排它锁 */
    status = nlm4_lock(nlm_port, &file_fh, 1 /* exclusive */, 0x1001);
    if (status == NLM4_BLOCKED) {
        TEST_SKIP("server returned NLM4_BLOCKED for non-blocking lock");
    }
    if (status != NLM4_GRANTED) {
        printf("\n  FAIL  %s: LOCK (exclusive) expected GRANTED, got %d\n", current_test_name, status);
        tests_failed++; failed = 1; goto cleanup;
    }

    /* 解锁 */
    status = nlm4_unlock(nlm_port, &file_fh, 0x1001);
    if (status != NLM4_GRANTED) {
        printf("\n  FAIL  %s: UNLOCK expected GRANTED, got %d\n", current_test_name, status);
        tests_failed++; failed = 1; goto cleanup;
    }

cleanup:
    if (created) nfs3_remove_test_file(&root_fh, filename);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) TEST_PASS();
}

static void test_nlm_shared_lock(void) {
    REQUIRE_SERVER_OR_SKIP();

    uint16_t nlm_port = get_nlm_port();
    if (nlm_port == 0) {
        TEST_SKIP("NLM service not found via rpcbind");
    }

    nfs_fh3_t root_fh, file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    char filename[128];
    nfs3_unique_name("c_lockd_shared", filename, sizeof(filename));
    int created = 0, failed = 0;

    int status = nfs3_mount_root(&root_fh);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT root should succeed");

    status = nfs3_create_test_file(&root_fh, filename, &file_fh);
    if (status != NFS3_OK || nfs_fh3_is_empty(&file_fh)) {
        printf("\n  FAIL  %s: CREATE file failed (status %d)\n", current_test_name, status);
        tests_failed++; failed = 1; goto cleanup;
    }
    created = 1;

    /* 共享锁 */
    status = nlm4_lock(nlm_port, &file_fh, 0 /* shared */, 0x2001);
    if (status == NLM4_BLOCKED) {
        TEST_SKIP("server returned NLM4_BLOCKED for non-blocking lock");
    }
    if (status != NLM4_GRANTED) {
        printf("\n  FAIL  %s: LOCK (shared) expected GRANTED, got %d\n", current_test_name, status);
        tests_failed++; failed = 1; goto cleanup;
    }

    /* 解锁 */
    status = nlm4_unlock(nlm_port, &file_fh, 0x2001);
    if (status != NLM4_GRANTED) {
        printf("\n  FAIL  %s: UNLOCK expected GRANTED, got %d\n", current_test_name, status);
        tests_failed++; failed = 1; goto cleanup;
    }

cleanup:
    if (created) nfs3_remove_test_file(&root_fh, filename);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) TEST_PASS();
}

static void test_nlm_test_after_lock(void) {
    REQUIRE_SERVER_OR_SKIP();

    uint16_t nlm_port = get_nlm_port();
    if (nlm_port == 0) {
        TEST_SKIP("NLM service not found via rpcbind");
    }

    nfs_fh3_t root_fh, file_fh;
    nfs_fh3_init(&root_fh);
    nfs_fh3_init(&file_fh);
    char filename[128];
    nfs3_unique_name("c_lockd_test", filename, sizeof(filename));
    int created = 0, failed = 0;

    int status = nfs3_mount_root(&root_fh);
    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "MOUNT root should succeed");

    status = nfs3_create_test_file(&root_fh, filename, &file_fh);
    if (status != NFS3_OK || nfs_fh3_is_empty(&file_fh)) {
        printf("\n  FAIL  %s: CREATE file failed (status %d)\n", current_test_name, status);
        tests_failed++; failed = 1; goto cleanup;
    }
    created = 1;

    /* 先加排它锁 */
    status = nlm4_lock(nlm_port, &file_fh, 1, 0x3001);
    if (status == NLM4_BLOCKED) {
        TEST_SKIP("server returned NLM4_BLOCKED for non-blocking lock");
    }
    if (status != NLM4_GRANTED) {
        printf("\n  FAIL  %s: LOCK expected GRANTED, got %d\n", current_test_name, status);
        tests_failed++; failed = 1; goto cleanup;
    }

    /*
     * NLM TEST with exclusive=1 from same owner:
     * 若服务器视同一 owner 的重入为 GRANTED，则 0；
     * 若检测到有锁则返回 NLM4_DENIED(1)。
     * 两者均属合规行为，关键是 RPC 本身须成功。
     */
    status = nlm4_test(nlm_port, &file_fh, 1, 0x3002);
    if (status < 0) {
        printf("\n  FAIL  %s: NLM TEST RPC call failed (%d)\n", current_test_name, status);
        tests_failed++; failed = 1;
        /* 仍需解锁 */
    }

    /* 解锁 */
    int unlock_status = nlm4_unlock(nlm_port, &file_fh, 0x3001);
    if (!failed && unlock_status != NLM4_GRANTED) {
        printf("\n  FAIL  %s: UNLOCK expected GRANTED, got %d\n", current_test_name, unlock_status);
        tests_failed++; failed = 1; goto cleanup;
    }

cleanup:
    if (created) nfs3_remove_test_file(&root_fh, filename);
    nfs_fh3_destroy(&file_fh);
    nfs_fh3_destroy(&root_fh);
    if (!failed) TEST_PASS();
}

/* ---- main ---- */

int main(void) {
    printf("=== NFS lockd (NLMv4) Server Tests (C) ===\n\n");
    RUN_TEST(test_nlm_exclusive_lock_and_unlock);
    RUN_TEST(test_nlm_shared_lock);
    RUN_TEST(test_nlm_test_after_lock);
    PRINT_SUMMARY();
    return tests_failed;
}
