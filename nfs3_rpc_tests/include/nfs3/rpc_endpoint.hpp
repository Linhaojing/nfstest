#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>

#include "nfs3/xdr_codec.hpp"
#include "nfs3/expected.hpp"

#ifdef HAVE_LIBTIRPC
#include <rpc/rpc.h>

namespace nfs3 {

struct xdr_nfs_data {
    char* data;
    u_int len;
    u_int max_len;
};

static bool_t xdr_nfs_opaque(XDR* xdrs, xdr_nfs_data* obj) {
    if (xdrs->x_op == XDR_ENCODE) {
        if (!xdr_opaque(xdrs, obj->data, obj->len)) {
            return FALSE;
        }
        return TRUE;
    } else if (xdrs->x_op == XDR_DECODE) {
        u_int actual_len = xdr_getpos(xdrs);
        if (actual_len > obj->max_len) {
            actual_len = obj->max_len;
        }
        if (!xdr_opaque(xdrs, obj->data, actual_len)) {
            return FALSE;
        }
        obj->len = actual_len;
        return TRUE;
    }
    return FALSE;
}

struct nfs_xdr_buffer {
    char* data;
    u_int size;
    u_int max_size;
};

static bool_t nfs_xdr_buffer_func(XDR* xdrs, nfs_xdr_buffer* obj) {
    if (xdrs->x_op == XDR_ENCODE) {
        int len = obj->size;
        if (!xdr_int(xdrs, &len)) return FALSE;
        return xdr_opaque(xdrs, obj->data, obj->size);
    } else if (xdrs->x_op == XDR_DECODE) {
        int len = 0;
        if (!xdr_int(xdrs, &len)) return FALSE;
        if (len <= 0 || len > (int)obj->max_size) return FALSE;
        if (!xdr_opaque(xdrs, obj->data, len)) return FALSE;
        obj->size = len;
        return TRUE;
    }
    return FALSE;
}

}

namespace nfs3 {

enum class RpcError {
    SUCCESS = 0,
    CONNECTION_FAILED,
    TIMEOUT,
    RPC_MISMATCH,
    PROG_UNAVAIL,
    PROG_MISMATCH,
    PROC_UNAVAIL,
    AUTH_ERROR,
    DECODE_ERROR,
    ENCODE_ERROR,
    UNKNOWN
};

#ifdef HAVE_LIBTIRPC
class RPCEndpointImpl {
public:
    CLIENT* client_ = nullptr;
    std::string host_;
    uint16_t port_;
    std::chrono::milliseconds timeout_;
    uint32_t xid_counter_ = 1;
    
    RPCEndpointImpl(const std::string& host, uint16_t port, std::chrono::milliseconds timeout)
        : host_(host), port_(port), timeout_(timeout) {}
    
    ~RPCEndpointImpl() {
        if (client_) {
            clnt_destroy(client_);
            client_ = nullptr;
        }
    }
};
#else
class RPCEndpointImpl {
public:
    bool connected_ = false;
};
#endif

class RPCEndpoint {
public:
    static RPCEndpoint create(
        const std::string& host,
        uint16_t port = 2049,
        std::chrono::milliseconds timeout = std::chrono::seconds(25)
    );
    
    ~RPCEndpoint();
    
    RPCEndpoint(const RPCEndpoint&) = delete;
    RPCEndpoint& operator=(const RPCEndpoint&) = delete;
    
    RPCEndpoint(RPCEndpoint&& other) noexcept;
    RPCEndpoint& operator=(RPCEndpoint&& other) noexcept;
    
