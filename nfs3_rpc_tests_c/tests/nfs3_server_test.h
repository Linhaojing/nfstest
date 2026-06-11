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
int nfs3_remove_if_test_file(const nfs_fh3_t* root_fh, const char* name);

#ifdef __cplusplus
}
#endif

#endif
