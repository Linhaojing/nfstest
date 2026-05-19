#include "nfs3_c/nfs3_xdr.h"
#include <stdio.h>

/* --- Basic types --- */

void xdr_pack_writeverf3(xdr_buf_t* buf, const writeverf3_t* v) {
    xdr_pack_opaque(buf, v->data, 8);
}

void xdr_unpack_writeverf3(xdr_buf_t* buf, writeverf3_t* v) {
    uint8_t* d;
    uint32_t len;
    xdr_unpack_opaque(buf, &d, &len);
    memset(v->data, 0, 8);
    if (len > 0) memcpy(v->data, d, len < 8 ? len : 8);
    free(d);
}

void xdr_pack_nfs_fh3(xdr_buf_t* buf, const nfs_fh3_t* fh) {
    xdr_pack_opaque(buf, fh->data, fh->len);
}

void xdr_unpack_nfs_fh3(xdr_buf_t* buf, nfs_fh3_t* fh) {
    uint8_t* d;
    uint32_t len;
    xdr_unpack_opaque(buf, &d, &len);
    nfs_fh3_destroy(fh);
    fh->data = d;
    fh->len = len;
    fh->cap = len;
}

void xdr_pack_nfstime3(xdr_buf_t* buf, const nfstime3_t* t) {
    xdr_pack_uint32(buf, t->seconds);
    xdr_pack_uint32(buf, t->nseconds);
}

void xdr_unpack_nfstime3(xdr_buf_t* buf, nfstime3_t* t) {
    xdr_unpack_uint32(buf, &t->seconds);
    xdr_unpack_uint32(buf, &t->nseconds);
}

void xdr_pack_specdata3(xdr_buf_t* buf, const specdata3_t* s) {
    xdr_pack_uint32(buf, s->specdata1);
    xdr_pack_uint32(buf, s->specdata2);
}

void xdr_unpack_specdata3(xdr_buf_t* buf, specdata3_t* s) {
    xdr_unpack_uint32(buf, &s->specdata1);
    xdr_unpack_uint32(buf, &s->specdata2);
}

void xdr_pack_fattr3(xdr_buf_t* buf, const fattr3_t* attr) {
    xdr_pack_uint32(buf, (uint32_t)attr->type);
    xdr_pack_uint32(buf, attr->mode);
    xdr_pack_uint32(buf, attr->nlink);
    xdr_pack_uint32(buf, attr->uid);
    xdr_pack_uint32(buf, attr->gid);
    xdr_pack_uint64(buf, attr->size);
    xdr_pack_uint64(buf, attr->used);
    xdr_pack_specdata3(buf, &attr->rdev);
    xdr_pack_uint64(buf, attr->fsid);
    xdr_pack_uint64(buf, attr->fileid);
    xdr_pack_nfstime3(buf, &attr->atime);
    xdr_pack_nfstime3(buf, &attr->mtime);
    xdr_pack_nfstime3(buf, &attr->ctime);
}

void xdr_unpack_fattr3(xdr_buf_t* buf, fattr3_t* attr) {
    uint32_t type_val;
    xdr_unpack_uint32(buf, &type_val);
    attr->type = (ftype3_t)type_val;
    xdr_unpack_uint32(buf, &attr->mode);
    xdr_unpack_uint32(buf, &attr->nlink);
    xdr_unpack_uint32(buf, &attr->uid);
    xdr_unpack_uint32(buf, &attr->gid);
    xdr_unpack_uint64(buf, &attr->size);
    xdr_unpack_uint64(buf, &attr->used);
    xdr_unpack_specdata3(buf, &attr->rdev);
    xdr_unpack_uint64(buf, &attr->fsid);
    xdr_unpack_uint64(buf, &attr->fileid);
    xdr_unpack_nfstime3(buf, &attr->atime);
    xdr_unpack_nfstime3(buf, &attr->mtime);
    xdr_unpack_nfstime3(buf, &attr->ctime);
}

void xdr_pack_post_op_attr(xdr_buf_t* buf, const post_op_attr_t* a) {
    xdr_pack_bool(buf, a->follow);
    if (a->follow) {
        xdr_pack_fattr3(buf, &a->attributes);
    }
}

void xdr_unpack_post_op_attr(xdr_buf_t* buf, post_op_attr_t* a) {
    xdr_unpack_bool(buf, &a->follow);
    if (a->follow) {
        xdr_unpack_fattr3(buf, &a->attributes);
    }
}

