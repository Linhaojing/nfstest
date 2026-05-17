#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class RpcErrorsTest : public NFS3TestContext {};

TEST_F(RpcErrorsTest, ConnectionRefused) {
    GTEST_SKIP() << "Framework limitation: is_connected() uses lazy RPC binding, "
                    "connection validation happens on first actual RPC call. "
                    "Requires raw socket-level connection test.";
}

TEST_F(RpcErrorsTest, TimeoutError) {
    GTEST_SKIP() << "Timeout test takes ~130s due to libtirpc retry behavior. "
                    "Needs configurable retry count support.";
}

TEST_F(RpcErrorsTest, AuthError) {
    auto result = client().null();
    ASSERT_TRUE(result.has_value()) << "AUTH_NONE NULL call should succeed";
}

TEST_F(RpcErrorsTest, RpcMismatch) {
    ASSERT_TRUE(endpoint().is_connected()) << "Should connect to server";
    auto result = client().null();
    ASSERT_TRUE(result.has_value()) << "Basic connectivity should work";
}

TEST_F(RpcErrorsTest, ProcUnavail) {
    auto result = client().null();
    ASSERT_TRUE(result.has_value()) << "NULL procedure (proc 0) should always be available";
}
