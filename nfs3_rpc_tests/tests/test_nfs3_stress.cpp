#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>
#include <vector>

using nfs3::NFS3TestContext;

class Nfs3StressTest : public NFS3TestContext {};

static nfs3::sattr3 RegularFileAttrs() {
    nfs3::sattr3 attrs;
    attrs.mode = 0644;
    attrs.size = 0;
    return attrs;
}

TEST_F(Nfs3StressTest, SmallCreateWriteReadRemoveLoop) {
    for (int i = 0; i < 5; ++i) {
        const std::string name = unique_name("stress_file") + "_" + std::to_string(i);
        auto create = client().create(root(), name, nfs3::createmode3::UNCHECKED, RegularFileAttrs());
        ASSERT_TRUE(create.has_value()) << "CREATE should succeed in stress loop";

        const std::vector<uint8_t> data = {
            's', 't', 'r', 'e', 's', 's', '_', static_cast<uint8_t>('0' + i)
        };
        auto write = client().write(create->object, 0, nfs3::stable_how::DATA_SYNC, data);
        ASSERT_TRUE(write.has_value()) << "WRITE should succeed in stress loop";
        EXPECT_EQ(write->count, data.size());

        auto read = client().read(create->object, 0, static_cast<uint32_t>(data.size()));
        ASSERT_TRUE(read.has_value()) << "READ should succeed in stress loop";
        EXPECT_EQ(read->data, data);

        auto remove = client().remove(root(), name);
        ASSERT_TRUE(remove.has_value()) << "REMOVE should succeed in stress loop";
    }
}
