#pragma once

#include "nfstest/rpc_protocol.hpp"
#include "nfstest/rpc_transport.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace nfstest {
namespace rpc {

class RpcClient {
public:
    RpcClient() = default;
    ~RpcClient();

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;
    RpcClient(RpcClient&& other) noexcept;
    RpcClient& operator=(RpcClient&& other) noexcept;

    bool connect(const std::string& host, uint16_t port, int timeout_ms = 3000);
    void disconnect();
    bool is_connected() const;

    std::pair<RpcStatus, std::vector<uint8_t>> call(
        uint32_t prog, uint32_t vers, uint32_t proc,
        const std::vector<uint8_t>& xdr_args,
        int timeout_ms);

    std::pair<RpcStatus, std::vector<uint8_t>> raw_call(
        const std::vector<uint8_t>& raw_msg,
        int timeout_ms);

private:
    TcpTransport transport_;
    uint32_t xid_ = 1;
};

} // namespace rpc
} // namespace nfstest
