#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3RemoveTest : public NFS3TestContext {};

static nfs3::sattr3 FileAttrs() {
    nfs3::sattr3 attrs;
    attrs.mode = 0644;
    attrs.size = 0;
    return attrs;
}

static nfs3::sattr3 DirAttrs() {
    nfs3::sattr3 attrs;
    attrs.mode = 0755;
    return attrs;
}

TEST_F(Nfs3RemoveTest, RemoveExistingFileSucceeds) {
    const std::string name = unique_name("remove_file");

    auto create = client().create(root(), name, nfs3::createmode3::UNCHECKED, FileAttrs());
    ASSERT_TRUE(create.has_value()) << "CREATE should succeed before REMOVE";

    auto remove = client().remove(root(), name);
    ASSERT_TRUE(remove.has_value()) << "REMOVE of created file should succeed";

    auto lookup = client().lookup(root(), name);
    EXPECT_FALSE(lookup.has_value()) << "removed file should no longer be found";
}

TEST_F(Nfs3RemoveTest, RemoveNonexistentFileFails) {
    auto remove = client().remove(root(), unique_name("remove_missing"));
    EXPECT_FALSE(remove.has_value()) << "REMOVE of nonexistent file should fail";
}

TEST_F(Nfs3RemoveTest, RemoveDirectoryFailsWithoutDeletingIt) {
    const std::string name = unique_name("remove_dir");

    auto mkdir = client().mkdir(root(), name, DirAttrs());
    ASSERT_TRUE(mkdir.has_value()) << "MKDIR should succeed before REMOVE directory test";

    auto remove = client().remove(root(), name);
    EXPECT_FALSE(remove.has_value()) << "REMOVE on a directory should fail";

    auto rmdir = client().rmdir(root(), name);
    EXPECT_TRUE(rmdir.has_value()) << "cleanup RMDIR should succeed";
}
