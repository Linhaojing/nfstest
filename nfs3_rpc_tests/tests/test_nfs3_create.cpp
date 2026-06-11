#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3CreateTest : public NFS3TestContext {};

static nfs3::sattr3 RegularFileAttrs() {
    nfs3::sattr3 attrs;
    attrs.mode = 0644;
    attrs.size = 0;
    return attrs;
}

TEST_F(Nfs3CreateTest, UncheckedCreateSucceeds) {
    const std::string name = unique_name("create_unchecked");

    auto create = client().create(root(), name, nfs3::createmode3::UNCHECKED, RegularFileAttrs());
    ASSERT_TRUE(create.has_value()) << "UNCHECKED CREATE should succeed";
    EXPECT_FALSE(create->object.data.empty());

    auto remove = client().remove(root(), name);
    EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed";
}

TEST_F(Nfs3CreateTest, GuardedDuplicateCreateFails) {
    const std::string name = unique_name("create_guarded");

    auto first = client().create(root(), name, nfs3::createmode3::GUARDED, RegularFileAttrs());
    ASSERT_TRUE(first.has_value()) << "initial GUARDED CREATE should succeed";

    auto duplicate = client().create(root(), name, nfs3::createmode3::GUARDED, RegularFileAttrs());
    EXPECT_FALSE(duplicate.has_value()) << "duplicate GUARDED CREATE should fail";

    auto remove = client().remove(root(), name);
    EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed";
}

TEST_F(Nfs3CreateTest, CreatedFileAttributesCanBeRead) {
    const std::string name = unique_name("create_getattr");

    auto create = client().create(root(), name, nfs3::createmode3::UNCHECKED, RegularFileAttrs());
    ASSERT_TRUE(create.has_value()) << "CREATE should succeed";

    auto getattr = client().getattr(create->object);
    EXPECT_TRUE(getattr.has_value()) << "GETATTR on created file should succeed";
    if (getattr.has_value()) {
        EXPECT_EQ(getattr->obj_attributes.type_, nfs3::ftype3::NF3REG);
        EXPECT_EQ(getattr->obj_attributes.size, 0u);
    }

    auto remove = client().remove(root(), name);
    EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed";
}
