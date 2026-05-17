#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;
using nfs3::xdr::XdrBuffer;

class Nfs3RemoveTest : public NFS3TestContext {};

template<typename T>
static T XdrRoundTrip(const T& original) {
    XdrBuffer buf;
    buf.pack(original);
    XdrBuffer back(buf.data());
    T restored;
    back.unpack(restored);
    return restored;
}

TEST_F(Nfs3RemoveTest, RemoveArgsRoundTrip) {
    nfs3::REMOVE3args args;
    args.object_dir.data = {0x01, 0x02};
    args.object_name = "deleteme.txt";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.object_name, restored.object_name);
}

TEST_F(Nfs3RemoveTest, RemoveArgsSpecialChars) {
    nfs3::REMOVE3args args;
    args.object_dir.data = {0xFF};
    args.object_name = "file with spaces and # special.txt";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.object_name, restored.object_name);
}

TEST_F(Nfs3RemoveTest, RemoveResOkRoundTrip) {
    nfs3::REMOVE3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->dir_wcc.after.follow = true;
    res.resok->dir_wcc.after.attributes.size = 8192;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3RemoveTest, RemoveResNoEntRoundTrip) {
    nfs3::REMOVE3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_NOENT;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3RemoveTest, RemoveResIsDirRoundTrip) {
    nfs3::REMOVE3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_ISDIR;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3RemoveTest, RmdirArgsRoundTrip) {
    nfs3::RMDIR3args args;
    args.object_dir.data = {0x01};
    args.object_name = "emptydir";
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.object_name, restored.object_name);
}

TEST_F(Nfs3RemoveTest, RmdirResOkRoundTrip) {
    nfs3::RMDIR3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->dir_wcc.after.follow = true;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}

TEST_F(Nfs3RemoveTest, RmdirResNotEmptyRoundTrip) {
    nfs3::RMDIR3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_NOTEMPTY;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}
