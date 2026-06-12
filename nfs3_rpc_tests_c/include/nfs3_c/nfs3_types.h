#ifndef NFS3_C_TYPES_H
#define NFS3_C_TYPES_H

#include "nfs3_constants.h"
#include "xdr_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t  data[8];
} writeverf3_t;

typedef struct {
    uint8_t* data;
    uint32_t len;
    uint32_t cap;
} nfs_fh3_t;

void nfs_fh3_init(nfs_fh3_t* fh);
void nfs_fh3_destroy(nfs_fh3_t* fh);
void nfs_fh3_set(nfs_fh3_t* fh, const uint8_t* buf, uint32_t len);
int  nfs_fh3_is_empty(const nfs_fh3_t* fh);

typedef struct {
    uint32_t seconds;
    uint32_t nseconds;
} nfstime3_t;

typedef struct {
    uint32_t specdata1;
    uint32_t specdata2;
} specdata3_t;

typedef struct {
    ftype3_t    type;
    uint32_t    mode;
    uint32_t    nlink;
    uint32_t    uid;
    uint32_t    gid;
    uint64_t    size;
    uint64_t    used;
    specdata3_t rdev;
    uint64_t    fsid;
    uint64_t    fileid;
    nfstime3_t  atime;
    nfstime3_t  mtime;
    nfstime3_t  ctime;
} fattr3_t;

void fattr3_init(fattr3_t* attr);

typedef struct {
    int       follow;
    fattr3_t  attributes;
} post_op_attr_t;

typedef struct {
    uint64_t   size;
    nfstime3_t mtime;
    nfstime3_t ctime;
} wcc_attr_t;

typedef struct {
    int        check;
    wcc_attr_t attributes;
} pre_op_attr_t;

typedef struct {
    pre_op_attr_t  before;
    post_op_attr_t after;
} wcc_data_t;

typedef struct {
    int        mode_set;
    uint32_t   mode;
    int        uid_set;
    uint32_t   uid;
    int        gid_set;
    uint32_t   gid;
    int        size_set;
    uint64_t   size;
    int        atime_set;
    nfstime3_t atime;
    int        mtime_set;
    nfstime3_t mtime;
} sattr3_t;

typedef struct {
    uint8_t data[8];
} createverf3_t;

void sattr3_init(sattr3_t* s);

/* GETATTR */
typedef struct {
    nfs_fh3_t object;
} GETATTR3args_t;

typedef struct {
    fattr3_t obj_attributes;
} GETATTR3resok_t;

typedef struct {
    nfsstat3_t  status;
    int         has_resok;
    GETATTR3resok_t resok;
} GETATTR3res_t;

/* SETATTR */
typedef struct {
    nfs_fh3_t object;
    sattr3_t  new_attributes;
    int       has_guard;
    nfs_fh3_t guard;
} SETATTR3args_t;

typedef struct {
    wcc_data_t obj_wcc;
} SETATTR3resok_t;

typedef struct {
    nfsstat3_t      status;
    int             has_resok;
    SETATTR3resok_t resok;
} SETATTR3res_t;

/* LOOKUP */
typedef struct {
    nfs_fh3_t what_dir;
    char*     what_name;
} LOOKUP3args_t;

void lookup3args_init(LOOKUP3args_t* args);
void lookup3args_destroy(LOOKUP3args_t* args);
void lookup3args_set_name(LOOKUP3args_t* args, const char* name);

typedef struct {
    nfs_fh3_t      object;
    post_op_attr_t obj_attributes;
    post_op_attr_t dir_attributes;
} LOOKUP3resok_t;

void lookup3resok_init(LOOKUP3resok_t* res);
void lookup3resok_destroy(LOOKUP3resok_t* res);

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    LOOKUP3resok_t resok;
} LOOKUP3res_t;

void lookup3res_init(LOOKUP3res_t* res);
void lookup3res_destroy(LOOKUP3res_t* res);

/* READ */
typedef struct {
    nfs_fh3_t file;
    uint64_t  offset;
    uint32_t  count;
} READ3args_t;

typedef struct {
    post_op_attr_t file_attributes;
    uint32_t       count;
    int            eof;
    uint8_t*       data;
    uint32_t       data_len;
} READ3resok_t;

void read3resok_init(READ3resok_t* res);
void read3resok_destroy(READ3resok_t* res);

