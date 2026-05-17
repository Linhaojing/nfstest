#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;
using nfs3::xdr::XdrBuffer;

class Nfs3ReaddirTest : public NFS3TestContext {};

template<typename T>
static T XdrRoundTrip(const T& original) {
    XdrBuffer buf;
    buf.pack(original);
    XdrBuffer back(buf.data());
    T restored;
    back.unpack(restored);
    return restored;
}

TEST_F(Nfs3ReaddirTest, ReaddirArgsRoundTrip) {
    nfs3::READDIR3args args;
    args.dir.data = {0x01, 0x02};
    args.cookie = 0;
    args.cookieverf = 0;
    args.count = 8192;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.dir.data, restored.dir.data);
    ASSERT_EQ(args.count, restored.count);
}

TEST_F(Nfs3ReaddirTest, ReaddirArgsWithCookie) {
    nfs3::READDIR3args args;
    args.dir.data = {0xFF};
    args.cookie = 42;
    args.cookieverf = 0x123456789ABCDEF0ULL;
    args.count = 4096;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.cookie, restored.cookie);
}

TEST_F(Nfs3ReaddirTest, ReaddirArgsLargeCount) {
    nfs3::READDIR3args args;
    args.dir.data = {0x01};
    args.count = 65536;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.count, restored.count);
}

TEST_F(Nfs3ReaddirTest, ReaddirPlusArgsRoundTrip) {
    nfs3::READDIRPLUS3args args;
    args.dir.data = {0x01, 0x02};
    args.cookie = 0;
    args.cookieverf = 0;
    args.dircount = 4096;
    args.maxcount = 32768;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.dircount, restored.dircount);
    ASSERT_EQ(args.maxcount, restored.maxcount);
}

TEST_F(Nfs3ReaddirTest, ReaddirPlusArgsMaxValues) {
    nfs3::READDIRPLUS3args args;
    args.dir.data = {0xFF};
    args.cookie = UINT64_MAX;
    args.cookieverf = UINT64_MAX;
    args.dircount = UINT32_MAX;
    args.maxcount = UINT32_MAX;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.dircount, restored.dircount);
}

TEST_F(Nfs3ReaddirTest, FsstatArgsRoundTrip) {
    nfs3::FSSTAT3args args;
    args.fsroot.data = {0x01, 0x02, 0x03};
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.fsroot.data, restored.fsroot.data);
}

TEST_F(Nfs3ReaddirTest, FsinfoArgsRoundTrip) {
    nfs3::FSINFO3args args;
    args.fsroot.data = {0xAA, 0xBB};
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.fsroot.data, restored.fsroot.data);
}

TEST_F(Nfs3ReaddirTest, PathconfArgsRoundTrip) {
    nfs3::PATHCONF3args args;
    args.object.data = {0xDE, 0xAD};
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.object.data, restored.object.data);
}

TEST_F(Nfs3ReaddirTest, CommitArgsRoundTrip) {
    nfs3::COMMIT3args args;
    args.file.data = {0x01, 0x02};
    args.offset = 0;
    args.count = 0;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.file.data, restored.file.data);
}

TEST_F(Nfs3ReaddirTest, CommitArgsWithOffset) {
    nfs3::COMMIT3args args;
    args.file.data = {0xFF};
    args.offset = 4096;
    args.count = 8192;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.offset, restored.offset);
    ASSERT_EQ(args.count, restored.count);
}
