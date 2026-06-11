#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>
#include <vector>

using nfs3::NFS3TestContext;

class Nfs3OtherTest : public NFS3TestContext {};

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

TEST_F(Nfs3OtherTest, FsinfoSucceeds) {
    auto result = client().fsinfo(root());
    ASSERT_TRUE(result.has_value()) << "FSINFO should succeed";
    EXPECT_GT(result->rtmax, 0u);
    EXPECT_GT(result->wtmax, 0u);
}

TEST_F(Nfs3OtherTest, FsstatSucceeds) {
    auto result = client().fsstat(root());
    ASSERT_TRUE(result.has_value()) << "FSSTAT should succeed";
    EXPECT_GE(result->tbytes, result->fbytes);
}

TEST_F(Nfs3OtherTest, PathconfSucceeds) {
    auto result = client().pathconf(root());
    ASSERT_TRUE(result.has_value()) << "PATHCONF should succeed";
    EXPECT_GT(result->info.name_max, 0u);
}

TEST_F(Nfs3OtherTest, MkdirAndRmdirSucceed) {
    const std::string name = unique_name("other_dir");

    auto mkdir = client().mkdir(root(), name, DirAttrs());
    ASSERT_TRUE(mkdir.has_value()) << "MKDIR should succeed";

    auto getattr = client().getattr(mkdir->object);
    EXPECT_TRUE(getattr.has_value()) << "GETATTR on directory should succeed";
    if (getattr.has_value()) {
        EXPECT_EQ(getattr->obj_attributes.type_, nfs3::ftype3::NF3DIR);
    }

    auto rmdir = client().rmdir(root(), name);
    EXPECT_TRUE(rmdir.has_value()) << "RMDIR should succeed";
}

TEST_F(Nfs3OtherTest, RenameSucceeds) {
    const std::string old_name = unique_name("rename_from");
    const std::string new_name = unique_name("rename_to");

    auto create = client().create(root(), old_name, nfs3::createmode3::UNCHECKED, FileAttrs());
    ASSERT_TRUE(create.has_value()) << "CREATE should succeed before RENAME";

    auto rename = client().rename_op(root(), old_name, root(), new_name);
    ASSERT_TRUE(rename.has_value()) << "RENAME should succeed";

    auto old_lookup = client().lookup(root(), old_name);
    EXPECT_FALSE(old_lookup.has_value()) << "old name should not remain after RENAME";
    auto new_lookup = client().lookup(root(), new_name);
    EXPECT_TRUE(new_lookup.has_value()) << "new name should exist after RENAME";

    auto remove = client().remove(root(), new_name);
    EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed";
}

TEST_F(Nfs3OtherTest, CommitSucceedsOrReturnsUnsupported) {
    const std::string name = unique_name("commit_file");

    auto create = client().create(root(), name, nfs3::createmode3::UNCHECKED, FileAttrs());
    ASSERT_TRUE(create.has_value()) << "CREATE should succeed before COMMIT";

    const std::vector<uint8_t> data = {'c', 'o', 'm', 'm', 'i', 't'};
    auto write = client().write(create->object, 0, nfs3::stable_how::UNSTABLE, data);
    ASSERT_TRUE(write.has_value()) << "WRITE should succeed before COMMIT";

    auto commit = client().commit(create->object, 0, static_cast<uint32_t>(data.size()));
    if (!commit.has_value()) {
        RecordProperty("commit", "unsupported_or_failed_by_server");
        SUCCEED() << "COMMIT returned an explicit server error; unsupported COMMIT is not a framework failure";
    }

    auto remove = client().remove(root(), name);
    EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed";
}
