#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3LookupTest : public NFS3TestContext {};

TEST_F(Nfs3LookupTest, LookupExistingFile) {
    GTEST_SKIP() << "TODO: Test LOOKUP for existing file in directory";
}

TEST_F(Nfs3LookupTest, LookupExistingDir) {
    GTEST_SKIP() << "TODO: Test LOOKUP for existing subdirectory";
}

TEST_F(Nfs3LookupTest, LookupNonExistent) {
    GTEST_SKIP() << "TODO: Test LOOKUP for non-existent entry returns error";
}

TEST_F(Nfs3LookupTest, LookupDot) {
    GTEST_SKIP() << "TODO: Test LOOKUP for '.' returns same directory";
}

TEST_F(Nfs3LookupTest, LookupDotDot) {
    GTEST_SKIP() << "TODO: Test LOOKUP for '..' returns parent directory";
}

TEST_F(Nfs3LookupTest, HandleAndAttributes) {
    GTEST_SKIP() << "TODO: Test LOOKUP returns valid handle and attributes";
}