void xdr_pack_wcc_attr(xdr_buf_t* buf, const wcc_attr_t* a) {
    xdr_pack_uint64(buf, a->size);
    xdr_pack_nfstime3(buf, &a->mtime);
    xdr_pack_nfstime3(buf, &a->ctime);
}

void xdr_unpack_wcc_attr(xdr_buf_t* buf, wcc_attr_t* a) {
    xdr_unpack_uint64(buf, &a->size);
    xdr_unpack_nfstime3(buf, &a->mtime);
    xdr_unpack_nfstime3(buf, &a->ctime);
}

void xdr_pack_pre_op_attr(xdr_buf_t* buf, const pre_op_attr_t* a) {
    xdr_pack_bool(buf, a->check);
    if (a->check) {
        xdr_pack_wcc_attr(buf, &a->attributes);
    }
}

void xdr_unpack_pre_op_attr(xdr_buf_t* buf, pre_op_attr_t* a) {
    xdr_unpack_bool(buf, &a->check);
    if (a->check) {
        xdr_unpack_wcc_attr(buf, &a->attributes);
    }
}

void xdr_pack_wcc_data(xdr_buf_t* buf, const wcc_data_t* d) {
    xdr_pack_pre_op_attr(buf, &d->before);
    xdr_pack_post_op_attr(buf, &d->after);
}

void xdr_unpack_wcc_data(xdr_buf_t* buf, wcc_data_t* d) {
    xdr_unpack_pre_op_attr(buf, &d->before);
    xdr_unpack_post_op_attr(buf, &d->after);
}

void xdr_pack_sattr3(xdr_buf_t* buf, const sattr3_t* s) {
    xdr_pack_uint32(buf, s->mode_set ? s->mode : 0xFFFFFFFFU);
    xdr_pack_uint32(buf, s->uid_set ? s->uid : 0xFFFFFFFFU);
    xdr_pack_uint32(buf, s->gid_set ? s->gid : 0xFFFFFFFFU);
    xdr_pack_bool(buf, s->size_set);
    if (s->size_set) {
        xdr_pack_uint64(buf, s->size);
    }
    xdr_pack_bool(buf, s->atime_set ? 1 : 0);
    if (s->atime_set) {
        xdr_pack_nfstime3(buf, &s->atime);
    }
    xdr_pack_bool(buf, s->mtime_set ? 1 : 0);
    if (s->mtime_set) {
        xdr_pack_nfstime3(buf, &s->mtime);
    }
}

#define SENTINEL_U32 0xFFFFFFFFU

void xdr_unpack_sattr3(xdr_buf_t* buf, sattr3_t* s) {
    uint32_t v;
    xdr_unpack_uint32(buf, &v);
    if (v != SENTINEL_U32) { s->mode_set = 1; s->mode = v; }
    else s->mode_set = 0;

    xdr_unpack_uint32(buf, &v);
    if (v != SENTINEL_U32) { s->uid_set = 1; s->uid = v; }
    else s->uid_set = 0;

    xdr_unpack_uint32(buf, &v);
    if (v != SENTINEL_U32) { s->gid_set = 1; s->gid = v; }
    else s->gid_set = 0;

    int flag;
    xdr_unpack_bool(buf, &flag);
    s->size_set = flag;
    if (flag) xdr_unpack_uint64(buf, &s->size);

    xdr_unpack_bool(buf, &flag);
    s->atime_set = flag;
    if (flag) xdr_unpack_nfstime3(buf, &s->atime);

    xdr_unpack_bool(buf, &flag);
    s->mtime_set = flag;
    if (flag) xdr_unpack_nfstime3(buf, &s->mtime);
}

void xdr_pack_createverf3(xdr_buf_t* buf, const createverf3_t* v) {
    xdr_pack_opaque(buf, v->data, 8);
}

void xdr_unpack_createverf3(xdr_buf_t* buf, createverf3_t* v) {
    uint8_t* d;
    uint32_t len;
    xdr_unpack_opaque(buf, &d, &len);
    memset(v->data, 0, 8);
    if (len > 0) memcpy(v->data, d, len < 8 ? len : 8);
    free(d);
}

/* --- GETATTR --- */

void xdr_pack_GETATTR3args(xdr_buf_t* buf, const GETATTR3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->object);
}

void xdr_unpack_GETATTR3args(xdr_buf_t* buf, GETATTR3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->object);
}

