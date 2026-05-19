#include "nfs3_c/xdr_codec.h"
#include <stdio.h>

static uint32_t swap_bytes32(uint32_t val) {
#ifdef __GNUC__
    return __builtin_bswap32(val);
#else
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x000000FF) << 24);
#endif
}

static uint64_t swap_bytes64(uint64_t val) {
#ifdef __GNUC__
    return __builtin_bswap64(val);
#else
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8)  |
           ((val & 0x00000000FF000000ULL) << 8)  |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
#endif
}

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define NATIVE_BIG_ENDIAN 1
#else
#define NATIVE_BIG_ENDIAN 0
#endif

static uint32_t host_to_network32(uint32_t val) {
    if (NATIVE_BIG_ENDIAN) return val;
    return swap_bytes32(val);
}

static uint32_t network_to_host32(uint32_t val) {
    return host_to_network32(val);
}

static uint64_t host_to_network64(uint64_t val) {
    if (NATIVE_BIG_ENDIAN) return val;
    return swap_bytes64(val);
}

static uint64_t network_to_host64(uint64_t val) {
    return host_to_network64(val);
}

#define XDR_MIN_CAPACITY 64

void xdr_buf_init(xdr_buf_t* buf) {
    memset(buf, 0, sizeof(*buf));
}

void xdr_buf_init_capacity(xdr_buf_t* buf, size_t cap) {
    memset(buf, 0, sizeof(*buf));
    if (cap < XDR_MIN_CAPACITY) cap = XDR_MIN_CAPACITY;
    buf->data = (uint8_t*)malloc(cap);
    buf->capacity = cap;
}

void xdr_buf_init_copy(xdr_buf_t* buf, const uint8_t* data, size_t len) {
    memset(buf, 0, sizeof(*buf));
    buf->data = (uint8_t*)malloc(len ? len : 1);
    memcpy(buf->data, data, len);
    buf->size = len;
    buf->capacity = len;
}

void xdr_buf_destroy(xdr_buf_t* buf) {
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
    buf->capacity = 0;
    buf->pos = 0;
}

void xdr_buf_reset_read(xdr_buf_t* buf) {
    buf->pos = 0;
}

void xdr_buf_clear(xdr_buf_t* buf) {
    buf->size = 0;
    buf->pos = 0;
}

static void ensure_write_capacity(xdr_buf_t* buf, size_t bytes) {
    size_t needed = buf->size + bytes;
    if (needed <= buf->capacity) return;
    size_t new_cap = buf->capacity ? buf->capacity * 2 : XDR_MIN_CAPACITY;
    while (new_cap < needed) new_cap *= 2;
    uint8_t* p = (uint8_t*)realloc(buf->data, new_cap);
    if (!p) {
        fprintf(stderr, "XDR: out of memory\n");
        return;
    }
    buf->data = p;
    buf->capacity = new_cap;
}

static void pad_to_4_bytes(xdr_buf_t* buf) {
    size_t pad = (4 - (buf->size % 4)) % 4;
    if (pad) {
        ensure_write_capacity(buf, pad);
        memset(buf->data + buf->size, 0, pad);
        buf->size += pad;
    }
}

static int ensure_read_space(const xdr_buf_t* buf, size_t bytes) {
    return (buf->pos + bytes <= buf->size);
}

void xdr_pack_int32(xdr_buf_t* buf, int32_t val) {
    xdr_pack_uint32(buf, (uint32_t)val);
}

void xdr_pack_uint32(xdr_buf_t* buf, uint32_t val) {
    ensure_write_capacity(buf, 4);
    uint32_t nv = host_to_network32(val);
    memcpy(buf->data + buf->size, &nv, 4);
    buf->size += 4;
}

void xdr_pack_uint64(xdr_buf_t* buf, uint64_t val) {
    ensure_write_capacity(buf, 8);
    uint64_t nv = host_to_network64(val);
    memcpy(buf->data + buf->size, &nv, 8);
    buf->size += 8;
}

void xdr_pack_bool(xdr_buf_t* buf, int val) {
    xdr_pack_uint32(buf, val ? 1 : 0);
}

void xdr_pack_cstring(xdr_buf_t* buf, const char* s) {
    uint32_t len = s ? (uint32_t)strlen(s) : 0;
    xdr_pack_uint32(buf, len);
    if (len > 0) {
        ensure_write_capacity(buf, len);
        memcpy(buf->data + buf->size, s, len);
        buf->size += len;
    }
    pad_to_4_bytes(buf);
}

void xdr_pack_opaque(xdr_buf_t* buf, const uint8_t* data, uint32_t len) {
    xdr_pack_uint32(buf, len);
    if (len > 0 && data) {
        ensure_write_capacity(buf, len);
        memcpy(buf->data + buf->size, data, len);
        buf->size += len;
    }
    pad_to_4_bytes(buf);
}

void xdr_pack_bytes(xdr_buf_t* buf, const uint8_t* data, size_t len) {
    if (len > 0 && data) {
        ensure_write_capacity(buf, len);
        memcpy(buf->data + buf->size, data, len);
        buf->size += len;
    }
}

int xdr_unpack_int32(xdr_buf_t* buf, int32_t* val) {
    uint32_t u;
    if (!xdr_unpack_uint32(buf, &u)) return 0;
    *val = (int32_t)u;
    return 1;
}

int xdr_unpack_uint32(xdr_buf_t* buf, uint32_t* val) {
    if (!ensure_read_space(buf, 4)) return 0;
    uint32_t nv;
    memcpy(&nv, buf->data + buf->pos, 4);
    buf->pos += 4;
    *val = network_to_host32(nv);
    return 1;
}

int xdr_unpack_uint64(xdr_buf_t* buf, uint64_t* val) {
    if (!ensure_read_space(buf, 8)) return 0;
    uint64_t nv;
    memcpy(&nv, buf->data + buf->pos, 8);
    buf->pos += 8;
    *val = network_to_host64(nv);
    return 1;
}

int xdr_unpack_bool(xdr_buf_t* buf, int* val) {
    uint32_t u;
    if (!xdr_unpack_uint32(buf, &u)) return 0;
    *val = (u != 0);
    return 1;
}

int xdr_unpack_cstring(xdr_buf_t* buf, char** s) {
    uint32_t len;
    if (!xdr_unpack_uint32(buf, &len)) return 0;
    if (!ensure_read_space(buf, len)) return 0;
    *s = (char*)malloc(len + 1);
    if (len > 0) {
        memcpy(*s, buf->data + buf->pos, len);
    }
    (*s)[len] = '\0';
    buf->pos += len;
    size_t pad = (4 - (len % 4)) % 4;
    buf->pos += pad;
    return 1;
}

int xdr_unpack_opaque(xdr_buf_t* buf, uint8_t** data, uint32_t* len) {
    if (!xdr_unpack_uint32(buf, len)) return 0;
    if (!ensure_read_space(buf, *len)) return 0;
    *data = (uint8_t*)malloc(*len ? *len : 1);
    if (*len > 0) {
        memcpy(*data, buf->data + buf->pos, *len);
    }
    buf->pos += *len;
    size_t pad = (4 - (*len % 4)) % 4;
    buf->pos += pad;
    return 1;
}

const uint8_t* xdr_buf_data(const xdr_buf_t* buf) {
    return buf->data;
}

size_t xdr_buf_size(const xdr_buf_t* buf) {
    return buf->size;
}

int xdr_buf_empty(const xdr_buf_t* buf) {
    return buf->size == 0;
}
