#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class RpcNullTest : public NFS3TestContext {};

TEST_F(RpcNullTest, BasicNullCall) {
    auto result = client().null();
    ASSERT_TRUE(result.has_value()) << "NFS NULL procedure should succeed";
}

TEST_F(RpcNullTest, NullWithConnection) {
    ASSERT_TRUE(endpoint().is_connected()) << "Should be connected before NULL call";
    auto result = client().null();
    ASSERT_TRUE(result.has_value()) << "NULL call should succeed on active connection";
    ASSERT_TRUE(endpoint().is_connected()) << "Should remain connected after NULL call";
}

TEST_F(RpcNullTest, MultipleNullCalls) {
    for (int i = 0; i < 100; ++i) {
        auto result = client().null();
        ASSERT_TRUE(result.has_value()) << "NULL call " << i << " should succeed";
    }
}

TEST_F(RpcNullTest, NullAfterOtherOperations) {
    auto result1 = client().null();
    ASSERT_TRUE(result1.has_value()) << "First NULL call should succeed";
    auto result2 = client().null();
    ASSERT_TRUE(result2.has_value()) << "Second NULL call should succeed";
    auto result3 = client().null();
    ASSERT_TRUE(result3.has_value()) << "Third NULL call should succeed";
}