typedef struct {
    nfsstat3_t  status;
    int         has_resok;
    READ3resok_t resok;
} READ3res_t;

void read3res_init(READ3res_t* res);
void read3res_destroy(READ3res_t* res);

/* WRITE */
typedef struct {
    nfs_fh3_t    file;
    uint64_t     offset;
    uint32_t     count;
    stable_how_t stable;
    uint8_t*     data;
    uint32_t     data_len;
} WRITE3args_t;

void write3args_init(WRITE3args_t* args);
void write3args_destroy(WRITE3args_t* args);

typedef struct {
    wcc_data_t   file_wcc;
    uint32_t     count;
    stable_how_t committed;
    writeverf3_t verf;
} WRITE3resok_t;

typedef struct {
    wcc_data_t file_wcc;
} WRITE3resfail_t;

typedef struct {
    nfsstat3_t       status;
    int              has_resok;
    WRITE3resok_t    resok;
    WRITE3resfail_t  resfail;
} WRITE3res_t;

/* CREATE */
typedef struct {
    nfs_fh3_t     where_dir;
    char*         where_name;
    createmode3_t how_mode;
    sattr3_t      how_attributes;
    createverf3_t how_verf;
} CREATE3args_t;

void create3args_init(CREATE3args_t* args);
void create3args_destroy(CREATE3args_t* args);
void create3args_set_name(CREATE3args_t* args, const char* name);

typedef struct {
    post_op_attr_t obj_attributes;
    wcc_data_t     dir_wcc;
    nfs_fh3_t      object;
} CREATE3resok_t;

void create3resok_init(CREATE3resok_t* res);
void create3resok_destroy(CREATE3resok_t* res);

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    CREATE3resok_t resok;
} CREATE3res_t;

void create3res_init(CREATE3res_t* res);
void create3res_destroy(CREATE3res_t* res);

/* REMOVE */
typedef struct {
    nfs_fh3_t object_dir;
    char*     object_name;
} REMOVE3args_t;

void remove3args_init(REMOVE3args_t* args);
void remove3args_destroy(REMOVE3args_t* args);
void remove3args_set_name(REMOVE3args_t* args, const char* name);

typedef struct {
    wcc_data_t dir_wcc;
} REMOVE3resok_t;

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    REMOVE3resok_t resok;
} REMOVE3res_t;

/* RMDIR */
typedef struct {
    nfs_fh3_t object_dir;
    char*     object_name;
} RMDIR3args_t;

void rmdir3args_init(RMDIR3args_t* args);
void rmdir3args_destroy(RMDIR3args_t* args);
void rmdir3args_set_name(RMDIR3args_t* args, const char* name);

typedef struct {
    wcc_data_t dir_wcc;
} RMDIR3resok_t;

typedef struct {
    nfsstat3_t    status;
    int           has_resok;
    RMDIR3resok_t resok;
} RMDIR3res_t;

/* RENAME */
typedef struct {
    nfs_fh3_t from_dir;
    char*     from_name;
    nfs_fh3_t to_dir;
    char*     to_name;
} RENAME3args_t;

void rename3args_init(RENAME3args_t* args);
void rename3args_destroy(RENAME3args_t* args);
void rename3args_set_from_name(RENAME3args_t* args, const char* name);
void rename3args_set_to_name(RENAME3args_t* args, const char* name);

typedef struct {
    wcc_data_t fromdir_wcc;
    wcc_data_t todir_wcc;
} RENAME3resok_t;

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    RENAME3resok_t resok;
} RENAME3res_t;

/* LINK */
typedef struct {
    nfs_fh3_t file;
    nfs_fh3_t link_dir;
    char*     link_name;
} LINK3args_t;

void link3args_init(LINK3args_t* args);
void link3args_destroy(LINK3args_t* args);
void link3args_set_name(LINK3args_t* args, const char* name);

typedef struct {
    post_op_attr_t file_attributes;
    wcc_data_t     linkdir_wcc;
} LINK3resok_t;

typedef struct {
    nfsstat3_t  status;
    int         has_resok;
    LINK3resok_t resok;
} LINK3res_t;

/* ACCESS */
typedef struct {
    nfs_fh3_t object;
    uint32_t  access;
} ACCESS3args_t;