void xdr_pack_GETATTR3res(xdr_buf_t* buf, const GETATTR3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_fattr3(buf, &res->resok.obj_attributes);
    }
}

void xdr_unpack_GETATTR3res(xdr_buf_t* buf, GETATTR3res_t* res) {
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_fattr3(buf, &res->resok.obj_attributes);
    }
}

/* --- SETATTR --- */

void xdr_pack_SETATTR3args(xdr_buf_t* buf, const SETATTR3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->object);
    xdr_pack_sattr3(buf, &args->new_attributes);
    xdr_pack_bool(buf, args->has_guard);
    if (args->has_guard) {
        xdr_pack_nfs_fh3(buf, &args->guard);
    }
}

void xdr_unpack_SETATTR3args(xdr_buf_t* buf, SETATTR3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->object);
    xdr_unpack_sattr3(buf, &args->new_attributes);
    xdr_unpack_bool(buf, &args->has_guard);
    if (args->has_guard) {
        xdr_unpack_nfs_fh3(buf, &args->guard);
    }
}

void xdr_pack_SETATTR3res(xdr_buf_t* buf, const SETATTR3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_wcc_data(buf, &res->resok.obj_wcc);
    }
}

void xdr_unpack_SETATTR3res(xdr_buf_t* buf, SETATTR3res_t* res) {
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_wcc_data(buf, &res->resok.obj_wcc);
    }
}

/* --- LOOKUP --- */

void xdr_pack_entry3(xdr_buf_t* buf, const entry3_t* e) {
    xdr_pack_uint64(buf, e->fileid);
    xdr_pack_cstring(buf, e->name);
    xdr_pack_uint64(buf, e->cookie);
    xdr_pack_bool(buf, e->nextentry != NULL);
    if (e->nextentry) {
        xdr_pack_entry3(buf, e->nextentry);
    }
}

void xdr_unpack_entry3(xdr_buf_t* buf, entry3_t* e) {
    xdr_unpack_uint64(buf, &e->fileid);
    xdr_unpack_cstring(buf, &e->name);
    xdr_unpack_uint64(buf, &e->cookie);
    int has_next;
    xdr_unpack_bool(buf, &has_next);
    if (has_next) {
        e->nextentry = (entry3_t*)calloc(1, sizeof(entry3_t));
        xdr_unpack_entry3(buf, e->nextentry);
    }
}

void xdr_pack_LOOKUP3args(xdr_buf_t* buf, const LOOKUP3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->what_dir);
    xdr_pack_cstring(buf, args->what_name);
}

void xdr_unpack_LOOKUP3args(xdr_buf_t* buf, LOOKUP3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->what_dir);
    free(args->what_name);
    xdr_unpack_cstring(buf, &args->what_name);
}

void xdr_pack_LOOKUP3res(xdr_buf_t* buf, const LOOKUP3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_nfs_fh3(buf, &res->resok.object);
        xdr_pack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_pack_post_op_attr(buf, &res->resok.dir_attributes);
    }
}

void xdr_unpack_LOOKUP3res(xdr_buf_t* buf, LOOKUP3res_t* res) {
    lookup3res_destroy(res);
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_nfs_fh3(buf, &res->resok.object);
        xdr_unpack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_unpack_post_op_attr(buf, &res->resok.dir_attributes);
    }
}

/* --- READ --- */

void xdr_pack_READ3args(xdr_buf_t* buf, const READ3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->file);
    xdr_pack_uint64(buf, args->offset);
    xdr_pack_uint32(buf, args->count);
}

void xdr_unpack_READ3args(xdr_buf_t* buf, READ3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->file);
    xdr_unpack_uint64(buf, &args->offset);
    xdr_unpack_uint32(buf, &args->count);
}

void xdr_pack_READ3res(xdr_buf_t* buf, const READ3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.file_attributes);
        xdr_pack_uint64(buf, res->resok.count);
        xdr_pack_bool(buf, res->resok.eof);
        xdr_pack_opaque(buf, res->resok.data, res->resok.data_len);
    }
}

void xdr_unpack_READ3res(xdr_buf_t* buf, READ3res_t* res) {
    read3res_destroy(res);
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.file_attributes);
        xdr_unpack_uint64(buf, &res->resok.count);
        xdr_unpack_bool(buf, &res->resok.eof);
        uint8_t* d;
        uint32_t len;
        xdr_unpack_opaque(buf, &d, &len);
        res->resok.data = d;
        res->resok.data_len = len;
    }
}

