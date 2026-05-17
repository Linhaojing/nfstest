#pragma once

#include "nfs3/xdr_codec.hpp"
#include <array>

namespace nfs3 {
namespace detail {

inline uint32_t swap_bytes32(uint32_t val) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(val);
#else
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8)  |
           ((val & 0x0000FF00) << 8)  |
           ((val & 0x000000FF) << 24);
#endif
}

inline uint64_t swap_bytes64(uint64_t val) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(val);
#else
    return ((val & 0xFF00000000000000ULL) >> 56) |
           ((val & 0x00FF000000000000ULL) >> 40) |
           ((val & 0x0000FF0000000000ULL) >> 24) |
           ((val & 0x000000FF00000000ULL) >> 8)  |
           ((val & 0x00000000FF000000ULL) << 8)  |
           ((val & 0x0000000000FF0000ULL) << 24) |
           ((val & 0x000000000000FF00ULL) << 40) |
           ((val & 0x00000000000000FFULL) << 56);
#endif
}

enum class Endianness {
    BIG,
    LITTLE
};

#if defined(__BYTE_ORDER__)
    #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        constexpr Endianness NATIVE_ENDIAN = Endianness::BIG;
    #else
        constexpr Endianness NATIVE_ENDIAN = Endianness::LITTLE;
    #endif
#elif defined(_WIN32)
    constexpr Endianness NATIVE_ENDIAN = Endianness::LITTLE;
#else
    constexpr Endianness NATIVE_ENDIAN = Endianness::LITTLE;
#endif

inline uint32_t host_to_network32(uint32_t val) {
    if (NATIVE_ENDIAN == Endianness::BIG) {
        return val;
    } else {
        return swap_bytes32(val);
    }
}

inline uint32_t network_to_host32(uint32_t val) {
    return host_to_network32(val);
}

inline uint64_t host_to_network64(uint64_t val) {
    if (NATIVE_ENDIAN == Endianness::BIG) {
        return val;
    } else {
        return swap_bytes64(val);
    }
}

inline uint64_t network_to_host64(uint64_t val) {
    return host_to_network64(val);
}

} 
}
