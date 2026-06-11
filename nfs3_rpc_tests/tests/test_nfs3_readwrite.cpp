#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>
#include <vector>

using nfs3::NFS3TestContext;

class Nfs3ReadWriteTest : public NFS3TestContext {};

static nfs3::sattr3 RegularFileAttrs() {
    nfs3::sattr3 attrs;
    attrs.mode = 0644;
    attrs.size = 0;
    return attrs;
}

TEST_F(Nfs3ReadWriteTest, CreateWriteReadOffsetAndCleanup) {
    const std::string name = unique_name("readwrite_file");

    auto create = client().create(root(), name, nfs3::createmode3::UNCHECKED, RegularFileAttrs());
    ASSERT_TRUE(create.has_value()) << "CREATE should succeed";

    const std::vector<uint8_t> data = {'N', 'F', 'S', '3', '-', 'r', 'e', 'a', 'd', 'w', 'r', 'i', 't', 'e'};
    auto write = client().write(create->object, 0, nfs3::stable_how::DATA_SYNC, data);
    ASSERT_TRUE(write.has_value()) << "WRITE should succeed";
    EXPECT_EQ(write->count, data.size());

    auto read_all = client().read(create->object, 0, static_cast<uint32_t>(data.size()));
    ASSERT_TRUE(read_all.has_value()) << "READ should succeed";
    EXPECT_EQ(read_all->data, data);

    auto read_offset = client().read(create->object, 5, 4);
    ASSERT_TRUE(read_offset.has_value()) << "offset READ should succeed";
    const std::vector<uint8_t> expected_offset = {'r', 'e', 'a', 'd'};
    EXPECT_EQ(read_offset->data, expected_offset);

    auto remove = client().remove(root(), name);
    EXPECT_TRUE(remove.has_value()) << "cleanup REMOVE should succeed";
}
