#ifndef NFS3_C_CONSTANTS_H
#define NFS3_C_CONSTANTS_H

#include <stdint.h>

#define NFS_PROGRAM   100003U
#define NFS_V3        3U
#define NFS3_FHSIZE   64U

typedef enum {
    NFS3_OK             = 0,
    NFS3ERR_PERM        = 1,
    NFS3ERR_NOENT       = 2,
    NFS3ERR_IO          = 5,
    NFS3ERR_NXIO        = 6,
    NFS3ERR_ACCES       = 13,
    NFS3ERR_EXIST       = 17,
    NFS3ERR_XDEV        = 18,
    NFS3ERR_NODEV       = 19,
    NFS3ERR_NOTDIR      = 20,
    NFS3ERR_ISDIR       = 21,
    NFS3ERR_INVAL       = 22,
    NFS3ERR_FBIG        = 27,
    NFS3ERR_NOSPC       = 28,
    NFS3ERR_ROFS        = 30,
    NFS3ERR_MLINK       = 31,
    NFS3ERR_NAMETOOLONG = 63,
    NFS3ERR_NOTEMPTY    = 66,
    NFS3ERR_DQUOT       = 69,
    NFS3ERR_STALE       = 70,
    NFS3ERR_REMOTE      = 71,
    NFS3ERR_BADHANDLE   = 10001,
    NFS3ERR_NOT_SYNC    = 10002,
    NFS3ERR_BAD_COOKIE  = 10003,
    NFS3ERR_NOTSUPP     = 10004,
    NFS3ERR_TOOSMALL    = 10005,
    NFS3ERR_SERVERFAULT = 10006,
    NFS3ERR_BADTYPE     = 10007,
    NFS3ERR_JUKEBOX     = 10008
} nfsstat3_t;

typedef enum {
    NF3NON  = 0,
    NF3REG  = 1,
    NF3DIR  = 2,
    NF3BLK  = 3,
    NF3CHR  = 4,
    NF3LNK  = 5,
    NF3SOCK = 6,
    NF3FIFO = 7
} ftype3_t;

typedef enum {
    UNSTABLE  = 0,
    DATA_SYNC = 1,
    FILE_SYNC = 2
} stable_how_t;

typedef enum {
    NFSPROC3_NULL       = 0,
    NFSPROC3_GETATTR    = 1,
    NFSPROC3_SETATTR    = 2,
    NFSPROC3_LOOKUP     = 3,
    NFSPROC3_ACCESS     = 4,
    NFSPROC3_READLINK   = 5,
    NFSPROC3_READ       = 6,
    NFSPROC3_WRITE      = 7,
    NFSPROC3_CREATE     = 8,
    NFSPROC3_MKDIR      = 9,
    NFSPROC3_SYMLINK    = 10,
    NFSPROC3_MKNOD      = 11,
    NFSPROC3_REMOVE     = 12,
    NFSPROC3_RMDIR      = 13,
    NFSPROC3_RENAME     = 14,
    NFSPROC3_LINK       = 15,
    NFSPROC3_READDIR    = 16,
    NFSPROC3_READDIRPLUS = 17,
    NFSPROC3_FSSTAT     = 18,
    NFSPROC3_FSINFO     = 19,
    NFSPROC3_PATHCONF   = 20,
    NFSPROC3_COMMIT     = 21
} proc_num_t;

typedef enum {
    UNCHECKED = 0,
    GUARDED   = 1,
    EXCLUSIVE = 2
} createmode3_t;

typedef enum {
    NF4REG   = 1,
    NF4DIR   = 2,
    NF4BLK   = 3,
    NF4CHR   = 4,
    NF4LNK   = 5,
    NF4SOCK  = 6,
    NF4FIFO  = 7,
    NF4ATTR  = 8,
    NF4NAMED = 9
} ftype4_t;

#define NFS3_ACCESS_READ    0x0001U
#define NFS3_ACCESS_LOOKUP  0x0002U
#define NFS3_ACCESS_MODIFY  0x0004U
#define NFS3_ACCESS_EXTEND  0x0008U
#define NFS3_ACCESS_DELETE  0x0010U
#define NFS3_ACCESS_EXECUTE 0x0020U

#endif
