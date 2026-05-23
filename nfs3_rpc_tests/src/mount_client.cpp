#include "nfs3/mount_client.hpp"
#ifdef HAVE_LIBTIRPC
#include <tirpc/rpc/rpc.h>
#include <tirpc/rpc/clnt.h>
#include <tirpc/rpc/xdr.h>
#else
#include "nfstest/rpc_client.hpp"
#include "nfs3/xdr_codec.hpp"
#endif
#include <cstring>

namespace nfs3 {

constexpr uint32_t MOUNTPROG = 100005;
constexpr uint32_t MOUNTVERS = 3;
constexpr uint32_t MOUNTPROC_MNT = 1;
constexpr uint32_t MOUNTPROC_UMNT = 3;
constexpr uint32_t MOUNTPROC_EXPORT = 5;

#ifdef HAVE_LIBTIRPC

struct fhstatus {
    uint32_t fhs_status;
    std::vector<uint8_t> fhs_fhandle;
    std::vector<int> auth_flavors;
};

struct MountClient::Impl {
    CLIENT* client = nullptr;
    std::string host;

    ~Impl() {
        if (client) {
            clnt_destroy(client);
            client = nullptr;
        }
    }
};

static bool_t xdr_fhstatus(XDR* xdrs, fhstatus* obj) {
    if (!xdr_u_int(xdrs, &obj->fhs_status)) return FALSE;
    if (obj->fhs_status == 0) {
        char* ptr = nullptr;
        u_int len = 0;
        constexpr u_int FHSIZE = 64;
        if (xdrs->x_op == XDR_DECODE) {
            if (!xdr_bytes(xdrs, &ptr, &len, FHSIZE)) return FALSE;
            obj->fhs_fhandle.assign(ptr, ptr + len);

            u_int auth_count = 0;
            if (!xdr_u_int(xdrs, &auth_count)) return FALSE;
            for (u_int i = 0; i < auth_count && i < 10; ++i) {
                int flavor = 0;
                if (!xdr_int(xdrs, &flavor)) return FALSE;
                obj->auth_flavors.push_back(flavor);
            }
        } else if (xdrs->x_op == XDR_ENCODE) {
            ptr = reinterpret_cast<char*>(obj->fhs_fhandle.data());
            len = obj->fhs_fhandle.size();
            if (!xdr_bytes(xdrs, &ptr, &len, FHSIZE)) return FALSE;

            u_int auth_count = obj->auth_flavors.size();
            if (!xdr_u_int(xdrs, &auth_count)) return FALSE;
            for (const auto& f : obj->auth_flavors) {
                int flavor = f;
                if (!xdr_int(xdrs, &flavor)) return FALSE;
            }
        }
    }
    return TRUE;
}

static bool_t xdr_exports_list(XDR* xdrs, std::vector<std::string>* result) {
    bool_t more = TRUE;
    while (more) {
        bool_t has_entry;
        if (!xdr_bool(xdrs, &has_entry)) return FALSE;
        if (!has_entry) break;

        char* dir = nullptr;
        if (!xdr_string(xdrs, &dir, 1024)) return FALSE;
        if (dir && *dir) {
            result->push_back(dir);
        }

        bool_t has_groups;
        if (!xdr_bool(xdrs, &has_groups)) return FALSE;
        if (has_groups) {
            char* grp = nullptr;
            while (true) {
                bool_t has_grp;
                if (!xdr_bool(xdrs, &has_grp)) return FALSE;
                if (!has_grp) break;
                if (!xdr_string(xdrs, &grp, 256)) return FALSE;
            }
        }

        if (!xdr_bool(xdrs, &more)) return FALSE;
    }
    return TRUE;
}

MountClient::MountClient(const std::string& host)
    : impl_(std::make_unique<Impl>()) {
    impl_->host = host;
    impl_->client = clnt_create(host.c_str(), MOUNTPROG, MOUNTVERS, "tcp");
    if (impl_->client) {
        auth_destroy(impl_->client->cl_auth);
        impl_->client->cl_auth = authunix_create_default();
    }
}

MountClient::~MountClient() = default;

bool MountClient::is_connected() const {
    return impl_ && impl_->client != nullptr;
}

expected<MountResult, MountError> MountClient::mnt(const std::string& path) {
    if (!is_connected()) {
        return unexpected(MountError::RPC_ERROR);
    }

    MountResult result;
    fhstatus res;
    memset(&res, 0, sizeof(res));

    char* path_ptr = const_cast<char*>(path.c_str());
    struct timeval timeout = {10, 0};

    enum clnt_stat stat = clnt_call(
        impl_->client,
        MOUNTPROC_MNT,
        (xdrproc_t)xdr_string,
        reinterpret_cast<caddr_t>(&path_ptr),
        (xdrproc_t)xdr_fhstatus,
        reinterpret_cast<caddr_t>(&res),
        timeout
    );

    if (stat != RPC_SUCCESS) {
        return unexpected(MountError::RPC_ERROR);
    }

    if (res.fhs_status != 0) {
        MountError err;
        switch (res.fhs_status) {
            case 1: err = MountError::PERM; break;
            case 2: err = MountError::NOENT; break;
            case 5: err = MountError::IO; break;
            case 13: err = MountError::ACCES; break;
            case 20: err = MountError::NOTDIR; break;
            case 63: err = MountError::NAMETOOLONG; break;
            default: err = MountError::RPC_ERROR; break;
        }
        return unexpected(err);
    }

    result.root_handle.data = std::move(res.fhs_fhandle);

    return result;
}

expected<void, MountError> MountClient::umnt(const std::string& path) {
    if (!is_connected()) {
        return unexpected(MountError::RPC_ERROR);
    }

    char* path_ptr = const_cast<char*>(path.c_str());
    char dummy;
    struct timeval timeout = {10, 0};

    enum clnt_stat stat = clnt_call(
        impl_->client,
        MOUNTPROC_UMNT,
        (xdrproc_t)xdr_string,
        reinterpret_cast<caddr_t>(&path_ptr),
        (xdrproc_t)xdr_void,
        reinterpret_cast<caddr_t>(&dummy),
        timeout
    );

    if (stat != RPC_SUCCESS) {
        return unexpected(MountError::RPC_ERROR);
    }

    return {};
}

expected<std::vector<std::string>, MountError> MountClient::list_exports() {
    if (!is_connected()) {
        return unexpected(MountError::RPC_ERROR);
    }

    std::vector<std::string> result;
    char dummy;
    struct timeval timeout = {10, 0};

    enum clnt_stat stat = clnt_call(
        impl_->client,
        MOUNTPROC_EXPORT,
        (xdrproc_t)xdr_void,
        reinterpret_cast<caddr_t>(&dummy),
        (xdrproc_t)xdr_exports_list,
        reinterpret_cast<caddr_t>(&result),
        timeout
    );

    if (stat != RPC_SUCCESS) {
        return unexpected(MountError::RPC_ERROR);
    }

    return result;
}

expected<std::vector<std::string>, MountError> MountClient::dump() {
    if (!is_connected()) {
        return unexpected(MountError::RPC_ERROR);
    }

    std::vector<std::string> result;
    return result;
}

#else // !HAVE_LIBTIRPC

struct MountClient::Impl {
    nfstest::rpc::RpcClient rpc_client;
    std::string host;