/* --- WRITE --- */

void xdr_pack_WRITE3args(xdr_buf_t* buf, const WRITE3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->file);
    xdr_pack_uint64(buf, args->offset);
    xdr_pack_uint32(buf, args->count);
    xdr_pack_uint32(buf, (uint32_t)args->stable);
    xdr_pack_opaque(buf, args->data, args->count);
}

void xdr_unpack_WRITE3args(xdr_buf_t* buf, WRITE3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->file);
    xdr_unpack_uint64(buf, &args->offset);
    xdr_unpack_uint32(buf, &args->count);
    uint32_t st;
    xdr_unpack_uint32(buf, &st);
    args->stable = (stable_how_t)st;
    free(args->data);
    uint32_t len;
    xdr_unpack_opaque(buf, &args->data, &len);
    args->data_len = len;
}

void xdr_pack_WRITE3res(xdr_buf_t* buf, const WRITE3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.file_attributes);
        xdr_pack_uint32(buf, res->resok.count);
        xdr_pack_uint32(buf, (uint32_t)res->resok.committed);
        xdr_pack_writeverf3(buf, &res->resok.verf);
    }
}

void xdr_unpack_WRITE3res(xdr_buf_t* buf, WRITE3res_t* res) {
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.file_attributes);
        xdr_unpack_uint32(buf, &res->resok.count);
        uint32_t st;
        xdr_unpack_uint32(buf, &st);
        res->resok.committed = (stable_how_t)st;
        xdr_unpack_writeverf3(buf, &res->resok.verf);
    }
}

/* --- CREATE --- */

void xdr_pack_CREATE3args(xdr_buf_t* buf, const CREATE3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->where_dir);
    xdr_pack_cstring(buf, args->where_name);
    xdr_pack_uint32(buf, (uint32_t)args->how_mode);
    xdr_pack_sattr3(buf, &args->how_attributes);
    if (args->how_mode == EXCLUSIVE) {
        xdr_pack_createverf3(buf, &args->how_verf);
    }
}

void xdr_unpack_CREATE3args(xdr_buf_t* buf, CREATE3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->where_dir);
    free(args->where_name);
    xdr_unpack_cstring(buf, &args->where_name);
    uint32_t mode_val;
    xdr_unpack_uint32(buf, &mode_val);
    args->how_mode = (createmode3_t)mode_val;
    xdr_unpack_sattr3(buf, &args->how_attributes);
    if (args->how_mode == EXCLUSIVE) {
        xdr_unpack_createverf3(buf, &args->how_verf);
    }
}

void xdr_pack_CREATE3res(xdr_buf_t* buf, const CREATE3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_pack_wcc_data(buf, &res->resok.dir_wcc);
        xdr_pack_nfs_fh3(buf, &res->resok.object);
    }
}

void xdr_unpack_CREATE3res(xdr_buf_t* buf, CREATE3res_t* res) {
    create3res_destroy(res);
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_unpack_wcc_data(buf, &res->resok.dir_wcc);
        xdr_unpack_nfs_fh3(buf, &res->resok.object);
    }
}

/* --- REMOVE --- */

void xdr_pack_REMOVE3args(xdr_buf_t* buf, const REMOVE3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->object_dir);
    xdr_pack_cstring(buf, args->object_name);
}

void xdr_unpack_REMOVE3args(xdr_buf_t* buf, REMOVE3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->object_dir);
    free(args->object_name);
    xdr_unpack_cstring(buf, &args->object_name);
}

void xdr_pack_REMOVE3res(xdr_buf_t* buf, const REMOVE3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_wcc_data(buf, &res->resok.dir_wcc);
    }
}

void xdr_unpack_REMOVE3res(xdr_buf_t* buf, REMOVE3res_t* res) {
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_wcc_data(buf, &res->resok.dir_wcc);
    }
}

/* --- ACCESS --- */

void xdr_pack_ACCESS3args(xdr_buf_t* buf, const ACCESS3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->object);
    xdr_pack_uint32(buf, args->access);
}

void xdr_unpack_ACCESS3args(xdr_buf_t* buf, ACCESS3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->object);
    xdr_unpack_uint32(buf, &args->access);
}

void xdr_pack_ACCESS3res(xdr_buf_t* buf, const ACCESS3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_pack_uint32(buf, res->resok.access);
    }
}

