#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;
using nfs3::xdr::XdrBuffer;

class Nfs3CreateTest : public NFS3TestContext {};

template<typename T>
static T XdrRoundTrip(const T& original) {
    XdrBuffer buf;
    buf.pack(original);
    XdrBuffer back(buf.data());
    T restored;
    back.unpack(restored);
    return restored;
}

TEST_F(Nfs3CreateTest, CreateArgsUncheckedRoundTrip) {
    nfs3::CREATE3args args;
    args.where_dir.data = {0x01, 0x02, 0x03};
    args.where_name = "newfile.txt";
    args.how_mode = nfs3::createmode3::UNCHECKED;
    args.how_attributes.mode = 0644;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.where_name, restored.where_name);
    ASSERT_EQ(args.how_mode, restored.how_mode);
}

TEST_F(Nfs3CreateTest, CreateArgsGuardedRoundTrip) {
    nfs3::CREATE3args args;
    args.where_dir.data = {0x10};
    args.where_name = "guarded_file.txt";
    args.how_mode = nfs3::createmode3::GUARDED;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.how_mode, restored.how_mode);
}

TEST_F(Nfs3CreateTest, CreateArgsExclusiveRoundTrip) {
    nfs3::CREATE3args args;
    args.where_dir.data = {0x20};
    args.where_name = "exclusive_file.txt";
    args.how_mode = nfs3::createmode3::EXCLUSIVE;
    args.how_verf.data[0] = 0xAB;
    args.how_verf.data[7] = 0xCD;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.how_mode, restored.how_mode);
    ASSERT_EQ(args.how_verf.data[0], restored.how_verf.data[0]);
    ASSERT_EQ(args.how_verf.data[7], restored.how_verf.data[7]);
}

TEST_F(Nfs3CreateTest, CreateResOkRoundTrip) {
    nfs3::CREATE3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->obj_attributes.follow = true;
    res.resok->obj_attributes.attributes.type_ = nfs3::ftype3::NF3REG;
    res.resok->obj_attributes.attributes.size = 0;
    res.resok->dir_attributes.follow = true;
    res.resok->object.data = {0xAA, 0xBB, 0xCC};
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
    ASSERT_EQ(res.resok->object.data, restored.resok->object.data);
}

TEST_F(Nfs3CreateTest, CreateResExistsRoundTrip) {
    nfs3::CREATE3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_EXIST;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3CreateTest, CreateResNoEntRoundTrip) {
    nfs3::CREATE3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_NOENT;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3CreateTest, CreateResNoSpaceRoundTrip) {
    nfs3::CREATE3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_NOSPC;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}
