#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class RpcNullTest : public NFS3TestContext {};

TEST_F(RpcNullTest, BasicNullCall) {
    GTEST_SKIP() << "TODO: Test basic RPC null procedure call";
}

TEST_F(RpcNullTest, NullWithConnection) {
    GTEST_SKIP() << "TODO: Test null call with active connection";
}

TEST_F(RpcNullTest, MultipleNullCalls) {
    GTEST_SKIP() << "TODO: Test multiple consecutive null calls";
}

TEST_F(RpcNullTest, NullAfterOtherOperations) {
    GTEST_SKIP() << "TODO: Test null call after other RPC operations";
}
