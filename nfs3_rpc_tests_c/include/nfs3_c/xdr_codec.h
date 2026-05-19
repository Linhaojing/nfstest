#ifndef NFS3_C_XDR_CODEC_H
#define NFS3_C_XDR_CODEC_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t* data;
    size_t   size;
    size_t   capacity;
    size_t   pos;
} xdr_buf_t;

void xdr_buf_init(xdr_buf_t* buf);
void xdr_buf_init_capacity(xdr_buf_t* buf, size_t cap);
void xdr_buf_init_copy(xdr_buf_t* buf, const uint8_t* data, size_t len);
void xdr_buf_destroy(xdr_buf_t* buf);
void xdr_buf_reset_read(xdr_buf_t* buf);
void xdr_buf_clear(xdr_buf_t* buf);

void xdr_pack_int32(xdr_buf_t* buf, int32_t val);
void xdr_pack_uint32(xdr_buf_t* buf, uint32_t val);
void xdr_pack_uint64(xdr_buf_t* buf, uint64_t val);
void xdr_pack_bool(xdr_buf_t* buf, int val);
void xdr_pack_cstring(xdr_buf_t* buf, const char* s);
void xdr_pack_opaque(xdr_buf_t* buf, const uint8_t* data, uint32_t len);
void xdr_pack_bytes(xdr_buf_t* buf, const uint8_t* data, size_t len);

int xdr_unpack_int32(xdr_buf_t* buf, int32_t* val);
int xdr_unpack_uint32(xdr_buf_t* buf, uint32_t* val);
int xdr_unpack_uint64(xdr_buf_t* buf, uint64_t* val);
int xdr_unpack_bool(xdr_buf_t* buf, int* val);
int xdr_unpack_cstring(xdr_buf_t* buf, char** s);
int xdr_unpack_opaque(xdr_buf_t* buf, uint8_t** data, uint32_t* len);

const uint8_t* xdr_buf_data(const xdr_buf_t* buf);
size_t xdr_buf_size(const xdr_buf_t* buf);
int xdr_buf_empty(const xdr_buf_t* buf);

#ifdef __cplusplus
}
#endif

#endif