    ~Impl() {
        rpc_client.disconnect();
    }
};

struct FhstatusNative {
    uint32_t status;
    std::vector<uint8_t> fhandle;
    std::vector<int> auth_flavors;
};

static void xdr_unpack_fhstatus(const std::vector<uint8_t>& data, FhstatusNative& out) {
    xdr::XdrBuffer buf(data);
    buf.unpack(out.status);
    if (out.status == 0) {
        buf.unpack(out.fhandle);
        uint32_t auth_count;
        buf.unpack(auth_count);
        for (uint32_t i = 0; i < auth_count && i < 10; ++i) {
            int flavor;
            buf.unpack(flavor);
            out.auth_flavors.push_back(flavor);
        }
    }
}

static void xdr_unpack_exports(const std::vector<uint8_t>& data, std::vector<std::string>& out) {
    xdr::XdrBuffer buf(data);
    while (true) {
        bool has_entry;
        buf.unpack(has_entry);
        if (!has_entry) break;

        std::string dir;
        buf.unpack(dir);
        out.push_back(dir);

        bool has_groups;
        buf.unpack(has_groups);
        if (has_groups) {
            while (true) {
                bool has_grp;
                buf.unpack(has_grp);
                if (!has_grp) break;
                std::string grp;
                buf.unpack(grp);
            }
        }

        bool more;
        buf.unpack(more);
        if (!more) break;
    }
}

MountClient::MountClient(const std::string& host)
    : impl_(std::make_unique<Impl>()) {
    impl_->host = host;
    impl_->rpc_client.connect(host, 111, 3000);
}

MountClient::~MountClient() = default;

bool MountClient::is_connected() const {
    return impl_ && impl_->rpc_client.is_connected();
}

expected<MountResult, MountError> MountClient::mnt(const std::string& path) {
    if (!is_connected()) {
        return unexpected(MountError::RPC_ERROR);
    }

    xdr::XdrBuffer args_buf;
    args_buf.pack(path);
    auto args_data = args_buf.data();

    auto [status, resp_data] = impl_->rpc_client.call(
        MOUNTPROG, MOUNTVERS, MOUNTPROC_MNT, args_data, 10000);

    if (status != nfstest::rpc::RpcStatus::OK) {
        return unexpected(MountError::RPC_ERROR);
    }

    FhstatusNative fhs;
    xdr_unpack_fhstatus(resp_data, fhs);

    if (fhs.status != 0) {
        MountError err;
        switch (fhs.status) {
            case 1: err = MountError::PERM; break;
            case 2: err = MountError::NOENT; break;
            case 5: err = MountError::IO; break;
            case 13: err = MountError::ACCES; break;
            case 20: err = MountError::NOTDIR; break;
            case 63: err = MountError::NAMETOOLONG; break;
            default: err = MountError::RPC_ERROR; break;
        }
        return unexpected(err);
    }

    MountResult result;
    result.root_handle.data = std::move(fhs.fhandle);
    return result;
}

expected<void, MountError> MountClient::umnt(const std::string& path) {
    if (!is_connected()) {
        return unexpected(MountError::RPC_ERROR);
    }

    xdr::XdrBuffer args_buf;
    args_buf.pack(path);
    auto args_data = args_buf.data();

    auto [status, resp_data] = impl_->rpc_client.call(
        MOUNTPROG, MOUNTVERS, MOUNTPROC_UMNT, args_data, 10000);

    if (status != nfstest::rpc::RpcStatus::OK) {
        return unexpected(MountError::RPC_ERROR);
    }

    return {};
}

expected<std::vector<std::string>, MountError> MountClient::list_exports() {
    if (!is_connected()) {
        return unexpected(MountError::RPC_ERROR);
    }

    std::vector<uint8_t> empty_args;
    auto [status, resp_data] = impl_->rpc_client.call(
        MOUNTPROG, MOUNTVERS, MOUNTPROC_EXPORT, empty_args, 10000);

    if (status != nfstest::rpc::RpcStatus::OK) {
        return unexpected(MountError::RPC_ERROR);
    }

    std::vector<std::string> result;
    xdr_unpack_exports(resp_data, result);
    return result;
}

expected<std::vector<std::string>, MountError> MountClient::dump() {
    if (!is_connected()) {
        return unexpected(MountError::RPC_ERROR);
    }
    std::vector<std::string> result;
    return result;
}

#endif

}
