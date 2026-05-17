#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <expected>
#include <vector>
#include <cstring>
#include <arpa/inet.h>

#include "nfs3/xdr_codec.hpp"

#ifdef HAVE_LIBTIRPC
#include <rpc/rpc.h>
#endif

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
    std::expected<ResType, RpcError> call(
        uint32_t proc_num,
        const ArgsType& args
    ) {
#ifdef HAVE_LIBTIRPC
        if (!is_connected()) {
            return std::unexpected(RpcError::CONNECTION_FAILED);
        }
        
        impl_->xid_counter_++;
        
        xdr::XdrBuffer req_buf;
        req_buf.pack(args);
        auto req_bytes = req_buf.data();
        
        std::vector<char> req_data(req_bytes.begin(), req_bytes.end());
        char* req_ptr = req_data.data();
        u_int req_len = req_data.size();
        
        std::vector<char> resp_data(65536, 0);
        char* resp_ptr = resp_data.data();
        u_int resp_len = 65536;
        
        struct timeval timeout_tv;
        timeout_tv.tv_sec = impl_->timeout_.count() / 1000;
        timeout_tv.tv_usec = (impl_->timeout_.count() % 1000) * 1000;
        
        enum clnt_stat result = clnt_call(
            impl_->client_,
            proc_num,
            reinterpret_cast<xdrproc_t>(xdr_bytes),
            reinterpret_cast<char*>(&req_ptr),
            reinterpret_cast<xdrproc_t>(xdr_bytes),
            reinterpret_cast<char*>(&resp_ptr),
            timeout_tv
        );
        
        switch (result) {
            case RPC_SUCCESS: break;
            case RPC_TIMEDOUT: return std::unexpected(RpcError::TIMEOUT);
            case RPC_PROGUNAVAIL: return std::unexpected(RpcError::PROG_UNAVAIL);
            case RPC_VERSMISMATCH: return std::unexpected(RpcError::PROG_MISMATCH);
            case RPC_PROCUNAVAIL: return std::unexpected(RpcError::PROC_UNAVAIL);
            default: return std::unexpected(RpcError::UNKNOWN);
        }
        
        if constexpr (!std::is_same_v<ResType, void>) {
            std::vector<uint8_t> resp_bytes(resp_ptr, resp_ptr + resp_len);
            xdr::XdrBuffer resp_buf(resp_bytes);
            ResType response;
            resp_buf.unpack(response);
            return response;
        } else {
            return {};
        }
#else
        (void)proc_num;
        (void)args;
        return std::unexpected(RpcError::CONNECTION_FAILED);
#endif
    }
    
    template<typename ResType>
    std::expected<ResType, RpcError> call_void(uint32_t proc_num) {
#ifdef HAVE_LIBTIRPC
        if (!is_connected()) {
            return std::unexpected(RpcError::CONNECTION_FAILED);
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
            case RPC_TIMEDOUT: return std::unexpected(RpcError::TIMEOUT);
            case RPC_PROGUNAVAIL: return std::unexpected(RpcError::PROG_UNAVAIL);
            case RPC_VERSMISMATCH: return std::unexpected(RpcError::PROG_MISMATCH);
            case RPC_PROCUNAVAIL: return std::unexpected(RpcError::PROC_UNAVAIL);
            default: return std::unexpected(RpcError::UNKNOWN);
        }
        
        if constexpr (!std::is_same_v<ResType, void>) {
            ResType response;
            return response;
        } else {
            return {};
        }
#else
        (void)proc_num;
        return std::unexpected(RpcError::CONNECTION_FAILED);
#endif
    }
    
    void shutdown();
    
    bool is_connected() const;
    
private:
    RPCEndpoint() = default;
    
    std::unique_ptr<RPCEndpointImpl> impl_;
};

} 