void xdr_unpack_ACCESS3res(xdr_buf_t* buf, ACCESS3res_t* res) {
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_unpack_uint32(buf, &res->resok.access);
    }
}

/* --- READLINK --- */

void xdr_pack_READLINK3args(xdr_buf_t* buf, const READLINK3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->symlink);
}

void xdr_unpack_READLINK3args(xdr_buf_t* buf, READLINK3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->symlink);
}

void xdr_pack_READLINK3res(xdr_buf_t* buf, const READLINK3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.symlink_attributes);
        xdr_pack_cstring(buf, res->resok.data);
    }
}

void xdr_unpack_READLINK3res(xdr_buf_t* buf, READLINK3res_t* res) {
    readlink3res_destroy(res);
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.symlink_attributes);
        xdr_unpack_cstring(buf, &res->resok.data);
    }
}

/* --- MKDIR --- */

void xdr_pack_MKDIR3args(xdr_buf_t* buf, const MKDIR3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->where_dir);
    xdr_pack_cstring(buf, args->where_name);
    xdr_pack_sattr3(buf, &args->attributes);
}

void xdr_unpack_MKDIR3args(xdr_buf_t* buf, MKDIR3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->where_dir);
    free(args->where_name);
    xdr_unpack_cstring(buf, &args->where_name);
    xdr_unpack_sattr3(buf, &args->attributes);
}

void xdr_pack_MKDIR3res(xdr_buf_t* buf, const MKDIR3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_pack_wcc_data(buf, &res->resok.dir_wcc);
        xdr_pack_nfs_fh3(buf, &res->resok.object);
    }
}

void xdr_unpack_MKDIR3res(xdr_buf_t* buf, MKDIR3res_t* res) {
    mkdir3res_destroy(res);
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_unpack_wcc_data(buf, &res->resok.dir_wcc);
        xdr_unpack_nfs_fh3(buf, &res->resok.object);
    }
}

/* --- READDIR --- */

void xdr_pack_READDIR3args(xdr_buf_t* buf, const READDIR3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->dir);
    xdr_pack_uint64(buf, args->cookie);
    xdr_pack_uint64(buf, args->cookieverf);
    xdr_pack_uint32(buf, args->count);
}

void xdr_unpack_READDIR3args(xdr_buf_t* buf, READDIR3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->dir);
    xdr_unpack_uint64(buf, &args->cookie);
    xdr_unpack_uint64(buf, &args->cookieverf);
    xdr_unpack_uint32(buf, &args->count);
}

static void xdr_pack_dirlist3(xdr_buf_t* buf, const dirlist3_t* dl) {
    xdr_pack_bool(buf, dl->entries != NULL);
    if (dl->entries) {
        xdr_pack_entry3(buf, dl->entries);
    }
    xdr_pack_bool(buf, dl->eof);
}

static void xdr_unpack_dirlist3(xdr_buf_t* buf, dirlist3_t* dl) {
    int has_entries;
    xdr_unpack_bool(buf, &has_entries);
    if (has_entries) {
        dl->entries = (entry3_t*)calloc(1, sizeof(entry3_t));
        xdr_unpack_entry3(buf, dl->entries);
    }
    xdr_unpack_bool(buf, &dl->eof);
}

void xdr_pack_READDIR3res(xdr_buf_t* buf, const READDIR3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.dir_attributes);
        xdr_pack_uint64(buf, res->resok.cookieverf);
        xdr_pack_dirlist3(buf, &res->resok.reply);
    }
}

void xdr_unpack_READDIR3res(xdr_buf_t* buf, READDIR3res_t* res) {
    readdir3res_destroy(res);
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.dir_attributes);
        xdr_unpack_uint64(buf, &res->resok.cookieverf);
        xdr_unpack_dirlist3(buf, &res->resok.reply);
    }
}

/* --- FSSTAT --- */

void xdr_pack_FSSTAT3args(xdr_buf_t* buf, const FSSTAT3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->fsroot);
}

void xdr_unpack_FSSTAT3args(xdr_buf_t* buf, FSSTAT3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->fsroot);
}

void xdr_pack_FSSTAT3res(xdr_buf_t* buf, const FSSTAT3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_pack_uint64(buf, res->resok.tbytes);
        xdr_pack_uint64(buf, res->resok.fbytes);
        xdr_pack_uint64(buf, res->resok.abytes);
        xdr_pack_uint64(buf, res->resok.tfiles);
        xdr_pack_uint64(buf, res->resok.ffiles);
        xdr_pack_uint64(buf, res->resok.afiles);
        xdr_pack_uint32(buf, res->resok.invarsec);
    }
}

