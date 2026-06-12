#include "test_framework.h"
#include "nfs3_c/nfs3_xdr.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const uint8_t FH_A[] = {0x01, 0x02, 0x03, 0x04};
static const uint8_t FH_B[] = {0x10, 0x20, 0x30, 0x40, 0x50};
static const uint8_t FH_C[] = {0xaa, 0xbb, 0xcc};

static void set_fh(nfs_fh3_t* fh, const uint8_t* data, uint32_t len) {
    nfs_fh3_set(fh, data, len);
}

static void assert_fh_eq(const nfs_fh3_t* actual, const uint8_t* expected, uint32_t len, const char* msg) {
    ASSERT_EQ_U32(actual->len, len, msg);
    ASSERT_MEMEQ(actual->data, expected, len, msg);
}

static fattr3_t sample_fattr(uint32_t seed) {
    fattr3_t attr;
    fattr3_init(&attr);
    attr.type = NF3REG;
    attr.mode = 0644U + seed;
    attr.nlink = 2U + seed;
    attr.uid = 1000U + seed;
    attr.gid = 100U + seed;
    attr.size = 4096ULL + seed;
    attr.used = 8192ULL + seed;
    attr.rdev.specdata1 = 11U + seed;
    attr.rdev.specdata2 = 22U + seed;
    attr.fsid = 0x100000000ULL + seed;
    attr.fileid = 0x200000000ULL + seed;
    attr.atime.seconds = 100U + seed;
    attr.atime.nseconds = 101U + seed;
    attr.mtime.seconds = 200U + seed;
    attr.mtime.nseconds = 201U + seed;
    attr.ctime.seconds = 300U + seed;
    attr.ctime.nseconds = 301U + seed;
    return attr;
}

static wcc_attr_t sample_wcc_attr(uint32_t seed) {
    wcc_attr_t attr;
    attr.size = 10000ULL + seed;
    attr.mtime.seconds = 400U + seed;
    attr.mtime.nseconds = 401U + seed;
    attr.ctime.seconds = 500U + seed;
    attr.ctime.nseconds = 501U + seed;
    return attr;
}

static sattr3_t sample_sattr_present(void) {
    sattr3_t attr;
    sattr3_init(&attr);
    attr.mode_set = 1;
    attr.mode = 0755;
    attr.uid_set = 1;
    attr.uid = 1234;
    attr.gid_set = 1;
    attr.gid = 5678;
    attr.size_set = 1;
    attr.size = 987654321ULL;
    attr.atime_set = 1;
    attr.atime.seconds = 11;
    attr.atime.nseconds = 12;
    attr.mtime_set = 1;
    attr.mtime.seconds = 21;
    attr.mtime.nseconds = 22;
    return attr;
}

static void fill_wcc(wcc_data_t* wcc, int present) {
    wcc->before.check = present;
    if (present) {
        wcc->before.attributes = sample_wcc_attr(1);
    }
    wcc->after.follow = present;
    if (present) {
        wcc->after.attributes = sample_fattr(2);
    }
}

static void assert_post_op_attr_eq(const post_op_attr_t* actual, const post_op_attr_t* expected, const char* msg) {
    ASSERT_EQ_INT(actual->follow, expected->follow, msg);
    if (expected->follow) {
        ASSERT_EQ_U32((uint32_t)actual->attributes.type, (uint32_t)expected->attributes.type, msg);
        ASSERT_EQ_U32(actual->attributes.mode, expected->attributes.mode, msg);
        ASSERT_EQ_U64(actual->attributes.size, expected->attributes.size, msg);
        ASSERT_EQ_U64(actual->attributes.fileid, expected->attributes.fileid, msg);
        ASSERT_EQ_U32(actual->attributes.mtime.seconds, expected->attributes.mtime.seconds, msg);
    }
}

