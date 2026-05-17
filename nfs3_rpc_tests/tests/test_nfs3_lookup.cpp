#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;
using nfs3::xdr::XdrBuffer;

class Nfs3LookupTest : public NFS3TestContext {};

template<typename T>
static T XdrRoundTrip(const T& original) {
    XdrBuffer buf;
    buf.pack(original);
    XdrBuffer back(buf.data());
    T restored;
    back.unpack(restored);
    return restored;
}

TEST_F(Nfs3LookupTest, LookupArgsRoundTrip) {
    nfs3::LOOKUP3args args;
    args.what_dir.data = {0x01, 0x02, 0x03};
    args.what_name = "testfile.txt";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.what_dir.data, restored.what_dir.data);
    ASSERT_EQ(args.what_name, restored.what_name);
}

TEST_F(Nfs3LookupTest, LookupArgsEmptyName) {
    nfs3::LOOKUP3args args;
    args.what_dir.data = {0xFF, 0xFE, 0xFD};
    args.what_name = "";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.what_name, restored.what_name);
}

TEST_F(Nfs3LookupTest, LookupArgsLongPath) {
    nfs3::LOOKUP3args args;
    args.what_dir.data = {0x01};
    args.what_name = "this/is/a/very/deep/nested/path/to/file.txt";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.what_name, restored.what_name);
}

TEST_F(Nfs3LookupTest, LookupResOkRoundTrip) {
    nfs3::LOOKUP3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->object.data = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    res.resok->obj_attributes.follow = true;
    res.resok->obj_attributes.attributes.type_ = nfs3::ftype3::NF3REG;
    res.resok->obj_attributes.attributes.size = 8192;
    res.resok->dir_attributes.follow = true;
    res.resok->dir_attributes.attributes.type_ = nfs3::ftype3::NF3DIR;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
    ASSERT_EQ(res.resok->object.data, restored.resok->object.data);
}

TEST_F(Nfs3LookupTest, LookupResNoEntRoundTrip) {
    nfs3::LOOKUP3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_NOENT;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3LookupTest, LookupResNotDirRoundTrip) {
    nfs3::LOOKUP3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_NOTDIR;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}
