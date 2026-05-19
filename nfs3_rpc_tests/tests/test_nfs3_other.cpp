#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;
using nfs3::xdr::XdrBuffer;

class Nfs3OtherTest : public NFS3TestContext {};

template<typename T>
static T XdrRoundTrip(const T& original) {
    XdrBuffer buf;
    buf.pack(original);
    XdrBuffer back(buf.data());
    T restored;
    back.unpack(restored);
    return restored;
}

TEST_F(Nfs3OtherTest, SetattrArgsRoundTrip) {
    nfs3::SETATTR3args args;
    args.object.data = {0x01, 0x02};
    args.new_attributes.mode = 0644;
    args.new_attributes.size = 8192;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.object.data, restored.object.data);
}

TEST_F(Nfs3OtherTest, SetattrArgsNoChanges) {
    nfs3::SETATTR3args args;
    args.object.data = {0xFF};
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.object.data, restored.object.data);
}

TEST_F(Nfs3OtherTest, SetattrArgsGuardTime) {
    nfs3::SETATTR3args args;
    args.object.data = {0x01};
    args.guard.emplace();
    args.guard->data = {0x11, 0x22, 0x33, 0x44};
    auto restored = XdrRoundTrip(args);
    ASSERT_TRUE(restored.guard.has_value());
}

TEST_F(Nfs3OtherTest, AccessArgsRoundTrip) {
    nfs3::ACCESS3args args;
    args.object.data = {0x01, 0x02};
    args.access = 0x01 | 0x02 | 0x04;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.object.data, restored.object.data);
    ASSERT_EQ(args.access, restored.access);
}

TEST_F(Nfs3OtherTest, AccessArgsAllMask) {
    nfs3::ACCESS3args args;
    args.object.data = {0xFF};
    args.access = 0xFFFFFFFF;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.access, restored.access);
}

TEST_F(Nfs3OtherTest, ReadlinkArgsRoundTrip) {
    nfs3::READLINK3args args;
    args.symlink.data = {0x01, 0x02, 0x03};
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.symlink.data, restored.symlink.data);
}

TEST_F(Nfs3OtherTest, ReadlinkResOkRoundTrip) {
    nfs3::READLINK3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->symlink_attributes.follow = true;
    res.resok->data = "/some/symlink/target/path";
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
    ASSERT_EQ(res.resok->data, restored.resok->data);
}

TEST_F(Nfs3OtherTest, MkdirArgsRoundTrip) {
    nfs3::MKDIR3args args;
    args.where_dir.data = {0x01};
    args.where_name = "newdir";
    args.attributes.mode = 0755;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.where_name, restored.where_name);
}

TEST_F(Nfs3OtherTest, MkdirResOkRoundTrip) {
    nfs3::MKDIR3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->obj_attributes.follow = true;
    res.resok->obj_attributes.attributes.type_ = nfs3::ftype3::NF3DIR;
    res.resok->dir_wcc.after.follow = true;
    res.resok->object.data = {0xAA, 0xBB, 0xCC};
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
    ASSERT_EQ(res.resok->object.data, restored.resok->object.data);
}

TEST_F(Nfs3OtherTest, SymlinkArgsRoundTrip) {
    nfs3::SYMLINK3args args;
    args.where_dir.data = {0x01};
    args.where_name = "link_to_target";
    args.symlink_attributes.mode = 0777;
    args.symlink_data = "/original/file";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.where_name, restored.where_name);
    ASSERT_EQ(args.symlink_data, restored.symlink_data);
}

TEST_F(Nfs3OtherTest, RenameArgsRoundTrip) {
    nfs3::RENAME3args args;
    args.from_dir.data = {0x01};
    args.from_name = "oldname.txt";
    args.to_dir.data = {0x02};
    args.to_name = "newname.txt";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.from_name, restored.from_name);
    ASSERT_EQ(args.to_name, restored.to_name);
}

TEST_F(Nfs3OtherTest, RenameResOkRoundTrip) {
    nfs3::RENAME3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->fromdir_wcc.after.follow = true;
    res.resok->todir_wcc.after.follow = true;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3OtherTest, LinkArgsRoundTrip) {
    nfs3::LINK3args args;
    args.file.data = {0x01, 0x02};
    args.link_dir.data = {0x03};
    args.link_name = "hardlink.txt";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.file.data, restored.file.data);
    ASSERT_EQ(args.link_name, restored.link_name);
}

TEST_F(Nfs3OtherTest, LinkResOkRoundTrip) {
    nfs3::LINK3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->file_attributes.follow = true;
    res.resok->linkdir_wcc.after.follow = true;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3OtherTest, MknodArgsCharRoundTrip) {
    nfs3::MKNOD3args args;
    args.where_dir.data = {0x01};
    args.where_name = "null_dev";
    args.what_type = nfs3::ftype4::NF4CHR;
    args.what_attributes.mode = 0666;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.what_type, restored.what_type);
}

TEST_F(Nfs3OtherTest, MknodArgsBlockRoundTrip) {
    nfs3::MKNOD3args args;
    args.where_dir.data = {0x01};
    args.where_name = "sda1";
    args.what_type = nfs3::ftype4::NF4BLK;
    args.what_attributes.mode = 0600;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.what_type, restored.what_type);
}