static void assert_wcc_eq(const wcc_data_t* actual, const wcc_data_t* expected, const char* msg) {
    ASSERT_EQ_INT(actual->before.check, expected->before.check, msg);
    if (expected->before.check) {
        ASSERT_EQ_U64(actual->before.attributes.size, expected->before.attributes.size, msg);
        ASSERT_EQ_U32(actual->before.attributes.mtime.seconds, expected->before.attributes.mtime.seconds, msg);
        ASSERT_EQ_U32(actual->before.attributes.mtime.nseconds, expected->before.attributes.mtime.nseconds, msg);
        ASSERT_EQ_U32(actual->before.attributes.ctime.seconds, expected->before.attributes.ctime.seconds, msg);
        ASSERT_EQ_U32(actual->before.attributes.ctime.nseconds, expected->before.attributes.ctime.nseconds, msg);
    }
    assert_post_op_attr_eq(&actual->after, &expected->after, msg);
}

static void assert_sattr_eq(const sattr3_t* actual, const sattr3_t* expected, const char* msg) {
    ASSERT_EQ_INT(actual->mode_set, expected->mode_set, msg);
    ASSERT_EQ_INT(actual->uid_set, expected->uid_set, msg);
    ASSERT_EQ_INT(actual->gid_set, expected->gid_set, msg);
    ASSERT_EQ_INT(actual->size_set, expected->size_set, msg);
    ASSERT_EQ_INT(actual->atime_set, expected->atime_set, msg);
    ASSERT_EQ_INT(actual->mtime_set, expected->mtime_set, msg);
    if (expected->mode_set) ASSERT_EQ_U32(actual->mode, expected->mode, msg);
    if (expected->uid_set) ASSERT_EQ_U32(actual->uid, expected->uid, msg);
    if (expected->gid_set) ASSERT_EQ_U32(actual->gid, expected->gid, msg);
    if (expected->size_set) ASSERT_EQ_U64(actual->size, expected->size, msg);
    if (expected->atime_set) ASSERT_EQ_U32(actual->atime.seconds, expected->atime.seconds, msg);
    if (expected->mtime_set) ASSERT_EQ_U32(actual->mtime.seconds, expected->mtime.seconds, msg);
}

#define ROUNDTRIP(type, init_fn, pack_fn, unpack_fn, source, decoded) do { \
    xdr_buf_t _buf; \
    xdr_buf_init(&_buf); \
    pack_fn(&_buf, &(source)); \
    xdr_buf_reset_read(&_buf); \
    init_fn(&(decoded)); \
    unpack_fn(&_buf, &(decoded)); \
    ASSERT_EQ_U32(_buf.pos, _buf.size, "all bytes consumed"); \
    xdr_buf_destroy(&_buf); \
} while (0)

static void init_RMDIR3res(RMDIR3res_t* v) { memset(v, 0, sizeof(*v)); }
static void init_RENAME3res(RENAME3res_t* v) { memset(v, 0, sizeof(*v)); }
static void init_LINK3res(LINK3res_t* v) { memset(v, 0, sizeof(*v)); }
static void init_PATHCONF3args(PATHCONF3args_t* v) { memset(v, 0, sizeof(*v)); nfs_fh3_init(&v->object); }
static void destroy_PATHCONF3args(PATHCONF3args_t* v) { nfs_fh3_destroy(&v->object); }
static void init_PATHCONF3res(PATHCONF3res_t* v) { memset(v, 0, sizeof(*v)); }
static void init_READDIRPLUS3args(READDIRPLUS3args_t* v) { memset(v, 0, sizeof(*v)); nfs_fh3_init(&v->dir); }
static void destroy_READDIRPLUS3args(READDIRPLUS3args_t* v) { nfs_fh3_destroy(&v->dir); }

