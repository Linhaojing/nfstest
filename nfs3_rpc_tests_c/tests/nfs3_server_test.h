#ifndef NFS3_SERVER_TEST_H
#define NFS3_SERVER_TEST_H

#include "nfs3_c/nfs3_types.h"
#include "nfs3_c/xdr_codec.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int nfs3_server_is_configured(void);
int nfs3_server_require_server(void);
const char* nfs3_server_skip_message(void);
int nfs3_mount_root(nfs_fh3_t* root_fh);
int nfs3_server_call(uint32_t proc, xdr_buf_t* args, uint8_t** resp, size_t* resp_len);
void nfs3_unique_name(const char* prefix, char* buffer, size_t buffer_len);
int nfs3_create_test_file(const nfs_fh3_t* dir, const char* name, nfs_fh3_t* out_fh);
int nfs3_create_test_dir(const nfs_fh3_t* dir, const char* name, nfs_fh3_t* out_fh);
int nfs3_lookup_name(const nfs_fh3_t* dir, const char* name, nfs_fh3_t* out_fh);
int nfs3_remove_test_file(const nfs_fh3_t* dir, const char* name);
int nfs3_rmdir_test_dir(const nfs_fh3_t* dir, const char* name);
int nfs3_write_data(const nfs_fh3_t* file, uint64_t offset, stable_how_t stable, const uint8_t* data, size_t len);
int nfs3_read_data(const nfs_fh3_t* file, uint64_t offset, uint32_t count, uint8_t** data, size_t* len, int* eof);
int nfs3_status_is_allowed(nfsstat3_t status, const nfsstat3_t* allowed, size_t allowed_count);
int nfs3_remove_if_test_file(const nfs_fh3_t* root_fh, const char* name);

#ifdef __cplusplus
}
#endif

#endif