    template<typename ResType, typename ArgsType>
    expected<ResType, RpcError> call(
        uint32_t proc_num,
        const ArgsType& args
    ) {
#ifdef HAVE_LIBTIRPC
        if (!is_connected()) {
            return unexpected(RpcError::CONNECTION_FAILED);
        }
        
        impl_->xid_counter_++;
        
        xdr::XdrBuffer req_buf;
        req_buf.pack(args);
        auto req_bytes = req_buf.data();
        
        struct timeval timeout_tv;
        timeout_tv.tv_sec = impl_->timeout_.count() / 1000;
        timeout_tv.tv_usec = (impl_->timeout_.count() % 1000) * 1000;
        
        nfs_xdr_buffer in_buf;
        in_buf.data = reinterpret_cast<char*>(const_cast<uint8_t*>(req_bytes.data()));
        in_buf.size = req_bytes.size();
        in_buf.max_size = req_bytes.size();
        
        constexpr u_int MAX_RESP = 65536;
        char* resp_data = static_cast<char*>(malloc(MAX_RESP));
        memset(resp_data, 0, MAX_RESP);
        
        nfs_xdr_buffer out_buf;
        out_buf.data = resp_data;
        out_buf.size = 0;
        out_buf.max_size = MAX_RESP;
        
        enum clnt_stat result = clnt_call(
            impl_->client_,
            proc_num,
            (xdrproc_t)nfs_xdr_buffer_func,
            (caddr_t)&in_buf,
            (xdrproc_t)nfs_xdr_buffer_func,
            (caddr_t)&out_buf,
            timeout_tv
        );
        
        if (result != RPC_SUCCESS) {
            std::cerr << "RPC call failed, proc=" << proc_num 
                      << ", status=" << static_cast<int>(result)
                      << ", msg=" << clnt_sperrno(result) << std::endl;
            free(resp_data);
        }
        
        switch (result) {
            case RPC_SUCCESS: break;
            case RPC_TIMEDOUT: return unexpected(RpcError::TIMEOUT);
            case RPC_PROGUNAVAIL: return unexpected(RpcError::PROG_UNAVAIL);
            case RPC_VERSMISMATCH: return unexpected(RpcError::PROG_MISMATCH);
            case RPC_PROCUNAVAIL: return unexpected(RpcError::PROC_UNAVAIL);
            default: return unexpected(RpcError::UNKNOWN);
        }
        
        if constexpr (!std::is_same_v<ResType, void>) {
            std::cerr << "Received " << out_buf.size << " bytes from server" << std::endl;
            std::vector<uint8_t> data(out_buf.data, out_buf.data + out_buf.size);
            free(resp_data);
            
            std::cerr << "Data (hex): ";
            for (size_t i = 0; i < std::min(data.size(), size_t(100)); ++i) {
                fprintf(stderr, "%02x ", data[i]);
                if ((i + 1) % 16 == 0) std::cerr << std::endl;
            }
            std::cerr << std::endl;
            
            if (data.empty()) {
                std::cerr << "No data received from server" << std::endl;
                return unexpected(RpcError::DECODE_ERROR);
            }
            
            xdr::XdrBuffer resp_buf(data);
            ResType response;
            try {
                resp_buf.unpack(response);
            } catch (const std::exception& e) {
                std::cerr << "XDR unpack failed: " << e.what() << std::endl;
                return unexpected(RpcError::DECODE_ERROR);
            }
            return response;
        } else {
            free(resp_data);
            return {};
        }
#else
        (void)proc_num;
        (void)args;
        return unexpected(RpcError::CONNECTION_FAILED);
#endif
    }
    
    template<typename ResType>
    expected<ResType, RpcError> call_void(uint32_t proc_num) {
#ifdef HAVE_LIBTIRPC
        if (!is_connected()) {
            return unexpected(RpcError::CONNECTION_FAILED);
        }
        
        impl_->xid_counter_++;
        
        struct timeval timeout_tv;
        timeout_tv.tv_sec = impl_->timeout_.count() / 1000;
        timeout_tv.tv_usec = (impl_->timeout_.count() % 1000) * 1000;
        
        enum clnt_stat result = clnt_call(
            impl_->client_,
            proc_num,
            reinterpret_cast<xdrproc_t>(xdr_void),
            nullptr,
            reinterpret_cast<xdrproc_t>(xdr_void),
            nullptr,
            timeout_tv
        );
        
        switch (result) {
            case RPC_SUCCESS: break;
            case RPC_TIMEDOUT: return unexpected(RpcError::TIMEOUT);
            case RPC_PROGUNAVAIL: return unexpected(RpcError::PROG_UNAVAIL);
            case RPC_VERSMISMATCH: return unexpected(RpcError::PROG_MISMATCH);
            case RPC_PROCUNAVAIL: return unexpected(RpcError::PROC_UNAVAIL);
            default: return unexpected(RpcError::UNKNOWN);
        }
        
        if constexpr (!std::is_same_v<ResType, void>) {
            ResType response;
            return response;
        } else {
            return {};
        }
#else
        (void)proc_num;
        return unexpected(RpcError::CONNECTION_FAILED);
#endif
    }
    
    void shutdown();
    
    bool is_connected() const;
    
private:
    RPCEndpoint() = default;
    
    std::unique_ptr<RPCEndpointImpl> impl_;
};

}

#endif 
