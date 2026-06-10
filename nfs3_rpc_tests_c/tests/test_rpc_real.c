#include "test_framework.h"
#include "nfstest/rpc_client.h"
#include <stdint.h>
#include <stdlib.h>

#define NFS_PROGRAM 100003U
#define NFS_V3 3U
#define NFSPROC3_NULL 0U

static void test_nfs_null_rpc_allows_empty_args(void) {
    nfstest_rpc_client_t* client = nfstest_rpc_connect("127.0.0.1", 2049, 3000);
    ASSERT_TRUE(client != NULL, "connect to NFS server");

    uint8_t* resp_data = NULL;
    size_t resp_len = 0;
    int status = nfstest_rpc_call(
        client,
        NFS_PROGRAM,
        NFS_V3,
        NFSPROC3_NULL,
        NULL,
        0,
        &resp_data,
        &resp_len,
        3000
    );

    ASSERT_EQ_INT(status, NFSTEST_RPC_OK, "NFS NULL RPC should accept empty args");
    ASSERT_TRUE(resp_data == NULL, "empty RPC response should not allocate data");
    ASSERT_EQ_U32((uint32_t)resp_len, 0, "empty RPC response length");

    nfstest_rpc_disconnect(client);
    free(resp_data);

    TEST_PASS();
}

int main(void) {
    printf("=== Real RPC Integration Tests (C) ===\n\n");
    RUN_TEST(test_nfs_null_rpc_allows_empty_args);
    PRINT_SUMMARY();
    return tests_failed;
}