typedef struct {
    post_op_attr_t obj_attributes;
    uint32_t       access;
} ACCESS3resok_t;

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    ACCESS3resok_t resok;
} ACCESS3res_t;

/* READLINK */
typedef struct {
    nfs_fh3_t symlink;
} READLINK3args_t;

typedef struct {
    post_op_attr_t symlink_attributes;
    char*          data;
} READLINK3resok_t;

void readlink3resok_init(READLINK3resok_t* res);
void readlink3resok_destroy(READLINK3resok_t* res);

typedef struct {
    nfsstat3_t       status;
    int              has_resok;
    READLINK3resok_t resok;
} READLINK3res_t;

void readlink3res_init(READLINK3res_t* res);
void readlink3res_destroy(READLINK3res_t* res);

/* MKDIR */
typedef struct {
    nfs_fh3_t where_dir;
    char*     where_name;
    sattr3_t  attributes;
} MKDIR3args_t;

void mkdir3args_init(MKDIR3args_t* args);
void mkdir3args_destroy(MKDIR3args_t* args);
void mkdir3args_set_name(MKDIR3args_t* args, const char* name);

typedef struct {
    post_op_attr_t obj_attributes;
    wcc_data_t     dir_wcc;
    nfs_fh3_t      object;
} MKDIR3resok_t;

void mkdir3resok_init(MKDIR3resok_t* res);
void mkdir3resok_destroy(MKDIR3resok_t* res);

typedef struct {
    nfsstat3_t    status;
    int           has_resok;
    MKDIR3resok_t resok;
} MKDIR3res_t;

void mkdir3res_init(MKDIR3res_t* res);
void mkdir3res_destroy(MKDIR3res_t* res);

/* SYMLINK */
typedef struct {
    nfs_fh3_t where_dir;
    char*     where_name;
    sattr3_t  symlink_attributes;
    char*     symlink_data;
} SYMLINK3args_t;

void symlink3args_init(SYMLINK3args_t* args);
void symlink3args_destroy(SYMLINK3args_t* args);
void symlink3args_set_name(SYMLINK3args_t* args, const char* name);
void symlink3args_set_data(SYMLINK3args_t* args, const char* data);

typedef struct {
    post_op_attr_t obj_attributes;
    wcc_data_t     dir_wcc;
    nfs_fh3_t      object;
} SYMLINK3resok_t;

void symlink3resok_init(SYMLINK3resok_t* res);
void symlink3resok_destroy(SYMLINK3resok_t* res);

typedef struct {
    nfsstat3_t       status;
    int              has_resok;
    SYMLINK3resok_t  resok;
} SYMLINK3res_t;

void symlink3res_init(SYMLINK3res_t* res);
void symlink3res_destroy(SYMLINK3res_t* res);

/* MKNOD */
typedef struct {
    nfs_fh3_t where_dir;
    char*     where_name;
    ftype4_t  what_type;
    sattr3_t  what_attributes;
} MKNOD3args_t;

void mknod3args_init(MKNOD3args_t* args);
void mknod3args_destroy(MKNOD3args_t* args);
void mknod3args_set_name(MKNOD3args_t* args, const char* name);

typedef struct {
    post_op_attr_t obj_attributes;
    wcc_data_t     dir_wcc;
    nfs_fh3_t      object;
} MKNOD3resok_t;

void mknod3resok_init(MKNOD3resok_t* res);
void mknod3resok_destroy(MKNOD3resok_t* res);

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    MKNOD3resok_t  resok;
} MKNOD3res_t;

void mknod3res_init(MKNOD3res_t* res);
void mknod3res_destroy(MKNOD3res_t* res);

/* READDIR */
typedef struct {
    nfs_fh3_t dir;
    uint64_t  cookie;
    uint64_t  cookieverf;
    uint32_t  count;
} READDIR3args_t;

typedef struct entry3_s {
    uint64_t          fileid;
    char*             name;
    uint64_t          cookie;
    struct entry3_s*  nextentry;
} entry3_t;

void entry3_init(entry3_t* e);
void entry3_destroy(entry3_t* e);
void entry3_set_name(entry3_t* e, const char* name);

typedef struct {
    entry3_t* entries;
    int       eof;
} dirlist3_t;

void dirlist3_init(dirlist3_t* dl);
void dirlist3_destroy(dirlist3_t* dl);