static void test_rmdir_args_res_roundtrip(void) {
    RMDIR3args_t args;
    rmdir3args_init(&args);
    set_fh(&args.object_dir, FH_A, sizeof(FH_A));
    rmdir3args_set_name(&args, "old-dir");
    RMDIR3args_t args_out;
    ROUNDTRIP(RMDIR3args_t, rmdir3args_init, xdr_pack_RMDIR3args, xdr_unpack_RMDIR3args, args, args_out);
    assert_fh_eq(&args_out.object_dir, FH_A, sizeof(FH_A), "RMDIR dir fh");
    ASSERT_STREQ(args_out.object_name, "old-dir", "RMDIR name");
    rmdir3args_destroy(&args_out);
    rmdir3args_destroy(&args);

    RMDIR3res_t ok;
    init_RMDIR3res(&ok);
    ok.status = NFS3_OK;
    ok.has_resok = 1;
    fill_wcc(&ok.resok.dir_wcc, 1);
    RMDIR3res_t ok_out;
    ROUNDTRIP(RMDIR3res_t, init_RMDIR3res, xdr_pack_RMDIR3res, xdr_unpack_RMDIR3res, ok, ok_out);
    ASSERT_EQ_U32(ok_out.status, NFS3_OK, "RMDIR success status");
    ASSERT_EQ_INT(ok_out.has_resok, 1, "RMDIR success has resok");
    assert_wcc_eq(&ok_out.resok.dir_wcc, &ok.resok.dir_wcc, "RMDIR success wcc");

    RMDIR3res_t err;
    init_RMDIR3res(&err);
    err.status = NFS3ERR_NOTEMPTY;
    RMDIR3res_t err_out;
    ROUNDTRIP(RMDIR3res_t, init_RMDIR3res, xdr_pack_RMDIR3res, xdr_unpack_RMDIR3res, err, err_out);
    ASSERT_EQ_U32(err_out.status, NFS3ERR_NOTEMPTY, "RMDIR error status");
    ASSERT_EQ_INT(err_out.has_resok, 0, "RMDIR error no resok");
    TEST_PASS();
}

static void test_rename_args_res_roundtrip(void) {
    RENAME3args_t args;
    rename3args_init(&args);
    set_fh(&args.from_dir, FH_A, sizeof(FH_A));
    set_fh(&args.to_dir, FH_B, sizeof(FH_B));
    rename3args_set_from_name(&args, "from-name");
    rename3args_set_to_name(&args, "to-name");
    RENAME3args_t args_out;
    ROUNDTRIP(RENAME3args_t, rename3args_init, xdr_pack_RENAME3args, xdr_unpack_RENAME3args, args, args_out);
    assert_fh_eq(&args_out.from_dir, FH_A, sizeof(FH_A), "RENAME from fh");
    assert_fh_eq(&args_out.to_dir, FH_B, sizeof(FH_B), "RENAME to fh");
    ASSERT_STREQ(args_out.from_name, "from-name", "RENAME from name");
    ASSERT_STREQ(args_out.to_name, "to-name", "RENAME to name");
    rename3args_destroy(&args_out);
    rename3args_destroy(&args);

    RENAME3res_t ok;
    init_RENAME3res(&ok);
    ok.status = NFS3_OK;
    ok.has_resok = 1;
    fill_wcc(&ok.resok.fromdir_wcc, 1);
    fill_wcc(&ok.resok.todir_wcc, 0);
    RENAME3res_t ok_out;
    ROUNDTRIP(RENAME3res_t, init_RENAME3res, xdr_pack_RENAME3res, xdr_unpack_RENAME3res, ok, ok_out);
    ASSERT_EQ_U32(ok_out.status, NFS3_OK, "RENAME success status");
    assert_wcc_eq(&ok_out.resok.fromdir_wcc, &ok.resok.fromdir_wcc, "RENAME from wcc present");
    assert_wcc_eq(&ok_out.resok.todir_wcc, &ok.resok.todir_wcc, "RENAME to wcc absent");

    RENAME3res_t err;
    init_RENAME3res(&err);
    err.status = NFS3ERR_EXIST;
    RENAME3res_t err_out;
    ROUNDTRIP(RENAME3res_t, init_RENAME3res, xdr_pack_RENAME3res, xdr_unpack_RENAME3res, err, err_out);
    ASSERT_EQ_U32(err_out.status, NFS3ERR_EXIST, "RENAME error status");
    ASSERT_EQ_INT(err_out.has_resok, 0, "RENAME error no resok");
    TEST_PASS();
}

