#include "nfstest/rpc_transport.hpp"
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <cstring>

namespace nfstest {
namespace rpc {

TcpTransport::~TcpTransport() {
    disconnect();
}

TcpTransport::TcpTransport(TcpTransport&& other) noexcept
    : fd_(other.fd_) {
    other.fd_ = -1;
}

TcpTransport& TcpTransport::operator=(TcpTransport&& other) noexcept {
    if (this != &other) {
        disconnect();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

bool TcpTransport::connect(const std::string& host, uint16_t port, int timeout_ms) {
    disconnect();

    fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_ < 0) return false;

    int on = 1;
    setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    struct hostent* he = gethostbyname(host.c_str());
    if (!he) {
        disconnect();
        return false;
    }
    std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

    int flags = fcntl(fd_, F_GETFL, 0);
    fcntl(fd_, F_SETFL, flags | O_NONBLOCK);

    int ret = ::connect(fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (ret < 0 && errno != EINPROGRESS) {
        disconnect();
        return false;
    }

    if (ret < 0) {
        struct pollfd pfd;
        pfd.fd = fd_;
        pfd.events = POLLOUT;
        int poll_ret = poll(&pfd, 1, timeout_ms);
        if (poll_ret <= 0) {
            disconnect();
            return false;
        }

        int err = 0;
        socklen_t len = sizeof(err);
        if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
            disconnect();
            return false;
        }
    }

    fcntl(fd_, F_SETFL, flags);

    return true;
}

void TcpTransport::disconnect() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

bool TcpTransport::is_connected() const {
    return fd_ >= 0;
}

bool TcpTransport::send(const std::vector<uint8_t>& message, int timeout_ms) {
    if (fd_ < 0) return false;

    uint32_t frag_header = htonl(static_cast<uint32_t>(message.size()) | LAST_FRAG_FLAG);

    uint8_t header[4];
    std::memcpy(header, &frag_header, 4);

    if (!write_all(header, 4, timeout_ms)) return false;
    if (!write_all(message.data(), message.size(), timeout_ms)) return false;

    return true;
}

bool TcpTransport::recv(std::vector<uint8_t>& message, int timeout_ms) {
    if (fd_ < 0) return false;

    uint8_t header[4];
    if (!read_all(header, 4, timeout_ms)) return false;

    uint32_t frag_header;
    std::memcpy(&frag_header, header, 4);
    uint32_t frag_len = ntohl(frag_header) & FRAG_LEN_MASK;

    message.resize(frag_len);
    if (frag_len > 0) {
        if (!read_all(message.data(), frag_len, timeout_ms)) return false;
    }

    return true;
}

bool TcpTransport::write_all(const uint8_t* data, size_t len, int timeout_ms) {
    size_t written = 0;
    while (written < len) {
        ssize_t n = ::write(fd_, data + written, len - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd;
                pfd.fd = fd_;
                pfd.events = POLLOUT;
                int ret = poll(&pfd, 1, timeout_ms);
                if (ret <= 0) return false;
                continue;
            }
            return false;
        }
        written += static_cast<size_t>(n);
    }
    return true;
}

bool TcpTransport::read_all(uint8_t* data, size_t len, int timeout_ms) {
    size_t readn = 0;
    while (readn < len) {
        ssize_t n = ::read(fd_, data + readn, len - readn);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                struct pollfd pfd;
                pfd.fd = fd_;
                pfd.events = POLLIN;
                int ret = poll(&pfd, 1, timeout_ms);
                if (ret <= 0) return false;
                continue;
            }
            return false;
        }
        if (n == 0) return false;
        readn += static_cast<size_t>(n);
    }
    return true;
}

bool TcpTransport::wait_readable(int timeout_ms) {
    if (fd_ < 0) return false;
    struct pollfd pfd;
    pfd.fd = fd_;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, timeout_ms);
    return ret > 0;
}

} // namespace rpc
} // namespace nfstest
