#ifndef NFS3_C_XDR_H
#define NFS3_C_XDR_H

#include "nfs3_types.h"
#include "xdr_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- Basic types --- */
void xdr_pack_writeverf3(xdr_buf_t* buf, const writeverf3_t* v);
void xdr_unpack_writeverf3(xdr_buf_t* buf, writeverf3_t* v);

void xdr_pack_nfs_fh3(xdr_buf_t* buf, const nfs_fh3_t* fh);
void xdr_unpack_nfs_fh3(xdr_buf_t* buf, nfs_fh3_t* fh);

void xdr_pack_nfstime3(xdr_buf_t* buf, const nfstime3_t* t);
void xdr_unpack_nfstime3(xdr_buf_t* buf, nfstime3_t* t);

void xdr_pack_specdata3(xdr_buf_t* buf, const specdata3_t* s);
void xdr_unpack_specdata3(xdr_buf_t* buf, specdata3_t* s);

void xdr_pack_fattr3(xdr_buf_t* buf, const fattr3_t* attr);
void xdr_unpack_fattr3(xdr_buf_t* buf, fattr3_t* attr);

void xdr_pack_post_op_attr(xdr_buf_t* buf, const post_op_attr_t* a);
void xdr_unpack_post_op_attr(xdr_buf_t* buf, post_op_attr_t* a);

void xdr_pack_wcc_attr(xdr_buf_t* buf, const wcc_attr_t* a);
void xdr_unpack_wcc_attr(xdr_buf_t* buf, wcc_attr_t* a);

void xdr_pack_pre_op_attr(xdr_buf_t* buf, const pre_op_attr_t* a);
void xdr_unpack_pre_op_attr(xdr_buf_t* buf, pre_op_attr_t* a);

void xdr_pack_wcc_data(xdr_buf_t* buf, const wcc_data_t* d);
void xdr_unpack_wcc_data(xdr_buf_t* buf, wcc_data_t* d);

void xdr_pack_sattr3(xdr_buf_t* buf, const sattr3_t* s);
void xdr_unpack_sattr3(xdr_buf_t* buf, sattr3_t* s);

void xdr_pack_createverf3(xdr_buf_t* buf, const createverf3_t* v);
void xdr_unpack_createverf3(xdr_buf_t* buf, createverf3_t* v);

/* --- GETATTR --- */
void xdr_pack_GETATTR3args(xdr_buf_t* buf, const GETATTR3args_t* args);
void xdr_unpack_GETATTR3args(xdr_buf_t* buf, GETATTR3args_t* args);
void xdr_pack_GETATTR3res(xdr_buf_t* buf, const GETATTR3res_t* res);
void xdr_unpack_GETATTR3res(xdr_buf_t* buf, GETATTR3res_t* res);

/* --- SETATTR --- */
void xdr_pack_SETATTR3args(xdr_buf_t* buf, const SETATTR3args_t* args);
void xdr_unpack_SETATTR3args(xdr_buf_t* buf, SETATTR3args_t* args);
void xdr_pack_SETATTR3res(xdr_buf_t* buf, const SETATTR3res_t* res);
void xdr_unpack_SETATTR3res(xdr_buf_t* buf, SETATTR3res_t* res);

/* --- LOOKUP --- */
void xdr_pack_LOOKUP3args(xdr_buf_t* buf, const LOOKUP3args_t* args);
void xdr_unpack_LOOKUP3args(xdr_buf_t* buf, LOOKUP3args_t* args);
void xdr_pack_LOOKUP3res(xdr_buf_t* buf, const LOOKUP3res_t* res);
void xdr_unpack_LOOKUP3res(xdr_buf_t* buf, LOOKUP3res_t* res);

/* --- READ --- */
void xdr_pack_READ3args(xdr_buf_t* buf, const READ3args_t* args);
void xdr_unpack_READ3args(xdr_buf_t* buf, READ3args_t* args);
void xdr_pack_READ3res(xdr_buf_t* buf, const READ3res_t* res);
void xdr_unpack_READ3res(xdr_buf_t* buf, READ3res_t* res);

/* --- WRITE --- */
void xdr_pack_WRITE3args(xdr_buf_t* buf, const WRITE3args_t* args);
void xdr_unpack_WRITE3args(xdr_buf_t* buf, WRITE3args_t* args);
void xdr_pack_WRITE3res(xdr_buf_t* buf, const WRITE3res_t* res);
void xdr_unpack_WRITE3res(xdr_buf_t* buf, WRITE3res_t* res);

