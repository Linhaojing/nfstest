#include "nfs3/test_context.hpp"
#include "nfs3/nfs3_types.hpp"
#include <gtest/gtest.h>

using nfs3::NFS3TestContext;
using nfs3::xdr::XdrBuffer;

class Nfs3ReadWriteTest : public NFS3TestContext {};

template<typename T>
static T XdrRoundTrip(const T& original) {
    XdrBuffer buf;
    buf.pack(original);
    XdrBuffer back(buf.data());
    T restored;
    back.unpack(restored);
    return restored;
}

TEST_F(Nfs3ReadWriteTest, ReadArgsRoundTrip) {
    nfs3::READ3args args;
    args.file.data = {0x01, 0x02, 0x03};
    args.offset = 1024;
    args.count = 4096;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.file.data, restored.file.data);
    ASSERT_EQ(args.offset, restored.offset);
    ASSERT_EQ(args.count, restored.count);
}

TEST_F(Nfs3ReadWriteTest, ReadArgsZeroOffset) {
    nfs3::READ3args args;
    args.file.data = {0xFF};
    args.offset = 0;
    args.count = 512;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.offset, restored.offset);
}

TEST_F(Nfs3ReadWriteTest, ReadArgsMaxCount) {
    nfs3::READ3args args;
    args.file.data = {0x01};
    args.offset = 0;
    args.count = 1048576;
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.count, restored.count);
}

TEST_F(Nfs3ReadWriteTest, ReadResOkRoundTrip) {
    nfs3::READ3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->file_attributes.follow = true;
    res.resok->file_attributes.attributes.type_ = nfs3::ftype3::NF3REG;
    res.resok->file_attributes.attributes.size = 8192;
    res.resok->count = 256;
    res.resok->eof = false;
    res.resok->data = {0xDE, 0xAD, 0xBE, 0xEF};
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
    ASSERT_EQ(res.resok->count, restored.resok->count);
    ASSERT_EQ(res.resok->eof, restored.resok->eof);
    ASSERT_EQ(res.resok->data, restored.resok->data);
}

TEST_F(Nfs3ReadWriteTest, ReadResEofRoundTrip) {
    nfs3::READ3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->file_attributes.follow = false;
    res.resok->count = 0;
    res.resok->eof = true;
    auto restored = XdrRoundTrip(res);
    ASSERT_TRUE(restored.resok->eof);
}

TEST_F(Nfs3ReadWriteTest, WriteArgsRoundTrip) {
    nfs3::WRITE3args args;
    args.file.data = {0x01, 0x02, 0x03};
    args.offset = 2048;
    args.stable = nfs3::stable_how::UNSTABLE;
    args.data = {'h', 'e', 'l', 'l', 'o'};
    auto restored = XdrRoundTrip(args);
    ASSERT_EQ(args.file.data, restored.file.data);
    ASSERT_EQ(args.offset, restored.offset);
    ASSERT_EQ(args.stable, restored.stable);
    ASSERT_EQ(args.data, restored.data);
}

TEST_F(Nfs3ReadWriteTest, WriteArgsStableModes) {
    for (auto mode : {nfs3::stable_how::UNSTABLE,
                      nfs3::stable_how::DATA_SYNC,
                      nfs3::stable_how::FILE_SYNC}) {
        nfs3::WRITE3args args;
        args.file.data = {0x01};
        args.offset = 0;
        args.stable = mode;
        args.data = {'x'};
        auto restored = XdrRoundTrip(args);
        ASSERT_EQ(args.stable, restored.stable);
    }
}

TEST_F(Nfs3ReadWriteTest, WriteResOkRoundTrip) {
    nfs3::WRITE3res res;
    res.status = nfs3::nfsstat3::NFS3_OK;
    res.resok.emplace();
    res.resok->file_attributes.follow = true;
    res.resok->file_attributes.attributes.size = 8192;
    res.resok->count = 1024;
    res.resok->committed = nfs3::stable_how::FILE_SYNC;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
    ASSERT_EQ(res.resok->count, restored.resok->count);
    ASSERT_EQ(res.resok->committed, restored.resok->committed);
}

TEST_F(Nfs3ReadWriteTest, WriteResErrorRoundTrip) {
    nfs3::WRITE3res res;
    res.status = nfs3::nfsstat3::NFS3ERR_ROFS;
    auto restored = XdrRoundTrip(res);
    ASSERT_EQ(res.status, restored.status);
}
