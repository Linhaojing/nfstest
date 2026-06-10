#pragma once

#include <string>
#include <memory>
#include <chrono>
#include <vector>
#include <cstring>
#include <iostream>

#include "nfs3/xdr_codec.hpp"
#include "nfs3/expected.hpp"

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

} // namespace nfs3

#ifdef HAVE_LIBTIRPC

#include <rpc/rpc.h>

namespace nfs3 {

template<typename ArgType, typename ResType>
expected<ResType, RpcError> call_with_xdr(
    CLIENT* client,
    uint32_t proc_num,
    const ArgType& args,
    xdrproc_t xdr_arg_func,
    xdrproc_t xdr_res_func,
    std::chrono::milliseconds timeout
) {
    ResType response;
    memset(&response, 0, sizeof(response));

    struct timeval timeout_tv;
    timeout_tv.tv_sec = timeout.count() / 1000;
    timeout_tv.tv_usec = (timeout.count() % 1000) * 1000;

    enum clnt_stat result = clnt_call(
        client,
        proc_num,
        xdr_arg_func,
        (caddr_t)&args,
        xdr_res_func,
        (caddr_t)&response,
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

    return response;
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

} // namespace nfs3

namespace nfs3 {

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

} // namespace nfs3

#else // !HAVE_LIBTIRPC

#include "nfstest/rpc_client.hpp"

typedef void (*xdrproc_t)();

namespace nfs3 {

class RPCEndpointImpl {
public:
    nfstest::rpc::RpcClient rpc_client_;
    std::string host_;
    uint16_t port_;
    std::chrono::milliseconds timeout_;

    RPCEndpointImpl(const std::string& host, uint16_t port, std::chrono::milliseconds timeout)
        : host_(host), port_(port), timeout_(timeout) {}

    RPCEndpointImpl() {}
};

} // namespace nfs3

#endif // HAVE_LIBTIRPC

namespace nfs3 {

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
        if (!is_connected()) {
            return unexpected(RpcError::CONNECTION_FAILED);
        }

        xdr::XdrBuffer req_buf;
        req_buf.pack(args);
        auto req_data = req_buf.data();

        int timeout_ms = static_cast<int>(impl_->timeout_.count());

        auto [status, resp_body] = impl_->rpc_client_.call(
            100003, 3, proc_num, req_data, timeout_ms);

        switch (status) {
            case nfstest::rpc::RpcStatus::OK: break;
            case nfstest::rpc::RpcStatus::TIMEOUT: return unexpected(RpcError::TIMEOUT);
            case nfstest::rpc::RpcStatus::PROG_UNAVAIL: return unexpected(RpcError::PROG_UNAVAIL);
            case nfstest::rpc::RpcStatus::PROG_MISMATCH: return unexpected(RpcError::PROG_MISMATCH);
            case nfstest::rpc::RpcStatus::PROC_UNAVAIL: return unexpected(RpcError::PROC_UNAVAIL);
            case nfstest::rpc::RpcStatus::AUTH_ERROR: return unexpected(RpcError::AUTH_ERROR);
            default: return unexpected(RpcError::CONNECTION_FAILED);
        }

        if constexpr (!std::is_same_v<ResType, void>) {
            if (resp_body.empty()) {
                return unexpected(RpcError::DECODE_ERROR);
            }
            xdr::XdrBuffer resp_buf(resp_body);
            ResType response;
            try {
                resp_buf.unpack(response);
            } catch (const std::exception& e) {
                std::cerr << "XDR unpack failed: " << e.what() << std::endl;
                return unexpected(RpcError::DECODE_ERROR);
            }
            return response;
        } else {
            return {};
        }
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
        if (!is_connected()) {
            return unexpected(RpcError::CONNECTION_FAILED);
        }

        int timeout_ms = static_cast<int>(impl_->timeout_.count());
        std::vector<uint8_t> empty_args;

        auto [status, resp_body] = impl_->rpc_client_.call(
            100003, 3, proc_num, empty_args, timeout_ms);

        switch (status) {
            case nfstest::rpc::RpcStatus::OK: break;
            case nfstest::rpc::RpcStatus::TIMEOUT: return unexpected(RpcError::TIMEOUT);
            case nfstest::rpc::RpcStatus::PROG_UNAVAIL: return unexpected(RpcError::PROG_UNAVAIL);
            case nfstest::rpc::RpcStatus::PROG_MISMATCH: return unexpected(RpcError::PROG_MISMATCH);
            case nfstest::rpc::RpcStatus::PROC_UNAVAIL: return unexpected(RpcError::PROC_UNAVAIL);
            default: return unexpected(RpcError::CONNECTION_FAILED);
        }

        if constexpr (!std::is_same_v<ResType, void>) {
            ResType response;
            return response;
        } else {
            return {};
        }
#endif
    }

    template<typename ArgType, typename ResType>
    expected<ResType, RpcError> call_with_xdr(
        uint32_t proc_num,
        const ArgType& args,
        xdrproc_t xdr_arg_func,
        xdrproc_t xdr_res_func
    ) {
#ifdef HAVE_LIBTIRPC
        if (!is_connected()) {
            return unexpected(RpcError::CONNECTION_FAILED);
        }

        impl_->xid_counter_++;

        ResType response;
        memset(&response, 0, sizeof(response));

        struct timeval timeout_tv;
        timeout_tv.tv_sec = impl_->timeout_.count() / 1000;
        timeout_tv.tv_usec = (impl_->timeout_.count() % 1000) * 1000;

        enum clnt_stat result = clnt_call(
            impl_->client_,
            proc_num,
            xdr_arg_func,
            (caddr_t)&args,
            xdr_res_func,
            (caddr_t)&response,
            timeout_tv
        );

        if (result != RPC_SUCCESS) {
            std::cerr << "RPC call_with_xdr failed, proc=" << proc_num
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

        return response;
#else
        (void)xdr_arg_func;
        (void)xdr_res_func;
        return call<ResType>(proc_num, args);
#endif
    }

    void shutdown();

    bool is_connected() const;

private:
    RPCEndpoint() = default;

    std::unique_ptr<RPCEndpointImpl> impl_;
};

} // namespace nfs3