static void test_link_args_res_roundtrip(void) {
    LINK3args_t args;
    link3args_init(&args);
    set_fh(&args.file, FH_A, sizeof(FH_A));
    set_fh(&args.link_dir, FH_B, sizeof(FH_B));
    link3args_set_name(&args, "hard-link");
    LINK3args_t args_out;
    ROUNDTRIP(LINK3args_t, link3args_init, xdr_pack_LINK3args, xdr_unpack_LINK3args, args, args_out);
    assert_fh_eq(&args_out.file, FH_A, sizeof(FH_A), "LINK file fh");
    assert_fh_eq(&args_out.link_dir, FH_B, sizeof(FH_B), "LINK dir fh");
    ASSERT_STREQ(args_out.link_name, "hard-link", "LINK name");
    link3args_destroy(&args_out);
    link3args_destroy(&args);

    LINK3res_t ok;
    init_LINK3res(&ok);
    ok.status = NFS3_OK;
    ok.has_resok = 1;
    ok.resok.file_attributes.follow = 1;
    ok.resok.file_attributes.attributes = sample_fattr(3);
    fill_wcc(&ok.resok.linkdir_wcc, 0);
    LINK3res_t ok_out;
    ROUNDTRIP(LINK3res_t, init_LINK3res, xdr_pack_LINK3res, xdr_unpack_LINK3res, ok, ok_out);
    ASSERT_EQ_U32(ok_out.status, NFS3_OK, "LINK success status");
    assert_post_op_attr_eq(&ok_out.resok.file_attributes, &ok.resok.file_attributes, "LINK attrs present");
    assert_wcc_eq(&ok_out.resok.linkdir_wcc, &ok.resok.linkdir_wcc, "LINK wcc absent");

    LINK3res_t err;
    init_LINK3res(&err);
    err.status = NFS3ERR_ACCES;
    LINK3res_t err_out;
    ROUNDTRIP(LINK3res_t, init_LINK3res, xdr_pack_LINK3res, xdr_unpack_LINK3res, err, err_out);
    ASSERT_EQ_U32(err_out.status, NFS3ERR_ACCES, "LINK error status");
    ASSERT_EQ_INT(err_out.has_resok, 0, "LINK error no resok");
    TEST_PASS();
}

static void test_symlink_args_res_roundtrip(void) {
    SYMLINK3args_t args;
    symlink3args_init(&args);
    set_fh(&args.where_dir, FH_A, sizeof(FH_A));
    symlink3args_set_name(&args, "link-name");
    args.symlink_attributes = sample_sattr_present();
    symlink3args_set_data(&args, "target/path");
    SYMLINK3args_t args_out;
    ROUNDTRIP(SYMLINK3args_t, symlink3args_init, xdr_pack_SYMLINK3args, xdr_unpack_SYMLINK3args, args, args_out);
    assert_fh_eq(&args_out.where_dir, FH_A, sizeof(FH_A), "SYMLINK dir fh");
    ASSERT_STREQ(args_out.where_name, "link-name", "SYMLINK name");
    ASSERT_STREQ(args_out.symlink_data, "target/path", "SYMLINK data");
    assert_sattr_eq(&args_out.symlink_attributes, &args.symlink_attributes, "SYMLINK sattr present");
    symlink3args_destroy(&args_out);
    symlink3args_destroy(&args);

    SYMLINK3res_t ok;
    symlink3res_init(&ok);
    ok.status = NFS3_OK;
    ok.has_resok = 1;
    set_fh(&ok.resok.object, FH_B, sizeof(FH_B));
    ok.resok.obj_attributes.follow = 0;
    fill_wcc(&ok.resok.dir_wcc, 1);
    SYMLINK3res_t ok_out;
    ROUNDTRIP(SYMLINK3res_t, symlink3res_init, xdr_pack_SYMLINK3res, xdr_unpack_SYMLINK3res, ok, ok_out);
    ASSERT_EQ_U32(ok_out.status, NFS3_OK, "SYMLINK success status");
    assert_fh_eq(&ok_out.resok.object, FH_B, sizeof(FH_B), "SYMLINK object present");
    ASSERT_EQ_INT(ok_out.resok.obj_attributes.follow, 0, "SYMLINK attrs absent");
    assert_wcc_eq(&ok_out.resok.dir_wcc, &ok.resok.dir_wcc, "SYMLINK wcc present");
    symlink3res_destroy(&ok_out);
    symlink3res_destroy(&ok);

    SYMLINK3res_t err;
    symlink3res_init(&err);
    err.status = NFS3ERR_EXIST;
    SYMLINK3res_t err_out;
    ROUNDTRIP(SYMLINK3res_t, symlink3res_init, xdr_pack_SYMLINK3res, xdr_unpack_SYMLINK3res, err, err_out);
    ASSERT_EQ_U32(err_out.status, NFS3ERR_EXIST, "SYMLINK error status");
    ASSERT_EQ_INT(err_out.has_resok, 0, "SYMLINK error no resok");
    symlink3res_destroy(&err_out);
    symlink3res_destroy(&err);
    TEST_PASS();
}

