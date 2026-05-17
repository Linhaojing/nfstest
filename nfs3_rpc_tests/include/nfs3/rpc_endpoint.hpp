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

struct xdr_bytes_args {
    char* ptr;
    u_int len;
};

static bool_t xdr_bytes_wrapper(XDR* xdrs, xdr_bytes_args* obj) {
    char* ptr = obj->ptr;
    u_int len = obj->len;
    if (xdrs->x_op == XDR_DECODE) {
        std::cerr << "XDR_DECODE: ptr=" << (void*)ptr << ", len=" << len << std::endl;
    }
    bool_t result = xdr_bytes(xdrs, &ptr, &len, 65536);
    if (xdrs->x_op == XDR_DECODE) {
        std::cerr << "XDR_DECODE result: " << result << ", ptr=" << (void*)ptr << ", len=" << len << std::endl;
        obj->ptr = ptr;
        obj->len = len;
    }
    return result;
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
        
        xdr_bytes_args in_data;
        in_data.ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(req_bytes.data()));
        in_data.len = req_bytes.size();
        
        xdr_bytes_args out_data = {nullptr, 0};
        
        enum clnt_stat result = clnt_call(
            impl_->client_,
            proc_num,
            (xdrproc_t)xdr_bytes_wrapper,
            (caddr_t)&in_data,
            (xdrproc_t)xdr_bytes_wrapper,
            (caddr_t)&out_data,
            timeout_tv
        );
        
        if (result != RPC_SUCCESS) {
            std::cerr << "RPC call failed, proc=" << proc_num 
                      << ", status=" << static_cast<int>(result)
                      << ", msg=" << clnt_sperrno(result) << std::endl;
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
            if (out_data.ptr == nullptr || out_data.len == 0) {
                std::cerr << "No data received from server" << std::endl;
                return unexpected(RpcError::DECODE_ERROR);
            }
            std::cerr << "Received " << out_data.len << " bytes from server" << std::endl;
            std::vector<uint8_t> data(out_data.ptr, out_data.ptr + out_data.len);
            xdr::XdrBuffer resp_buf(data);
            ResType response;
            try {
                resp_buf.unpack(response);
            } catch (const std::exception& e) {
                std::cerr << "XDR unpack failed: " << e.what() << std::endl;
                free(out_data.ptr);
                return unexpected(RpcError::DECODE_ERROR);
            }
            free(out_data.ptr);
            return response;
        } else {
            if (out_data.ptr) {
                free(out_data.ptr);
            }
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
