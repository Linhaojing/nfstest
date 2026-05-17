#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;
using nfs3::xdr::XdrBuffer;

class Nfs3GetattrTest : public NFS3TestContext {};

template<typename T>
static T XdrRoundTrip(const T& original) {
    XdrBuffer buf;
    buf.pack(original);
    XdrBuffer back(buf.data());
    T restored;
    back.unpack(restored);
    return restored;
}

TEST_F(Nfs3GetattrTest, GetattrArgsRoundTrip) {
    nfs3::GETATTR3args args;
    args.object.data = {0x01, 0x02, 0x03, 0x04};
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.object.data, restored.object.data);
}

TEST_F(Nfs3GetattrTest, GetattrResOkRoundTrip) {
    nfs3::GETATTR3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->obj_attributes.type_ = nfs3::ftype3::NF3REG;
    res.resok->obj_attributes.mode = 0644;
    res.resok->obj_attributes.nlink = 1;
    res.resok->obj_attributes.uid = 1000;
    res.resok->obj_attributes.gid = 1000;
    res.resok->obj_attributes.size = 4096;
    res.resok->obj_attributes.used = 4096;
    res.resok->obj_attributes.fsid = 42;
    res.resok->obj_attributes.fileid = 12345;
    res.resok->obj_attributes.atime.seconds = 1700000000;
    res.resok->obj_attributes.mtime.seconds = 1700000000;
    res.resok->obj_attributes.ctime.seconds = 1700000000;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
    ASSERT_EQ(res.resok->obj_attributes.mode, restored.resok->obj_attributes.mode);
}

TEST_F(Nfs3GetattrTest, GetattrResErrorRoundTrip) {
    nfs3::GETATTR3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_NOENT;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3GetattrTest, Fattr3RegularFileRoundTrip) {
    nfs3::fattr3 attr;
    attr.type_ = nfs3::ftype3::NF3REG;
    attr.mode = 0755;
    attr.nlink = 3;
    attr.uid = 65534;
    attr.gid = 65534;
    attr.size = 102400;
    attr.used = 102400;
    attr.fsid = 99;
    attr.fileid = 999;
    attr.atime.seconds = 1700000000;
    attr.atime.nseconds = 500000000;
    attr.mtime.seconds = 1700000001;
    attr.mtime.nseconds = 0;
    attr.ctime.seconds = 1700000002;
    attr.ctime.nseconds = 100000000;
    auto restored = XdrRoundTrip(attr);
    ASSERT_EQ(attr.type_, restored.type_);
    ASSERT_EQ(attr.mode, restored.mode);
    ASSERT_EQ(attr.size, restored.size);
}

TEST_F(Nfs3GetattrTest, Fattr3DirectoryRoundTrip) {
    nfs3::fattr3 attr;
    attr.type_ = nfs3::ftype3::NF3DIR;
    attr.mode = 0555;
    attr.nlink = 15;
    attr.uid = 0;
    attr.gid = 0;
    attr.size = 4096;
    attr.used = 4096;
    attr.fsid = 99;
    attr.fileid = 2;
    auto restored = XdrRoundTrip(attr);
    ASSERT_EQ(attr.type_, restored.type_);
    ASSERT_EQ(attr.nlink, restored.nlink);
}