static void test_mknod_args_res_roundtrip(void) {
    MKNOD3args_t args;
    mknod3args_init(&args);
    set_fh(&args.where_dir, FH_A, sizeof(FH_A));
    mknod3args_set_name(&args, "node-name");
    args.what_type = NF4FIFO;
    args.what_attributes = sample_sattr_present();
    MKNOD3args_t args_out;
    ROUNDTRIP(MKNOD3args_t, mknod3args_init, xdr_pack_MKNOD3args, xdr_unpack_MKNOD3args, args, args_out);
    assert_fh_eq(&args_out.where_dir, FH_A, sizeof(FH_A), "MKNOD dir fh");
    ASSERT_STREQ(args_out.where_name, "node-name", "MKNOD name");
    ASSERT_EQ_U32(args_out.what_type, NF4FIFO, "MKNOD type");
    assert_sattr_eq(&args_out.what_attributes, &args.what_attributes, "MKNOD sattr present");
    mknod3args_destroy(&args_out);
    mknod3args_destroy(&args);

    MKNOD3res_t ok;
    mknod3res_init(&ok);
    ok.status = NFS3_OK;
    ok.has_resok = 1;
    set_fh(&ok.resok.object, FH_C, sizeof(FH_C));
    ok.resok.obj_attributes.follow = 1;
    ok.resok.obj_attributes.attributes = sample_fattr(4);
    fill_wcc(&ok.resok.dir_wcc, 0);
    MKNOD3res_t ok_out;
    ROUNDTRIP(MKNOD3res_t, mknod3res_init, xdr_pack_MKNOD3res, xdr_unpack_MKNOD3res, ok, ok_out);
    ASSERT_EQ_U32(ok_out.status, NFS3_OK, "MKNOD success status");
    assert_fh_eq(&ok_out.resok.object, FH_C, sizeof(FH_C), "MKNOD object present");
    assert_post_op_attr_eq(&ok_out.resok.obj_attributes, &ok.resok.obj_attributes, "MKNOD attrs present");
    assert_wcc_eq(&ok_out.resok.dir_wcc, &ok.resok.dir_wcc, "MKNOD wcc absent");
    mknod3res_destroy(&ok_out);
    mknod3res_destroy(&ok);

    MKNOD3res_t err;
    mknod3res_init(&err);
    err.status = NFS3ERR_NOTSUPP;
    MKNOD3res_t err_out;
    ROUNDTRIP(MKNOD3res_t, mknod3res_init, xdr_pack_MKNOD3res, xdr_unpack_MKNOD3res, err, err_out);
    ASSERT_EQ_U32(err_out.status, NFS3ERR_NOTSUPP, "MKNOD error status");
    ASSERT_EQ_INT(err_out.has_resok, 0, "MKNOD error no resok");
    mknod3res_destroy(&err_out);
    mknod3res_destroy(&err);
    TEST_PASS();
}

