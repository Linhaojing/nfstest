#include "nfs3/rpc_endpoint.hpp"
#include "nfs3/xdr_codec.hpp"
#include "nfs3/nfs3_constants.hpp"

namespace nfs3 {

#ifdef HAVE_LIBTIRPC

RPCEndpoint RPCEndpoint::create(const std::string& host, uint16_t port, std::chrono::milliseconds timeout) {
    RPCEndpoint endpoint;
    endpoint.impl_ = std::make_unique<RPCEndpointImpl>(host, port, timeout);
    
    endpoint.impl_->client_ = clnt_create(
        host.c_str(),
        NFS_PROGRAM,
        NFS_V3,
        "tcp"
    );
    
    if (!endpoint.impl_->client_) {
        return endpoint;
    }
    
    if (timeout.count() > 0) {
        struct timeval tv;
        tv.tv_sec = timeout.count() / 1000;
        tv.tv_usec = (timeout.count() % 1000) * 1000;
        clnt_control(endpoint.impl_->client_, CLSET_TIMEOUT, &tv);
    }
    
    auth_destroy(endpoint.impl_->client_->cl_auth);
    endpoint.impl_->client_->cl_auth = authnone_create();
    
    return endpoint;
}

RPCEndpoint::~RPCEndpoint() = default;

RPCEndpoint::RPCEndpoint(RPCEndpoint&& other) noexcept 
    : impl_(std::move(other.impl_)) {}

RPCEndpoint& RPCEndpoint::operator=(RPCEndpoint&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

void RPCEndpoint::shutdown() {
    if (impl_ && impl_->client_) {
        clnt_destroy(impl_->client_);
        impl_->client_ = nullptr;
    }
}

bool RPCEndpoint::is_connected() const {
    return impl_ && impl_->client_ != nullptr;
}

#else

RPCEndpoint RPCEndpoint::create(const std::string& /*host*/, uint16_t /*port*/, std::chrono::milliseconds /*timeout*/) {
    RPCEndpoint endpoint;
    endpoint.impl_ = std::make_unique<RPCEndpointImpl>();
    return endpoint;
}

RPCEndpoint::~RPCEndpoint() = default;

RPCEndpoint::RPCEndpoint(RPCEndpoint&& other) noexcept 
    : impl_(std::move(other.impl_)) {}

RPCEndpoint& RPCEndpoint::operator=(RPCEndpoint&& other) noexcept {
    if (this != &other) {
        impl_ = std::move(other.impl_);
    }
    return *this;
}

void RPCEndpoint::shutdown() {
    impl_.reset();
}

bool RPCEndpoint::is_connected() const {
    return impl_ && impl_->connected_;
}

#endif

} 