typedef struct {
    post_op_attr_t dir_attributes;
    uint64_t       cookieverf;
    dirlist3_t     reply;
} READDIR3resok_t;

void readdir3resok_init(READDIR3resok_t* res);
void readdir3resok_destroy(READDIR3resok_t* res);

typedef struct {
    nfsstat3_t      status;
    int             has_resok;
    READDIR3resok_t resok;
} READDIR3res_t;

void readdir3res_init(READDIR3res_t* res);
void readdir3res_destroy(READDIR3res_t* res);

/* READDIRPLUS */
typedef struct entryplus3_s {
    uint64_t               fileid;
    char*                  name;
    uint64_t               cookie;
    nfs_fh3_t              name_handle;
    post_op_attr_t         name_attributes;
    struct entryplus3_s*   nextentry;
} entryplus3_t;

void entryplus3_init(entryplus3_t* e);
void entryplus3_destroy(entryplus3_t* e);
void entryplus3_set_name(entryplus3_t* e, const char* name);

typedef struct {
    entryplus3_t* entries;
    int           eof;
} dirlistplus3_t;

void dirlistplus3_init(dirlistplus3_t* dl);
void dirlistplus3_destroy(dirlistplus3_t* dl);

typedef struct {
    nfs_fh3_t dir;
    uint64_t  cookie;
    uint64_t  cookieverf;
    uint32_t  dircount;
    uint32_t  maxcount;
} READDIRPLUS3args_t;

typedef struct {
    post_op_attr_t dir_attributes;
    uint64_t       cookieverf;
    dirlistplus3_t reply;
} READDIRPLUS3resok_t;

void readdirplus3resok_init(READDIRPLUS3resok_t* res);
void readdirplus3resok_destroy(READDIRPLUS3resok_t* res);

typedef struct {
    nfsstat3_t          status;
    int                 has_resok;
    READDIRPLUS3resok_t resok;
} READDIRPLUS3res_t;

void readdirplus3res_init(READDIRPLUS3res_t* res);
void readdirplus3res_destroy(READDIRPLUS3res_t* res);

/* FSSTAT */
typedef struct {
    nfs_fh3_t fsroot;
} FSSTAT3args_t;

typedef struct {
    post_op_attr_t obj_attributes;
    uint64_t       tbytes;
    uint64_t       fbytes;
    uint64_t       abytes;
    uint64_t       tfiles;
    uint64_t       ffiles;
    uint64_t       afiles;
    uint32_t       invarsec;
} FSSTAT3resok_t;

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    FSSTAT3resok_t resok;
} FSSTAT3res_t;

/* FSINFO */
typedef struct {
    nfs_fh3_t fsroot;
} FSINFO3args_t;

typedef struct {
    post_op_attr_t obj_attributes;
    uint32_t       rtmax;
    uint32_t       rtpref;
    uint32_t       rtmult;
    uint32_t       wtmax;
    uint32_t       wtpref;
    uint32_t       wtmult;
    uint32_t       dtpref;
    uint64_t       maxfilesize;
    uint32_t       time_delta_seconds;
    uint32_t       time_delta_nseconds;
    uint32_t       properties;
} FSINFO3resok_t;

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    FSINFO3resok_t resok;
} FSINFO3res_t;

/* PATHCONF */
typedef struct {
    nfs_fh3_t object;
} PATHCONF3args_t;

typedef struct {
    int32_t linkmax;
    int32_t name_max;
    int     no_trunc;
    int     chown_restricted;
    int     case_insensitive;
    int     case_preserving;
} pathconf3_resok_info_t;

typedef struct {
    post_op_attr_t          obj_attributes;
    pathconf3_resok_info_t  info;
} PATHCONF3resok_t;

typedef struct {
    nfsstat3_t       status;
    int              has_resok;
    PATHCONF3resok_t resok;
} PATHCONF3res_t;

/* COMMIT */
typedef struct {
    nfs_fh3_t file;
    uint64_t  offset;
    uint32_t  count;
} COMMIT3args_t;

typedef struct {
    post_op_attr_t file_wcc;
    writeverf3_t   verf;
} COMMIT3resok_t;

typedef struct {
    nfsstat3_t     status;
    int            has_resok;
    COMMIT3resok_t resok;
} COMMIT3res_t;

#ifdef __cplusplus
}
#endif

#endif
