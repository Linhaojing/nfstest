#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

namespace nfs3 {
namespace xdr {

class XdrBuffer {
public:
    XdrBuffer() = default;
    
    explicit XdrBuffer(size_t initial_capacity) {
        buffer_.reserve(initial_capacity);
    }
    
    explicit XdrBuffer(const std::vector<uint8_t>& data) 
        : buffer_(data) {}
    
    explicit XdrBuffer(std::vector<uint8_t>&& data) 
        : buffer_(std::move(data)) {}

    void pack(int32_t val);
    void pack(uint32_t val);
    void pack(uint64_t val);
    void pack(bool val);
    void pack(const std::string& s);
    void pack(const std::vector<uint8_t>& data);
    void pack(const uint8_t* data, size_t len);
    
    void unpack(int32_t& val);
    void unpack(uint32_t& val);
    void unpack(uint64_t& val);
    void unpack(bool& val);
    void unpack(std::string& s);
    void unpack(std::vector<uint8_t>& data);

    template<typename T>
    void pack(const T& obj) {
        obj.serialize(*this);
    }

    template<typename T>
    void unpack(T& obj) {
        obj.deserialize(*this);
    }

    const std::vector<uint8_t>& data() const { return buffer_; }
    size_t size() const { return buffer_.size(); }
    bool empty() const { return buffer_.empty(); }
    
    void clear() {
        buffer_.clear();
        pos_ = 0;
    }
    
    void reset_read() {
        pos_ = 0;
    }

private:
    void ensure_write_space(size_t bytes);
    void ensure_read_space(size_t bytes) const;
    
    static uint32_t host_to_network32(uint32_t val);
    static uint32_t network_to_host32(uint32_t val);
    static uint64_t host_to_network64(uint64_t val);
    static uint64_t network_to_host64(uint64_t val);
    
    void pad_to_4_bytes();
    
    std::vector<uint8_t> buffer_;
    mutable size_t pos_ = 0;
};

class XdrError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

} 
}
