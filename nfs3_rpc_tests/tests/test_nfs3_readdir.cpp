#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using nfs3::NFS3TestContext;

class Nfs3ReaddirTest : public NFS3TestContext {};

static nfs3::sattr3 RegularFileAttrs() {
    nfs3::sattr3 attrs;
    attrs.mode = 0644;
    attrs.size = 0;
    return attrs;
}

static bool ContainsEntry(const nfs3::READDIR3resok& dir, const std::string& name) {
    const nfs3::entry3* entry = dir.reply.entries.get();
    while (entry != nullptr) {
        if (entry->name == name) {
            return true;
        }
        entry = entry->nextentry.get();
    }
    return false;
}

TEST_F(Nfs3ReaddirTest, ReaddirFindsCreatedFiles) {
    std::vector<std::string> names;
    for (int i = 0; i < 3; ++i) {
        names.push_back(unique_name("readdir_file") + "_" + std::to_string(i));
        auto create = client().create(root(), names.back(), nfs3::createmode3::UNCHECKED, RegularFileAttrs());
        ASSERT_TRUE(create.has_value()) << "CREATE should succeed for " << names.back();
    }

    auto dir = client().readdir(root(), 0, 0, 32768);
    ASSERT_TRUE(dir.has_value()) << "READDIR should succeed";
    for (const auto& name : names) {
        EXPECT_TRUE(ContainsEntry(*dir, name)) << "READDIR should contain " << name;
    }

    for (const auto& name : names) {
        auto remove = client().remove(root(), name);
        EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed for " << name;
    }
}
