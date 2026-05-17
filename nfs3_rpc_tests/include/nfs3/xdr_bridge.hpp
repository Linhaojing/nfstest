#pragma once

#include <rpc/rpc.h>
#include <vector>
#include <string>
#include <cstdint>

namespace nfs3 {

struct nfs_fh3 {
    std::vector<uint8_t> data;
};

inline bool xdr_nfs_fh3(XDR* xdrs, nfs_fh3* obj) {
    u_int len = obj->data.size();
    char* ptr = len > 0 ? reinterpret_cast<char*>(obj->data.data()) : nullptr;
    if (!xdr_bytes(xdrs, &ptr, &len, 64)) return false;
    if (xdrs->x_op == XDR_DECODE) {
        obj->data.assign(ptr, ptr + len);
    }
    return true;
}

inline bool xdr_uint64(XDR* xdrs, uint64_t* val) {
    return xdr_u_hyper(xdrs, reinterpret_cast<u_quad_t*>(val));
}

inline bool xdr_uint32(XDR* xdrs, uint32_t* val) {
    return xdr_u_int(xdrs, val);
}

inline bool xdr_int32(XDR* xdrs, int32_t* val) {
    return xdr_int(xdrs, val);
}

inline bool xdr_bool_val(XDR* xdrs, bool* val) {
    bool_t b = *val;
    if (!xdr_bool(xdrs, &b)) return false;
    *val = b;
    return true;
}

inline bool xdr_opaque_data(XDR* xdrs, std::vector<uint8_t>* data, u_int maxlen) {
    u_int len = data->size();
    char* ptr = len > 0 ? reinterpret_cast<char*>(data->data()) : nullptr;
    if (!xdr_bytes(xdrs, &ptr, &len, maxlen)) return false;
    if (xdrs->x_op == XDR_DECODE) {
        data->assign(ptr, ptr + len);
    }
    return true;
}

inline bool xdr_string_val(XDR* xdrs, std::string* str, u_int maxlen) {
    char* ptr = const_cast<char*>(str->c_str());
    if (!xdr_string(xdrs, &ptr, maxlen)) return false;
    if (xdrs->x_op == XDR_DECODE) {
        *str = ptr;
    }
    return true;
}

}
