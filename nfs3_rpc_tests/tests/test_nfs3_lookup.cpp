#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;

class Nfs3LookupTest : public NFS3TestContext {};

static nfs3::sattr3 RegularFileAttrs() {
    nfs3::sattr3 attrs;
    attrs.mode = 0644;
    attrs.size = 0;
    return attrs;
}

TEST_F(Nfs3LookupTest, ExistingFileLookupSucceeds) {
    const std::string name = unique_name("lookup_file");

    auto create = client().create(root(), name, nfs3::createmode3::UNCHECKED, RegularFileAttrs());
    ASSERT_TRUE(create.has_value()) << "CREATE should succeed before LOOKUP";

    auto lookup = client().lookup(root(), name);
    EXPECT_TRUE(lookup.has_value()) << "LOOKUP of created file should succeed";
    if (lookup.has_value()) {
        EXPECT_FALSE(lookup->object.data.empty());
        EXPECT_EQ(lookup->object.data, create->object.data);
    }

    auto remove = client().remove(root(), name);
    EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed";
}

TEST_F(Nfs3LookupTest, NonexistentFileLookupFails) {
    auto result = client().lookup(root(), unique_name("lookup_missing"));
    EXPECT_FALSE(result.has_value()) << "LOOKUP of a nonexistent file should fail";
}
