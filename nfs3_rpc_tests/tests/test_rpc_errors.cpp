#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class RpcErrorsTest : public NFS3TestContext {};

TEST_F(RpcErrorsTest, ConnectionRefused) {
    GTEST_SKIP() << "TODO: Test connection refused error handling";
}

TEST_F(RpcErrorsTest, TimeoutError) {
    GTEST_SKIP() << "TODO: Test RPC timeout error handling";
}

TEST_F(RpcErrorsTest, AuthError) {
    GTEST_SKIP() << "TODO: Test authentication error handling";
}

TEST_F(RpcErrorsTest, RpcMismatch) {
    GTEST_SKIP() << "TODO: Test RPC version mismatch error";
}

TEST_F(RpcErrorsTest, ProcUnavail) {
    GTEST_SKIP() << "TODO: Test procedure unavailable error";
}
