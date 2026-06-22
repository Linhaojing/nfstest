#include "nfstest/rpc_client.hpp"
#include "nfstest/rpc_client.h"
#include <cstdlib>
#include <cstring>

namespace nfstest {
namespace rpc {

RpcClient::~RpcClient() {
    disconnect();
}

RpcClient::RpcClient(RpcClient&& other) noexcept
    : transport_(std::move(other.transport_)),
      xid_(other.xid_) {
    other.xid_ = 1;
}

RpcClient& RpcClient::operator=(RpcClient&& other) noexcept {
    if (this != &other) {
        transport_ = std::move(other.transport_);
        xid_ = other.xid_;
        other.xid_ = 1;
    }
    return *this;
}

bool RpcClient::connect(const std::string& host, uint16_t port, int timeout_ms) {
    return transport_.connect(host, port, timeout_ms);
}

void RpcClient::disconnect() {
    transport_.disconnect();
    xid_ = 1;
}

bool RpcClient::is_connected() const {
    return transport_.is_connected();
}

std::pair<RpcStatus, std::vector<uint8_t>> RpcClient::raw_call(
    const std::vector<uint8_t>& raw_msg,
    int timeout_ms) {
    if (!transport_.is_connected()) {
        return {RpcStatus::CONN_ERR, {}};
    }

    if (!transport_.send(raw_msg, timeout_ms)) {
        return {RpcStatus::SEND_ERR, {}};
    }

    std::vector<uint8_t> reply_data;
    if (!transport_.recv(reply_data, timeout_ms)) {
        return {RpcStatus::RECV_ERR, {}};
    }

    return {RpcStatus::OK, std::move(reply_data)};
}

std::pair<RpcStatus, std::vector<uint8_t>> RpcClient::call(
    uint32_t prog, uint32_t vers, uint32_t proc,
    const std::vector<uint8_t>& xdr_args,
    int timeout_ms) {
    if (!transport_.is_connected()) {
        return {RpcStatus::CONN_ERR, {}};
    }

    uint32_t current_xid = xid_++;

    std::vector<uint8_t> call_msg = build_call(current_xid, prog, vers, proc, xdr_args);

    if (!transport_.send(call_msg, timeout_ms)) {
        return {RpcStatus::SEND_ERR, {}};
    }

    std::vector<uint8_t> reply_data;
    if (!transport_.recv(reply_data, timeout_ms)) {
        return {RpcStatus::RECV_ERR, {}};
    }

    std::vector<uint8_t> resp_body;
    RpcStatus status = parse_reply(reply_data.data(), reply_data.size(), current_xid, resp_body);
    return {status, std::move(resp_body)};
}

} // namespace rpc
} // namespace nfstest

extern "C" {

struct nfstest_rpc_client {
    nfstest::rpc::RpcClient client;
};

static int map_status(nfstest::rpc::RpcStatus s) {
    switch (s) {
        case nfstest::rpc::RpcStatus::OK:            return NFSTEST_RPC_OK;
        case nfstest::rpc::RpcStatus::CONN_ERR:       return NFSTEST_RPC_CONN_ERR;
        case nfstest::rpc::RpcStatus::TIMEOUT:        return NFSTEST_RPC_TIMEOUT;
        case nfstest::rpc::RpcStatus::PROG_UNAVAIL:   return NFSTEST_RPC_PROG_UNAVAIL;
        case nfstest::rpc::RpcStatus::PROG_MISMATCH:  return NFSTEST_RPC_PROG_MISMATCH;
        case nfstest::rpc::RpcStatus::PROC_UNAVAIL:   return NFSTEST_RPC_PROC_UNAVAIL;
        case nfstest::rpc::RpcStatus::GARBAGE_ARGS:   return NFSTEST_RPC_GARBAGE_ARGS;
        case nfstest::rpc::RpcStatus::AUTH_ERROR:     return NFSTEST_RPC_AUTH_ERROR;
        case nfstest::rpc::RpcStatus::PROTO_ERR:      return NFSTEST_RPC_PROTO_ERR;
        case nfstest::rpc::RpcStatus::SEND_ERR:       return NFSTEST_RPC_SEND_ERR;
        case nfstest::rpc::RpcStatus::RECV_ERR:       return NFSTEST_RPC_RECV_ERR;
        default: return NFSTEST_RPC_PROTO_ERR;
    }
}

nfstest_rpc_client_t* nfstest_rpc_connect(const char* host, uint16_t port, int timeout_ms) {
    auto* handle = new nfstest_rpc_client_t();
    if (!handle->client.connect(host, port, timeout_ms)) {
        delete handle;
        return nullptr;
    }
    return handle;
}

void nfstest_rpc_disconnect(nfstest_rpc_client_t* client) {
    if (client) {
        client->client.disconnect();
        delete client;
    }
}

int nfstest_rpc_call(nfstest_rpc_client_t* client,
                     uint32_t prog, uint32_t vers, uint32_t proc,
                     const uint8_t* args_data, size_t args_len,
                     uint8_t** resp_data, size_t* resp_len,
                     int timeout_ms) {
    if (!client || !resp_data || !resp_len || (args_len > 0 && !args_data)) {
        return NFSTEST_RPC_CONN_ERR;
    }

    std::vector<uint8_t> args;
    if (args_len > 0) {
        args.assign(args_data, args_data + args_len);
    }
    auto [status, resp] = client->client.call(prog, vers, proc, args, timeout_ms);

    if (status != nfstest::rpc::RpcStatus::OK) {
        *resp_data = nullptr;
        *resp_len = 0;
        return map_status(status);
    }

    *resp_len = resp.size();
    if (resp.empty()) {
        *resp_data = nullptr;
        return NFSTEST_RPC_OK;
    }

    *resp_data = static_cast<uint8_t*>(std::malloc(resp.size()));
    if (!*resp_data) {
        *resp_len = 0;
        return NFSTEST_RPC_CONN_ERR;
    }
    std::memcpy(*resp_data, resp.data(), resp.size());
    return NFSTEST_RPC_OK;
}

int nfstest_rpc_raw_call(nfstest_rpc_client_t* client,
                         const uint8_t* call_data, size_t call_len,
                         uint8_t** resp_data, size_t* resp_len,
                         int timeout_ms) {
    if (!client || !resp_data || !resp_len || (call_len > 0 && !call_data)) {
        return NFSTEST_RPC_CONN_ERR;
    }

    std::vector<uint8_t> args(call_data, call_data + call_len);
    auto [status, resp] = client->client.raw_call(args, timeout_ms);

    if (status != nfstest::rpc::RpcStatus::OK) {
        *resp_data = nullptr;
        *resp_len = 0;
        return map_status(status);
    }

    *resp_len = resp.size();
    if (resp.empty()) {
        *resp_data = nullptr;
        return NFSTEST_RPC_OK;
    }

    *resp_data = static_cast<uint8_t*>(std::malloc(resp.size()));
    if (!*resp_data) {
        *resp_len = 0;
        return NFSTEST_RPC_CONN_ERR;
    }
    std::memcpy(*resp_data, resp.data(), resp.size());
    return NFSTEST_RPC_OK;
}

}
