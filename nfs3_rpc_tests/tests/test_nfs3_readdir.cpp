#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3ReaddirTest : public NFS3TestContext {};

TEST_F(Nfs3ReaddirTest, ReaddirRoot) {
    GTEST_SKIP() << "TODO: Test READDIR on root directory";
}

TEST_F(Nfs3ReaddirTest, ReaddirEntries) {
    GTEST_SKIP() << "TODO: Test READDIR returns directory entries";
}

TEST_F(Nfs3ReaddirTest, ReaddirCookie) {
    GTEST_SKIP() << "TODO: Test READDIR with cookie for continuation";
}

TEST_F(Nfs3ReaddirTest, ReaddirPlusBasic) {
    GTEST_SKIP() << "TODO: Test READDIRPLUS basic operation";
}

TEST_F(Nfs3ReaddirTest, ReaddirPlusAttributes) {
    GTEST_SKIP() << "TODO: Test READDIRPLUS returns attributes with entries";
}

TEST_F(Nfs3ReaddirTest, ReaddirEmptyDir) {
    GTEST_SKIP() << "TODO: Test READDIR on empty directory";
}
