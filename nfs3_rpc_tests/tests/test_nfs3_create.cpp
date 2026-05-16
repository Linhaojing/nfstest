#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3CreateTest : public NFS3TestContext {};

TEST_F(Nfs3CreateTest, CreateRegularFile) {
    GTEST_SKIP() << "TODO: Test CREATE new regular file";
}

TEST_F(Nfs3CreateTest, CreateUnchecked) {
    GTEST_SKIP() << "TODO: Test CREATE with UNCHECKED mode";
}

TEST_F(Nfs3CreateTest, CreateGuarded) {
    GTEST_SKIP() << "TODO: Test CREATE with GUARDED mode";
}

TEST_F(Nfs3CreateTest, CreateExclusive) {
    GTEST_SKIP() << "TODO: Test CREATE with EXCLUSIVE mode";
}

TEST_F(Nfs3CreateTest, CreateDuplicateGuarded) {
    GTEST_SKIP() << "TODO: Test CREATE duplicate file with GUARDED fails";
}

TEST_F(Nfs3CreateTest, CreateWithAttributes) {
    GTEST_SKIP() << "TODO: Test CREATE file with specified attributes";
}

TEST_F(Nfs3CreateTest, VerifyCreatedFile) {
    GTEST_SKIP() << "TODO: Test CREATE then LOOKUP/GETATTR to verify file exists";
}
