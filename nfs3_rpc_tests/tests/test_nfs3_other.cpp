#include "nfs3/test_context.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3OtherTest : public NFS3TestContext {};

TEST_F(Nfs3OtherTest, SetattrModifySize) {
    GTEST_SKIP() << "TODO: Test SETATTR to truncate/extend file size";
}

TEST_F(Nfs3OtherTest, SetattrModifyMode) {
    GTEST_SKIP() << "TODO: Test SETATTR to change file permissions";
}

TEST_F(Nfs3OtherTest, AccessCheckRead) {
    GTEST_SKIP() << "TODO: Test ACCESS check for read permission";
}

TEST_F(Nfs3OtherTest, AccessCheckWrite) {
    GTEST_SKIP() << "TODO: Test ACCESS check for write permission";
}

TEST_F(Nfs3OtherTest, ReadlinkSymlink) {
    GTEST_SKIP() << "TODO: Test READLINK on symbolic link";
}

TEST_F(Nfs3OtherTest, MkdirNewDirectory) {
    GTEST_SKIP() << "TODO: Test MKDIR creates new directory";
}

TEST_F(Nfs3OtherTest, SymlinkCreate) {
    GTEST_SKIP() << "TODO: Test SYMLINK creates symbolic link";
}

TEST_F(Nfs3OtherTest, RenameFile) {
    GTEST_SKIP() << "TODO: Test RENAME moves file between directories";
}

TEST_F(Nfs3OtherTest, LinkHardLink) {
    GTEST_SKIP() << "TODO: Test LINK creates hard link";
}

TEST_F(Nfs3OtherTest, FsstatInfo) {
    GTEST_SKIP() << "TODO: Test FSSTAT returns filesystem statistics";
}

TEST_F(Nfs3OtherTest, FsinfoAndPathconf) {
    GTEST_SKIP() << "TODO: Test FSINFO and PATHCONF return configuration info";
}
