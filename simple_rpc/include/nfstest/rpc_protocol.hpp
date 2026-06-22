#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include <arpa/inet.h>

namespace nfstest {
namespace rpc {

constexpr uint32_t MSG_CALL  = 0;
constexpr uint32_t MSG_REPLY = 1;

constexpr uint32_t RPCVERS = 2;

constexpr uint32_t AUTH_NONE = 0;
constexpr uint32_t AUTH_SYS  = 1;

enum class AcceptStat : uint32_t {
    SUCCESS       = 0,
    PROG_UNAVAIL  = 1,
    PROG_MISMATCH = 2,
    PROC_UNAVAIL  = 3,
    GARBAGE_ARGS  = 4,
    SYSTEM_ERR    = 5,
};

enum class RejectStat : uint32_t {
    RPC_MISMATCH = 0,
    AUTH_ERROR   = 1,
};

enum class AuthStat : uint32_t {
    AUTH_OK           = 0,
    AUTH_BADCRED      = 1,
    AUTH_REJECTEDCRED = 2,
    AUTH_BADVERF      = 3,
    AUTH_REJECTEDVERF = 4,
    AUTH_TOOWEAK      = 5,
};

enum class RpcStatus {
    OK = 0,
    CONN_ERR,
    TIMEOUT,
    PROG_UNAVAIL,
    PROG_MISMATCH,
    PROC_UNAVAIL,
    GARBAGE_ARGS,
    AUTH_ERROR,
    PROTO_ERR,
    SEND_ERR,
    RECV_ERR,
};

inline void write_uint32_be(std::vector<uint8_t>& buf, uint32_t val) {
    uint32_t n = htonl(val);
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&n);
    buf.insert(buf.end(), p, p + 4);
}

inline uint32_t read_uint32_be(const uint8_t* data, size_t offset) {
    uint32_t n;
    std::memcpy(&n, data + offset, 4);
    return ntohl(n);
}

inline void write_auth_none(std::vector<uint8_t>& buf) {
    write_uint32_be(buf, AUTH_NONE);
    write_uint32_be(buf, 0);
}

inline void write_auth_sys_cred(std::vector<uint8_t>& buf,
                                 uint32_t stamp,
                                 uint32_t uid, uint32_t gid,
                                 const std::vector<uint32_t>& gids) {
    write_uint32_be(buf, AUTH_SYS);
    size_t cred_len_offset = buf.size();
    write_uint32_be(buf, 0);

    size_t cred_start = buf.size();
    write_uint32_be(buf, stamp);

    const char* hostname = "localhost";
    uint32_t hostname_len = std::strlen(hostname);
    write_uint32_be(buf, hostname_len);
    for (uint32_t i = 0; i < hostname_len; i += 4) {
        for (size_t j = 0; j < 4 && (i + j) < hostname_len; j++) {
            buf.push_back(static_cast<uint8_t>(hostname[i + j]));
        }
        for (size_t j = hostname_len - i; j < 4 && j < 4; j++) {
            buf.push_back(0);
        }
    }

    write_uint32_be(buf, uid);
    write_uint32_be(buf, gid);

    write_uint32_be(buf, static_cast<uint32_t>(gids.size()));
    for (uint32_t g : gids) {
        write_uint32_be(buf, g);
    }

    uint32_t cred_len = htonl(static_cast<uint32_t>(buf.size() - cred_start));
    std::memcpy(&buf[cred_len_offset], &cred_len, 4);
}

inline std::vector<uint8_t> build_call(uint32_t xid, uint32_t prog, uint32_t vers,
                                        uint32_t proc,
                                        const std::vector<uint8_t>& args_body) {
    std::vector<uint8_t> buf;
    buf.reserve(128 + args_body.size());

    write_uint32_be(buf, xid);
    write_uint32_be(buf, MSG_CALL);
    write_uint32_be(buf, RPCVERS);
    write_uint32_be(buf, prog);
    write_uint32_be(buf, vers);
    write_uint32_be(buf, proc);

    write_auth_sys_cred(buf, 0, 0, 0, {});

    write_auth_none(buf);

    buf.insert(buf.end(), args_body.begin(), args_body.end());

    return buf;
}

inline RpcStatus parse_reply(const uint8_t* data, size_t data_len,
                              uint32_t expected_xid,
                              std::vector<uint8_t>& out_body) {
    if (data_len < 20) {
        return RpcStatus::PROTO_ERR;
    }

    uint32_t xid = read_uint32_be(data, 0);
    if (xid != expected_xid) {
        return RpcStatus::PROTO_ERR;
    }

    uint32_t msg_type = read_uint32_be(data, 4);
    if (msg_type != MSG_REPLY) {
        return RpcStatus::PROTO_ERR;
    }

    uint32_t reply_stat = read_uint32_be(data, 8);

    if (reply_stat == 0) {
        size_t offset = 12;

        uint32_t auth_flavor = read_uint32_be(data, offset);
        offset += 4;
        uint32_t auth_len = read_uint32_be(data, offset);
        offset += 4;
        uint32_t auth_len_rounded = ((auth_len + 3) / 4) * 4;
        offset += auth_len_rounded;
        if (offset > data_len) return RpcStatus::PROTO_ERR;

        uint32_t accept_stat = read_uint32_be(data, offset);
        offset += 4;

        switch (accept_stat) {
            case 0: /* SUCCESS */ break;
            case 1: return RpcStatus::PROG_UNAVAIL;
            case 2: return RpcStatus::PROG_MISMATCH;
            case 3: return RpcStatus::PROC_UNAVAIL;
            case 4: return RpcStatus::GARBAGE_ARGS;
            case 5: return RpcStatus::PROTO_ERR;
            default: return RpcStatus::PROTO_ERR;
        }

        if (auth_flavor == AUTH_SYS) {
            if (offset + 4 > data_len) return RpcStatus::PROTO_ERR;
            uint32_t verf_len = read_uint32_be(data, offset);
            offset += 4;
            verf_len = ((verf_len + 3) / 4) * 4;
            offset += verf_len;
            if (offset > data_len) return RpcStatus::PROTO_ERR;
        }

        size_t body_len = data_len - offset;
        out_body.assign(data + offset, data + offset + body_len);
        return RpcStatus::OK;
    }

    if (reply_stat == 1) {
        size_t offset = 12;
        uint32_t reject_stat = read_uint32_be(data, offset);
        if (reject_stat == 0) return RpcStatus::PROG_MISMATCH;
        if (reject_stat == 1) return RpcStatus::AUTH_ERROR;
        return RpcStatus::PROTO_ERR;
    }

    return RpcStatus::PROTO_ERR;
}

} // namespace rpc
} // namespace nfstest