static void test_readdirplus_args_res_roundtrip(void) {
    READDIRPLUS3args_t args;
    init_READDIRPLUS3args(&args);
    set_fh(&args.dir, FH_A, sizeof(FH_A));
    args.cookie = 7;
    args.cookieverf = 8;
    args.dircount = 512;
    args.maxcount = 4096;
    READDIRPLUS3args_t args_out;
    ROUNDTRIP(READDIRPLUS3args_t, init_READDIRPLUS3args, xdr_pack_READDIRPLUS3args, xdr_unpack_READDIRPLUS3args, args, args_out);
    assert_fh_eq(&args_out.dir, FH_A, sizeof(FH_A), "READDIRPLUS dir fh");
    ASSERT_EQ_U64(args_out.cookie, 7, "READDIRPLUS cookie");
    ASSERT_EQ_U64(args_out.cookieverf, 8, "READDIRPLUS cookieverf");
    ASSERT_EQ_U32(args_out.dircount, 512, "READDIRPLUS dircount");
    ASSERT_EQ_U32(args_out.maxcount, 4096, "READDIRPLUS maxcount");
    destroy_READDIRPLUS3args(&args_out);
    destroy_READDIRPLUS3args(&args);

    READDIRPLUS3res_t empty;
    readdirplus3res_init(&empty);
    empty.status = NFS3_OK;
    empty.has_resok = 1;
    empty.resok.dir_attributes.follow = 0;
    empty.resok.cookieverf = 111;
    empty.resok.reply.entries = NULL;
    empty.resok.reply.eof = 1;
    READDIRPLUS3res_t empty_out;
    ROUNDTRIP(READDIRPLUS3res_t, readdirplus3res_init, xdr_pack_READDIRPLUS3res, xdr_unpack_READDIRPLUS3res, empty, empty_out);
    ASSERT_EQ_U32(empty_out.status, NFS3_OK, "READDIRPLUS empty status");
    ASSERT_EQ_INT(empty_out.resok.dir_attributes.follow, 0, "READDIRPLUS dir attrs absent");
    ASSERT_TRUE(empty_out.resok.reply.entries == NULL, "READDIRPLUS empty list");
    ASSERT_EQ_INT(empty_out.resok.reply.eof, 1, "READDIRPLUS empty eof");
    readdirplus3res_destroy(&empty_out);
    readdirplus3res_destroy(&empty);

    READDIRPLUS3res_t nonempty;
    readdirplus3res_init(&nonempty);
    nonempty.status = NFS3_OK;
    nonempty.has_resok = 1;
    nonempty.resok.dir_attributes.follow = 1;
    nonempty.resok.dir_attributes.attributes = sample_fattr(5);
    nonempty.resok.cookieverf = 222;
    nonempty.resok.reply.eof = 0;
    nonempty.resok.reply.entries = (entryplus3_t*)calloc(1, sizeof(entryplus3_t));
    entryplus3_init(nonempty.resok.reply.entries);
    nonempty.resok.reply.entries->fileid = 1001;
    entryplus3_set_name(nonempty.resok.reply.entries, "first");
    nonempty.resok.reply.entries->cookie = 2001;
    nonempty.resok.reply.entries->name_attributes.follow = 1;
    nonempty.resok.reply.entries->name_attributes.attributes = sample_fattr(6);
    set_fh(&nonempty.resok.reply.entries->name_handle, FH_B, sizeof(FH_B));
    nonempty.resok.reply.entries->nextentry = (entryplus3_t*)calloc(1, sizeof(entryplus3_t));
    entryplus3_init(nonempty.resok.reply.entries->nextentry);
    nonempty.resok.reply.entries->nextentry->fileid = 1002;
    entryplus3_set_name(nonempty.resok.reply.entries->nextentry, "second");
    nonempty.resok.reply.entries->nextentry->cookie = 2002;
    nonempty.resok.reply.entries->nextentry->name_attributes.follow = 0;
    READDIRPLUS3res_t nonempty_out;
    ROUNDTRIP(READDIRPLUS3res_t, readdirplus3res_init, xdr_pack_READDIRPLUS3res, xdr_unpack_READDIRPLUS3res, nonempty, nonempty_out);
    ASSERT_EQ_U32(nonempty_out.status, NFS3_OK, "READDIRPLUS non-empty status");
    ASSERT_EQ_INT(nonempty_out.resok.dir_attributes.follow, 1, "READDIRPLUS dir attrs present");
    ASSERT_EQ_U64(nonempty_out.resok.cookieverf, 222, "READDIRPLUS cookieverf");
    ASSERT_TRUE(nonempty_out.resok.reply.entries != NULL, "READDIRPLUS first entry present");
    ASSERT_STREQ(nonempty_out.resok.reply.entries->name, "first", "READDIRPLUS first name");
    assert_fh_eq(&nonempty_out.resok.reply.entries->name_handle, FH_B, sizeof(FH_B), "READDIRPLUS first handle present");
    ASSERT_TRUE(nonempty_out.resok.reply.entries->nextentry != NULL, "READDIRPLUS second entry present");
    ASSERT_STREQ(nonempty_out.resok.reply.entries->nextentry->name, "second", "READDIRPLUS second name");
    ASSERT_EQ_INT(nonempty_out.resok.reply.entries->nextentry->name_attributes.follow, 0, "READDIRPLUS second attrs absent");
    ASSERT_TRUE(nfs_fh3_is_empty(&nonempty_out.resok.reply.entries->nextentry->name_handle), "READDIRPLUS second handle absent");
    readdirplus3res_destroy(&nonempty_out);
    readdirplus3res_destroy(&nonempty);

    READDIRPLUS3res_t err;
    readdirplus3res_init(&err);
    err.status = NFS3ERR_BAD_COOKIE;
    READDIRPLUS3res_t err_out;
    ROUNDTRIP(READDIRPLUS3res_t, readdirplus3res_init, xdr_pack_READDIRPLUS3res, xdr_unpack_READDIRPLUS3res, err, err_out);
    ASSERT_EQ_U32(err_out.status, NFS3ERR_BAD_COOKIE, "READDIRPLUS error status");
    ASSERT_EQ_INT(err_out.has_resok, 0, "READDIRPLUS error no resok");
    readdirplus3res_destroy(&err_out);
    readdirplus3res_destroy(&err);
    TEST_PASS();
}

