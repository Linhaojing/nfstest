#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace nfstest {
namespace rpc {

class TcpTransport {
public:
    TcpTransport() = default;
    ~TcpTransport();

    TcpTransport(const TcpTransport&) = delete;
    TcpTransport& operator=(const TcpTransport&) = delete;
    TcpTransport(TcpTransport&& other) noexcept;
    TcpTransport& operator=(TcpTransport&& other) noexcept;

    bool connect(const std::string& host, uint16_t port, int timeout_ms);
    void disconnect();
    bool is_connected() const;

    bool send(const std::vector<uint8_t>& message, int timeout_ms);
    bool recv(std::vector<uint8_t>& message, int timeout_ms);

private:
    int fd_ = -1;

    static constexpr uint32_t LAST_FRAG_FLAG = 0x80000000;
    static constexpr uint32_t FRAG_LEN_MASK   = 0x7FFFFFFF;

    bool write_all(const uint8_t* data, size_t len, int timeout_ms);
    bool read_all(uint8_t* data, size_t len, int timeout_ms);
    bool wait_readable(int timeout_ms);
};

} // namespace rpc
} // namespace nfstest
