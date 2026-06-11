#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3GetattrTest : public NFS3TestContext {};

static nfs3::sattr3 RegularFileAttrs() {
    nfs3::sattr3 attrs;
    attrs.mode = 0644;
    attrs.size = 0;
    return attrs;
}

TEST_F(Nfs3GetattrTest, RootGetattrSucceeds) {
    auto result = client().getattr(root());
    ASSERT_TRUE(result.has_value()) << "GETATTR on root should succeed";
    EXPECT_EQ(result->obj_attributes.type_, nfs3::ftype3::NF3DIR);
    EXPECT_NE(result->obj_attributes.mode, 0u);
}

TEST_F(Nfs3GetattrTest, CreatedFileGetattrSucceeds) {
    const std::string name = unique_name("getattr_file");

    auto create = client().create(root(), name, nfs3::createmode3::UNCHECKED, RegularFileAttrs());
    ASSERT_TRUE(create.has_value()) << "CREATE should succeed before GETATTR";

    auto result = client().getattr(create->object);
    EXPECT_TRUE(result.has_value()) << "GETATTR on created file should succeed";
    if (result.has_value()) {
        EXPECT_EQ(result->obj_attributes.type_, nfs3::ftype3::NF3REG);
        EXPECT_EQ(result->obj_attributes.size, 0u);
    }

    auto remove = client().remove(root(), name);
    EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed";
}

TEST_F(Nfs3GetattrTest, InvalidHandleGetattrFails) {
    nfs3::nfs_fh3 invalid_fh;
    invalid_fh.data.resize(nfs3::NFS3_FHSIZE, 0);

    auto result = client().getattr(invalid_fh);
    EXPECT_FALSE(result.has_value()) << "GETATTR with an invalid handle should fail";
}