static void test_pathconf_args_res_roundtrip(void) {
    PATHCONF3args_t args;
    init_PATHCONF3args(&args);
    set_fh(&args.object, FH_C, sizeof(FH_C));
    PATHCONF3args_t args_out;
    ROUNDTRIP(PATHCONF3args_t, init_PATHCONF3args, xdr_pack_PATHCONF3args, xdr_unpack_PATHCONF3args, args, args_out);
    assert_fh_eq(&args_out.object, FH_C, sizeof(FH_C), "PATHCONF object fh");
    destroy_PATHCONF3args(&args_out);
    destroy_PATHCONF3args(&args);

    PATHCONF3res_t ok;
    init_PATHCONF3res(&ok);
    ok.status = NFS3_OK;
    ok.has_resok = 1;
    ok.resok.obj_attributes.follow = 1;
    ok.resok.obj_attributes.attributes = sample_fattr(7);
    ok.resok.info.linkmax = 32000;
    ok.resok.info.name_max = 255;
    ok.resok.info.no_trunc = 1;
    ok.resok.info.chown_restricted = 1;
    ok.resok.info.case_insensitive = 0;
    ok.resok.info.case_preserving = 1;
    PATHCONF3res_t ok_out;
    ROUNDTRIP(PATHCONF3res_t, init_PATHCONF3res, xdr_pack_PATHCONF3res, xdr_unpack_PATHCONF3res, ok, ok_out);
    ASSERT_EQ_U32(ok_out.status, NFS3_OK, "PATHCONF success status");
    assert_post_op_attr_eq(&ok_out.resok.obj_attributes, &ok.resok.obj_attributes, "PATHCONF attrs present");
    ASSERT_EQ_INT(ok_out.resok.info.linkmax, 32000, "PATHCONF linkmax");
    ASSERT_EQ_INT(ok_out.resok.info.name_max, 255, "PATHCONF name max");
    ASSERT_EQ_INT(ok_out.resok.info.no_trunc, 1, "PATHCONF no_trunc");
    ASSERT_EQ_INT(ok_out.resok.info.case_preserving, 1, "PATHCONF case_preserving");

    PATHCONF3res_t err;
    init_PATHCONF3res(&err);
    err.status = NFS3ERR_STALE;
    PATHCONF3res_t err_out;
    ROUNDTRIP(PATHCONF3res_t, init_PATHCONF3res, xdr_pack_PATHCONF3res, xdr_unpack_PATHCONF3res, err, err_out);
    ASSERT_EQ_U32(err_out.status, NFS3ERR_STALE, "PATHCONF error status");
    ASSERT_EQ_INT(err_out.has_resok, 0, "PATHCONF error no resok");
    TEST_PASS();
}

int main(void) {
    printf("=== NFSv3 Full XDR Roundtrip Tests (C) ===\n\n");
    RUN_TEST(test_rmdir_args_res_roundtrip);
    RUN_TEST(test_rename_args_res_roundtrip);
    RUN_TEST(test_link_args_res_roundtrip);
    RUN_TEST(test_symlink_args_res_roundtrip);
    RUN_TEST(test_mknod_args_res_roundtrip);
    RUN_TEST(test_readdirplus_args_res_roundtrip);
    RUN_TEST(test_pathconf_args_res_roundtrip);
    PRINT_SUMMARY();
    return tests_failed;
}
