#ifndef NFS3_XDR_FUNCTIONS_HPP
#define NFS3_XDR_FUNCTIONS_HPP

#include <rpc/rpc.h>
#include "nfs3_types.hpp"

namespace nfs3 {

bool_t xdr_nfs_fh3(XDR* xdrs, nfs_fh3* obj);
bool_t xdr_fattr3(XDR* xdrs, fattr3* obj);
bool_t xdr_nfstime3(XDR* xdrs, nfstime3* obj);
bool_t xdr_specdata3(XDR* xdrs, specdata3* obj);
bool_t xdr_writeverf3(XDR* xdrs, writeverf3* obj);
bool_t xdr_createverf3(XDR* xdrs, createverf3* obj);
bool_t xdr_post_op_attr(XDR* xdrs, post_op_attr* obj);
bool_t xdr_pre_op_attr(XDR* xdrs, pre_op_attr* obj);
bool_t xdr_wcc_attr(XDR* xdrs, wcc_attr* obj);
bool_t xdr_wcc_data(XDR* xdrs, wcc_data* obj);
bool_t xdr_sattr3(XDR* xdrs, sattr3* obj);

bool_t xdr_GETATTR3args(XDR* xdrs, GETATTR3args* obj);
bool_t xdr_GETATTR3res(XDR* xdrs, GETATTR3res* obj);

bool_t xdr_SETATTR3args(XDR* xdrs, SETATTR3args* obj);
bool_t xdr_SETATTR3res(XDR* xdrs, SETATTR3res* obj);

bool_t xdr_LOOKUP3args(XDR* xdrs, LOOKUP3args* obj);
bool_t xdr_LOOKUP3res(XDR* xdrs, LOOKUP3res* obj);

bool_t xdr_ACCESS3args(XDR* xdrs, ACCESS3args* obj);
bool_t xdr_ACCESS3res(XDR* xdrs, ACCESS3res* obj);

bool_t xdr_READLINK3args(XDR* xdrs, READLINK3args* obj);
bool_t xdr_READLINK3res(XDR* xdrs, READLINK3res* obj);

bool_t xdr_READ3args(XDR* xdrs, READ3args* obj);
bool_t xdr_READ3res(XDR* xdrs, READ3res* obj);

bool_t xdr_WRITE3args(XDR* xdrs, WRITE3args* obj);
bool_t xdr_WRITE3res(XDR* xdrs, WRITE3res* obj);

bool_t xdr_CREATE3args(XDR* xdrs, CREATE3args* obj);
bool_t xdr_CREATE3res(XDR* xdrs, CREATE3res* obj);

bool_t xdr_MKDIR3args(XDR* xdrs, MKDIR3args* obj);
bool_t xdr_MKDIR3res(XDR* xdrs, MKDIR3res* obj);

bool_t xdr_SYMLINK3args(XDR* xdrs, SYMLINK3args* obj);
bool_t xdr_SYMLINK3res(XDR* xdrs, SYMLINK3res* obj);

bool_t xdr_MKNOD3args(XDR* xdrs, MKNOD3args* obj);
bool_t xdr_MKNOD3res(XDR* xdrs, MKNOD3res* obj);

bool_t xdr_REMOVE3args(XDR* xdrs, REMOVE3args* obj);
bool_t xdr_REMOVE3res(XDR* xdrs, REMOVE3res* obj);

bool_t xdr_RMDIR3args(XDR* xdrs, RMDIR3args* obj);
bool_t xdr_RMDIR3res(XDR* xdrs, RMDIR3res* obj);

bool_t xdr_RENAME3args(XDR* xdrs, RENAME3args* obj);
bool_t xdr_RENAME3res(XDR* xdrs, RENAME3res* obj);

bool_t xdr_LINK3args(XDR* xdrs, LINK3args* obj);
bool_t xdr_LINK3res(XDR* xdrs, LINK3res* obj);

bool_t xdr_READDIR3args(XDR* xdrs, READDIR3args* obj);
bool_t xdr_READDIR3res(XDR* xdrs, READDIR3res* obj);

bool_t xdr_READDIRPLUS3args(XDR* xdrs, READDIRPLUS3args* obj);
bool_t xdr_READDIRPLUS3res(XDR* xdrs, READDIRPLUS3res* obj);

bool_t xdr_FSSTAT3args(XDR* xdrs, FSSTAT3args* obj);
bool_t xdr_FSSTAT3res(XDR* xdrs, FSSTAT3res* obj);

bool_t xdr_FSINFO3args(XDR* xdrs, FSINFO3args* obj);
bool_t xdr_FSINFO3res(XDR* xdrs, FSINFO3res* obj);

bool_t xdr_PATHCONF3args(XDR* xdrs, PATHCONF3args* obj);
bool_t xdr_PATHCONF3res(XDR* xdrs, PATHCONF3res* obj);

bool_t xdr_COMMIT3args(XDR* xdrs, COMMIT3args* obj);
bool_t xdr_COMMIT3res(XDR* xdrs, COMMIT3res* obj);

}
#endif