void xdr_unpack_FSSTAT3res(xdr_buf_t* buf, FSSTAT3res_t* res) {
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_unpack_uint64(buf, &res->resok.tbytes);
        xdr_unpack_uint64(buf, &res->resok.fbytes);
        xdr_unpack_uint64(buf, &res->resok.abytes);
        xdr_unpack_uint64(buf, &res->resok.tfiles);
        xdr_unpack_uint64(buf, &res->resok.ffiles);
        xdr_unpack_uint64(buf, &res->resok.afiles);
        xdr_unpack_uint32(buf, &res->resok.invarsec);
    }
}

/* --- FSINFO --- */

void xdr_pack_FSINFO3args(xdr_buf_t* buf, const FSINFO3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->fsroot);
}

void xdr_unpack_FSINFO3args(xdr_buf_t* buf, FSINFO3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->fsroot);
}

void xdr_pack_FSINFO3res(xdr_buf_t* buf, const FSINFO3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_pack_uint32(buf, res->resok.rtmax);
        xdr_pack_uint32(buf, res->resok.rtpref);
        xdr_pack_uint32(buf, res->resok.rtmult);
        xdr_pack_uint32(buf, res->resok.wtmax);
        xdr_pack_uint32(buf, res->resok.wtpref);
        xdr_pack_uint32(buf, res->resok.wtmult);
        xdr_pack_uint32(buf, res->resok.dtpref);
        xdr_pack_uint64(buf, res->resok.maxfilesize);
        xdr_pack_uint32(buf, res->resok.time_delta_seconds);
        xdr_pack_uint32(buf, res->resok.time_delta_nseconds);
        xdr_pack_uint32(buf, res->resok.properties);
    }
}

void xdr_unpack_FSINFO3res(xdr_buf_t* buf, FSINFO3res_t* res) {
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.obj_attributes);
        xdr_unpack_uint32(buf, &res->resok.rtmax);
        xdr_unpack_uint32(buf, &res->resok.rtpref);
        xdr_unpack_uint32(buf, &res->resok.rtmult);
        xdr_unpack_uint32(buf, &res->resok.wtmax);
        xdr_unpack_uint32(buf, &res->resok.wtpref);
        xdr_unpack_uint32(buf, &res->resok.wtmult);
        xdr_unpack_uint32(buf, &res->resok.dtpref);
        xdr_unpack_uint64(buf, &res->resok.maxfilesize);
        xdr_unpack_uint32(buf, &res->resok.time_delta_seconds);
        xdr_unpack_uint32(buf, &res->resok.time_delta_nseconds);
        xdr_unpack_uint32(buf, &res->resok.properties);
    }
}

/* --- COMMIT --- */

void xdr_pack_COMMIT3args(xdr_buf_t* buf, const COMMIT3args_t* args) {
    xdr_pack_nfs_fh3(buf, &args->file);
    xdr_pack_uint64(buf, args->offset);
    xdr_pack_uint32(buf, args->count);
}

void xdr_unpack_COMMIT3args(xdr_buf_t* buf, COMMIT3args_t* args) {
    xdr_unpack_nfs_fh3(buf, &args->file);
    xdr_unpack_uint64(buf, &args->offset);
    xdr_unpack_uint32(buf, &args->count);
}

void xdr_pack_COMMIT3res(xdr_buf_t* buf, const COMMIT3res_t* res) {
    xdr_pack_uint32(buf, (uint32_t)res->status);
    if (res->status == NFS3_OK && res->has_resok) {
        xdr_pack_post_op_attr(buf, &res->resok.file_wcc);
        xdr_pack_writeverf3(buf, &res->resok.verf);
    }
}

void xdr_unpack_COMMIT3res(xdr_buf_t* buf, COMMIT3res_t* res) {
    uint32_t s;
    xdr_unpack_uint32(buf, &s);
    res->status = (nfsstat3_t)s;
    res->has_resok = (res->status == NFS3_OK);
    if (res->has_resok) {
        xdr_unpack_post_op_attr(buf, &res->resok.file_wcc);
        xdr_unpack_writeverf3(buf, &res->resok.verf);
    }
}