/* --- CREATE --- */
void xdr_pack_CREATE3args(xdr_buf_t* buf, const CREATE3args_t* args);
void xdr_unpack_CREATE3args(xdr_buf_t* buf, CREATE3args_t* args);
void xdr_pack_CREATE3res(xdr_buf_t* buf, const CREATE3res_t* res);
void xdr_unpack_CREATE3res(xdr_buf_t* buf, CREATE3res_t* res);

/* --- REMOVE --- */
void xdr_pack_REMOVE3args(xdr_buf_t* buf, const REMOVE3args_t* args);
void xdr_unpack_REMOVE3args(xdr_buf_t* buf, REMOVE3args_t* args);
void xdr_pack_REMOVE3res(xdr_buf_t* buf, const REMOVE3res_t* res);
void xdr_unpack_REMOVE3res(xdr_buf_t* buf, REMOVE3res_t* res);

/* --- ACCESS --- */
void xdr_pack_ACCESS3args(xdr_buf_t* buf, const ACCESS3args_t* args);
void xdr_unpack_ACCESS3args(xdr_buf_t* buf, ACCESS3args_t* args);
void xdr_pack_ACCESS3res(xdr_buf_t* buf, const ACCESS3res_t* res);
void xdr_unpack_ACCESS3res(xdr_buf_t* buf, ACCESS3res_t* res);

/* --- READLINK --- */
void xdr_pack_READLINK3args(xdr_buf_t* buf, const READLINK3args_t* args);
void xdr_unpack_READLINK3args(xdr_buf_t* buf, READLINK3args_t* args);
void xdr_pack_READLINK3res(xdr_buf_t* buf, const READLINK3res_t* res);
void xdr_unpack_READLINK3res(xdr_buf_t* buf, READLINK3res_t* res);

/* --- MKDIR --- */
void xdr_pack_MKDIR3args(xdr_buf_t* buf, const MKDIR3args_t* args);
void xdr_unpack_MKDIR3args(xdr_buf_t* buf, MKDIR3args_t* args);
void xdr_pack_MKDIR3res(xdr_buf_t* buf, const MKDIR3res_t* res);
void xdr_unpack_MKDIR3res(xdr_buf_t* buf, MKDIR3res_t* res);

/* --- READDIR --- */
void xdr_pack_READDIR3args(xdr_buf_t* buf, const READDIR3args_t* args);
void xdr_unpack_READDIR3args(xdr_buf_t* buf, READDIR3args_t* args);
void xdr_pack_READDIR3res(xdr_buf_t* buf, const READDIR3res_t* res);
void xdr_unpack_READDIR3res(xdr_buf_t* buf, READDIR3res_t* res);

/* --- FSSTAT --- */
void xdr_pack_FSSTAT3args(xdr_buf_t* buf, const FSSTAT3args_t* args);
void xdr_unpack_FSSTAT3args(xdr_buf_t* buf, FSSTAT3args_t* args);
void xdr_pack_FSSTAT3res(xdr_buf_t* buf, const FSSTAT3res_t* res);
void xdr_unpack_FSSTAT3res(xdr_buf_t* buf, FSSTAT3res_t* res);

/* --- FSINFO --- */
void xdr_pack_FSINFO3args(xdr_buf_t* buf, const FSINFO3args_t* args);
void xdr_unpack_FSINFO3args(xdr_buf_t* buf, FSINFO3args_t* args);
void xdr_pack_FSINFO3res(xdr_buf_t* buf, const FSINFO3res_t* res);
void xdr_unpack_FSINFO3res(xdr_buf_t* buf, FSINFO3res_t* res);

/* --- COMMIT --- */
void xdr_pack_COMMIT3args(xdr_buf_t* buf, const COMMIT3args_t* args);
void xdr_unpack_COMMIT3args(xdr_buf_t* buf, COMMIT3args_t* args);
void xdr_pack_COMMIT3res(xdr_buf_t* buf, const COMMIT3res_t* res);
void xdr_unpack_COMMIT3res(xdr_buf_t* buf, COMMIT3res_t* res);

#ifdef __cplusplus
}
#endif

#endif
