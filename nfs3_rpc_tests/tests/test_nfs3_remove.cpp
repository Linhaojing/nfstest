#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3RemoveTest : public NFS3TestContext {};

TEST_F(Nfs3RemoveTest, RemoveExistingFile) {
    GTEST_SKIP() << "TODO: Test REMOVE existing file from directory";
}

TEST_F(Nfs3RemoveTest, RemoveNonExistent) {
    GTEST_SKIP() << "TODO: Test REMOVE non-existent file returns error";
}

TEST_F(Nfs3RemoveTest, RemoveEmptyDirectory) {
    GTEST_SKIP() << "TODO: Test RMDIR on empty directory";
}

TEST_F(Nfs3RemoveTest, RemoveNonEmptyDirectory) {
    GTEST_SKIP() << "TODO: Test RMDIR on non-empty directory fails";
}

TEST_F(Nfs3RemoveTest, RemoveThenLookup) {
    GTEST_SKIP() << "TODO: Test REMOVE then LOOKUP confirms file removed";
}
