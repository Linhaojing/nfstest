#include "nfs3/xdr_codec.hpp"
#include "nfs3/detail/xdr_primitive.hpp"
#include <algorithm>

namespace nfs3 {
namespace xdr {

void XdrBuffer::ensure_write_space(size_t bytes) {
    if (buffer_.size() + bytes > buffer_.capacity()) {
        buffer_.reserve(std::max(buffer_.capacity() * 2, buffer_.size() + bytes));
    }
}

void XdrBuffer::ensure_read_space(size_t bytes) const {
    if (pos_ + bytes > buffer_.size()) {
        throw XdrError("XDR read: insufficient data (need " + 
                       std::to_string(bytes) + " bytes at offset " + 
                       std::to_string(pos_) + ", have " + 
                       std::to_string(buffer_.size() - pos_) + ")");
    }
}

uint32_t XdrBuffer::host_to_network32(uint32_t val) {
    return detail::host_to_network32(val);
}

uint32_t XdrBuffer::network_to_host32(uint32_t val) {
    return detail::network_to_host32(val);
}

uint64_t XdrBuffer::host_to_network64(uint64_t val) {
    return detail::host_to_network64(val);
}

uint64_t XdrBuffer::network_to_host64(uint64_t val) {
    return detail::network_to_host64(val);
}

void XdrBuffer::pad_to_4_bytes() {
    size_t padding = (4 - (buffer_.size() % 4)) % 4;
    for (size_t i = 0; i < padding; ++i) {
        buffer_.push_back(0);
    }
}

void XdrBuffer::pack(int32_t val) {
    ensure_write_space(4);
    uint32_t net_val = host_to_network32(static_cast<uint32_t>(val));
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&net_val);
    buffer_.insert(buffer_.end(), bytes, bytes + 4);
}

void XdrBuffer::pack(uint32_t val) {
    ensure_write_space(4);
    uint32_t net_val = host_to_network32(val);
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&net_val);
    buffer_.insert(buffer_.end(), bytes, bytes + 4);
}

void XdrBuffer::pack(uint64_t val) {
    ensure_write_space(8);
    uint64_t net_val = host_to_network64(val);
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&net_val);
    buffer_.insert(buffer_.end(), bytes, bytes + 8);
}

void XdrBuffer::pack(bool val) {
    pack(static_cast<uint32_t>(val ? 1 : 0));
}

void XdrBuffer::pack(const std::string& s) {
    if (s.size() > UINT32_MAX) {
        throw XdrError("XDR string too long: " + std::to_string(s.size()));
    }
    
    pack(static_cast<uint32_t>(s.size()));
    
    ensure_write_space(s.size());
    buffer_.insert(buffer_.end(), s.begin(), s.end());
    
    pad_to_4_bytes();
}

void XdrBuffer::pack(const std::vector<uint8_t>& data) {
    pack(data.data(), data.size());
}

void XdrBuffer::pack(const uint8_t* data, size_t len) {
    if (len > UINT32_MAX) {
        throw XdrError("XDR opaque data too long: " + std::to_string(len));
    }
    
    size_t padding = (4 - (len % 4)) % 4;
    
    pack(static_cast<uint32_t>(len));
    
    ensure_write_space(len + padding);
    buffer_.insert(buffer_.end(), data, data + len);
    
    for (size_t i = 0; i < padding; ++i) {
        buffer_.push_back(0);
    }
}

void XdrBuffer::unpack(int32_t& val) {
    ensure_read_space(4);
    uint32_t net_val;
    std::memcpy(&net_val, &buffer_[pos_], 4);
    pos_ += 4;
    val = static_cast<int32_t>(network_to_host32(net_val));
}

void XdrBuffer::unpack(uint32_t& val) {
    ensure_read_space(4);
    uint32_t net_val;
    std::memcpy(&net_val, &buffer_[pos_], 4);
    pos_ += 4;
    val = network_to_host32(net_val);
}

void XdrBuffer::unpack(uint64_t& val) {
    ensure_read_space(8);
    uint64_t net_val;
    std::memcpy(&net_val, &buffer_[pos_], 8);
    pos_ += 8;
    val = network_to_host64(net_val);
}

void XdrBuffer::unpack(bool& val) {
    uint32_t int_val;
    unpack(int_val);
    val = (int_val != 0);
}

void XdrBuffer::unpack(std::string& s) {
    uint32_t len;
    unpack(len);
    
    ensure_read_space(len);
    s.assign(reinterpret_cast<const char*>(&buffer_[pos_]), len);
    pos_ += len;
    
    size_t padding = (4 - (len % 4)) % 4;
    pos_ += padding;
}

void XdrBuffer::unpack(std::vector<uint8_t>& data) {
    uint32_t len;
    unpack(len);
    
    ensure_read_space(len);
    data.assign(&buffer_[pos_], &buffer_[pos_] + len);
    pos_ += len;
    
    size_t padding = (4 - (len % 4)) % 4;
    pos_ += padding;
}

} 
}
