#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <future>
#include <random>

using nfs3::NFS3TestContext;
using nfs3::xdr::XdrBuffer;

class Nfs3StressTest : public NFS3TestContext {};

template<typename T>
static T XdrRoundTrip(const T& original) {
    XdrBuffer buf;
    buf.pack(original);
    XdrBuffer back(buf.data());
    T restored;
    back.unpack(restored);
    return restored;
}

TEST_F(Nfs3StressTest, LargeFilehandleRoundTrip) {
    nfs3::nfs_fh3 fh;
    fh.data.resize(64, 0xAB);
    fh.data[0] = 0x01;
    fh.data[63] = 0xFF;
    auto restored = XdrRoundTrip(fh);
    ASSERT_EQ(fh.data, restored.data);
}

TEST_F(Nfs3StressTest, LargeDataPayloadRoundTrip) {
    nfs3::WRITE3args args;
    args.file.data = {0x01};
    args.offset = 0;
    args.stable = nfs3::stable_how::UNSTABLE;
    args.data.resize(65536, 0x42);
    args.data[0] = 0xDE;
    args.data[65535] = 0xAD;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.data.size(), restored.data.size());
    ASSERT_EQ(args.data[0], restored.data[0]);
    ASSERT_EQ(args.data[65535], restored.data[65535]);
}

TEST_F(Nfs3StressTest, ManyFieldsSattr3RoundTrip) {
    nfs3::sattr3 attr;
    attr.mode = 0644;
    attr.uid = 1000;
    attr.gid = 1000;
    attr.size = 1048576;
    attr.atime.emplace();
    attr.atime->seconds = 1700000000;
    attr.atime->nseconds = 123456;
    attr.mtime.emplace();
    attr.mtime->seconds = 1700000001;
    attr.mtime->nseconds = 654321;
    auto restored = XdrRoundTrip(attr);
    ASSERT_EQ(attr.mode, restored.mode);
    ASSERT_EQ(attr.uid, restored.uid);
    ASSERT_EQ(attr.gid, restored.gid);
    ASSERT_EQ(attr.size, restored.size);
    ASSERT_TRUE(restored.atime.has_value());
    ASSERT_TRUE(restored.mtime.has_value());
}

TEST_F(Nfs3StressTest, MultipleRoundTripsConsistency) {
    for (int i = 0; i < 1000; ++i) {
        nfs3::LOOKUP3args args;
        args.what_dir.data = {static_cast<uint8_t>(i & 0xFF)};
        args.what_name = "file_" + std::to_string(i);
        auto restored = XdrRoundTrip(args);
        ASSERT_EQ(args.what_name, restored.what_name);
        ASSERT_EQ(args.what_dir.data, restored.what_dir.data);
    }
}

TEST_F(Nfs3StressTest, ParallelRoundTrips) {
    std::vector<std::future<bool>> futures;
    for (int i = 0; i < 10; ++i) {
        futures.push_back(std::async(std::launch::async, [i]() {
            for (int j = 0; j < 100; ++j) {
                nfs3::GETATTR3args args;
                args.object.data.resize(4);
                args.object.data[0] = static_cast<uint8_t>(i);
                args.object.data[1] = static_cast<uint8_t>(j >> 8);
                args.object.data[2] = static_cast<uint8_t>(j & 0xFF);
                args.object.data[3] = static_cast<uint8_t>((i + j) & 0xFF);
                auto restored = XdrRoundTrip(args);
                if (args.object.data != restored.object.data) {
                    return false;
                }
            }
            return true;
        }));
    }
    for (auto& f : futures) {
        ASSERT_TRUE(f.get());
    }
}

TEST_F(Nfs3StressTest, MixedProtocolOpsRoundTrip) {
    nfs3::GETATTR3args ga;
    ga.object.data = {0x01, 0x02};
    auto ga_r = XdrRoundTrip(ga);
    ASSERT_EQ(ga.object.data, ga_r.object.data);

    nfs3::READ3args ra;
    ra.file.data = {0x03, 0x04};
    ra.offset = 4096;
    ra.count = 8192;
    auto ra_r = XdrRoundTrip(ra);
    ASSERT_EQ(ra.offset, ra_r.offset);

    nfs3::WRITE3args wa;
    wa.file.data = {0x05};
    wa.offset = 0;
    wa.stable = nfs3::stable_how::FILE_SYNC;
    wa.data = {'t', 'e', 's', 't'};
    auto wa_r = XdrRoundTrip(wa);
    ASSERT_EQ(wa.data, wa_r.data);

    nfs3::REMOVE3args rm;
    rm.object_dir.data = {0x06};
    rm.object_name = "cleanup.txt";
    auto rm_r = XdrRoundTrip(rm);
    ASSERT_EQ(rm.object_name, rm_r.object_name);
}
