#include "nfs3/nfs3_client.hpp"
#ifdef HAVE_LIBTIRPC
#include "nfs3/nfs3_xdr.hpp"
#endif

namespace nfs3 {

NFS3TestClient::NFS3TestClient(RPCEndpoint endpoint)
    : endpoint_(std::move(endpoint)) {}

expected<void, Nfs3Error> NFS3TestClient::null() {
    auto result = endpoint_.call_void<void>(
        static_cast<uint32_t>(proc_num::NFSPROC3_NULL)
    );

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    return {};
}

expected<GETATTR3resok, Nfs3Error> NFS3TestClient::getattr(const nfs_fh3& object) {
    GETATTR3args args{object};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<GETATTR3args, GETATTR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_GETATTR),
        args,
        reinterpret_cast<xdrproc_t>(xdr_GETATTR3args),
        reinterpret_cast<xdrproc_t>(xdr_GETATTR3res)
    );
#else
    auto result = endpoint_.call<GETATTR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_GETATTR),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<SETATTR3resok, Nfs3Error> NFS3TestClient::setattr(
    const nfs_fh3& object,
    const sattr3& new_attributes) {

    SETATTR3args args{object, new_attributes, std::nullopt};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<SETATTR3args, SETATTR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_SETATTR),
        args,
        reinterpret_cast<xdrproc_t>(xdr_SETATTR3args),
        reinterpret_cast<xdrproc_t>(xdr_SETATTR3res)
    );
#else
    auto result = endpoint_.call<SETATTR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_SETATTR),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<LOOKUP3resok, Nfs3Error> NFS3TestClient::lookup(
    const nfs_fh3& dir,
    std::string_view name) {

    LOOKUP3args args{dir, std::string(name)};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<LOOKUP3args, LOOKUP3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_LOOKUP),
        args,
        reinterpret_cast<xdrproc_t>(xdr_LOOKUP3args),
        reinterpret_cast<xdrproc_t>(xdr_LOOKUP3res)
    );
#else
    auto result = endpoint_.call<LOOKUP3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_LOOKUP),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<ACCESS3resok, Nfs3Error> NFS3TestClient::access(
    const nfs_fh3& object,
    uint32_t access_mask) {

    ACCESS3args args{object, access_mask};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<ACCESS3args, ACCESS3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_ACCESS),
        args,
        reinterpret_cast<xdrproc_t>(xdr_ACCESS3args),
        reinterpret_cast<xdrproc_t>(xdr_ACCESS3res)
    );
#else
    auto result = endpoint_.call<ACCESS3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_ACCESS),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<READLINK3resok, Nfs3Error> NFS3TestClient::readlink(const nfs_fh3& symlink) {
    READLINK3args args{symlink};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<READLINK3args, READLINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READLINK),
        args,
        reinterpret_cast<xdrproc_t>(xdr_READLINK3args),
        reinterpret_cast<xdrproc_t>(xdr_READLINK3res)
    );
#else
    auto result = endpoint_.call<READLINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READLINK),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<READ3resok, Nfs3Error> NFS3TestClient::read(
    const nfs_fh3& file,
    uint64_t offset,
    uint32_t count) {

    READ3args args{file, offset, count};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<READ3args, READ3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READ),
        args,
        reinterpret_cast<xdrproc_t>(xdr_READ3args),
        reinterpret_cast<xdrproc_t>(xdr_READ3res)
    );
#else
    auto result = endpoint_.call<READ3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READ),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<WRITE3resok, Nfs3Error> NFS3TestClient::write(
    const nfs_fh3& file,
    uint64_t offset,
    stable_how stable,
    const bytes& data) {

    WRITE3args args{file, offset, static_cast<uint32_t>(data.size()), stable, data};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<WRITE3args, WRITE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_WRITE),
        args,
        reinterpret_cast<xdrproc_t>(xdr_WRITE3args),
        reinterpret_cast<xdrproc_t>(xdr_WRITE3res)
    );
#else
    auto result = endpoint_.call<WRITE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_WRITE),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<CREATE3resok, Nfs3Error> NFS3TestClient::create(
    const nfs_fh3& dir,
    std::string_view name,
    createmode3 mode,
    const sattr3& attributes) {

    CREATE3args args{
        dir,
        std::string(name),
        mode,
        attributes,
        createverf3{}
    };

#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<CREATE3args, CREATE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_CREATE),
        args,
        reinterpret_cast<xdrproc_t>(xdr_CREATE3args),
        reinterpret_cast<xdrproc_t>(xdr_CREATE3res)
    );
#else
    auto result = endpoint_.call<CREATE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_CREATE),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<MKDIR3resok, Nfs3Error> NFS3TestClient::mkdir(
    const nfs_fh3& dir,
    std::string_view name,
    const sattr3& attributes) {

    MKDIR3args args{dir, std::string(name), attributes};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<MKDIR3args, MKDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_MKDIR),
        args,
        reinterpret_cast<xdrproc_t>(xdr_MKDIR3args),
        reinterpret_cast<xdrproc_t>(xdr_MKDIR3res)
    );
#else
    auto result = endpoint_.call<MKDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_MKDIR),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<SYMLINK3resok, Nfs3Error> NFS3TestClient::symlink(
    const nfs_fh3& dir,
    std::string_view name,
    const sattr3& attributes,
    const std::string& data) {

    SYMLINK3args args{dir, std::string(name), attributes, data};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<SYMLINK3args, SYMLINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_SYMLINK),
        args,
        reinterpret_cast<xdrproc_t>(xdr_SYMLINK3args),
        reinterpret_cast<xdrproc_t>(xdr_SYMLINK3res)
    );
#else
    auto result = endpoint_.call<SYMLINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_SYMLINK),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<MKNOD3resok, Nfs3Error> NFS3TestClient::mknod(
    const nfs_fh3& dir,
    std::string_view name,
    ftype4 type,
    const sattr3& attributes) {

    MKNOD3args args{dir, std::string(name), type, attributes};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<MKNOD3args, MKNOD3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_MKNOD),
        args,
        reinterpret_cast<xdrproc_t>(xdr_MKNOD3args),
        reinterpret_cast<xdrproc_t>(xdr_MKNOD3res)
    );
#else
    auto result = endpoint_.call<MKNOD3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_MKNOD),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<REMOVE3resok, Nfs3Error> NFS3TestClient::remove(
    const nfs_fh3& dir,
    std::string_view name) {

    REMOVE3args args{dir, std::string(name)};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<REMOVE3args, REMOVE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_REMOVE),
        args,
        reinterpret_cast<xdrproc_t>(xdr_REMOVE3args),
        reinterpret_cast<xdrproc_t>(xdr_REMOVE3res)
    );
#else
    auto result = endpoint_.call<REMOVE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_REMOVE),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<RMDIR3resok, Nfs3Error> NFS3TestClient::rmdir(
    const nfs_fh3& dir,
    std::string_view name) {

    RMDIR3args args{dir, std::string(name)};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<RMDIR3args, RMDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_RMDIR),
        args,
        reinterpret_cast<xdrproc_t>(xdr_RMDIR3args),
        reinterpret_cast<xdrproc_t>(xdr_RMDIR3res)
    );
#else
    auto result = endpoint_.call<RMDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_RMDIR),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<RENAME3resok, Nfs3Error> NFS3TestClient::rename_op(
    const nfs_fh3& from_dir,
    std::string_view from_name,
    const nfs_fh3& to_dir,
    std::string_view to_name) {

    RENAME3args args{
        from_dir,
        std::string(from_name),
        to_dir,
        std::string(to_name)
    };

#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<RENAME3args, RENAME3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_RENAME),
        args,
        reinterpret_cast<xdrproc_t>(xdr_RENAME3args),
        reinterpret_cast<xdrproc_t>(xdr_RENAME3res)
    );
#else
    auto result = endpoint_.call<RENAME3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_RENAME),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<LINK3resok, Nfs3Error> NFS3TestClient::link(
    const nfs_fh3& file,
    const nfs_fh3& link_dir,
    std::string_view link_name) {

    LINK3args args{file, link_dir, std::string(link_name)};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<LINK3args, LINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_LINK),
        args,
        reinterpret_cast<xdrproc_t>(xdr_LINK3args),
        reinterpret_cast<xdrproc_t>(xdr_LINK3res)
    );
#else
    auto result = endpoint_.call<LINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_LINK),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<READDIR3resok, Nfs3Error> NFS3TestClient::readdir(
    const nfs_fh3& dir,
    uint64_t cookie,
    uint64_t cookieverf,
    uint32_t count) {

    READDIR3args args{dir, cookie, cookieverf, count};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<READDIR3args, READDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READDIR),
        args,
        reinterpret_cast<xdrproc_t>(xdr_READDIR3args),
        reinterpret_cast<xdrproc_t>(xdr_READDIR3res)
    );
#else
    auto result = endpoint_.call<READDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READDIR),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return std::move(result->resok.value());
}

expected<READDIRPLUS3resok, Nfs3Error> NFS3TestClient::readdirplus(
    const nfs_fh3& dir,
    uint64_t cookie,
    uint64_t cookieverf,
    uint32_t dircount,
    uint32_t maxcount) {

    READDIRPLUS3args args{dir, cookie, cookieverf, dircount, maxcount};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<READDIRPLUS3args, READDIRPLUS3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READDIRPLUS),
        args,
        reinterpret_cast<xdrproc_t>(xdr_READDIRPLUS3args),
        reinterpret_cast<xdrproc_t>(xdr_READDIRPLUS3res)
    );
#else
    auto result = endpoint_.call<READDIRPLUS3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READDIRPLUS),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return std::move(result->resok.value());
}

expected<FSSTAT3resok, Nfs3Error> NFS3TestClient::fsstat(const nfs_fh3& fsroot) {
    FSSTAT3args args{fsroot};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<FSSTAT3args, FSSTAT3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_FSSTAT),
        args,
        reinterpret_cast<xdrproc_t>(xdr_FSSTAT3args),
        reinterpret_cast<xdrproc_t>(xdr_FSSTAT3res)
    );
#else
    auto result = endpoint_.call<FSSTAT3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_FSSTAT),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<FSINFO3resok, Nfs3Error> NFS3TestClient::fsinfo(const nfs_fh3& fsroot) {
    FSINFO3args args{fsroot};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<FSINFO3args, FSINFO3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_FSINFO),
        args,
        reinterpret_cast<xdrproc_t>(xdr_FSINFO3args),
        reinterpret_cast<xdrproc_t>(xdr_FSINFO3res)
    );
#else
    auto result = endpoint_.call<FSINFO3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_FSINFO),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<PATHCONF3resok, Nfs3Error> NFS3TestClient::pathconf(const nfs_fh3& object) {
    PATHCONF3args args{object};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<PATHCONF3args, PATHCONF3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_PATHCONF),
        args,
        reinterpret_cast<xdrproc_t>(xdr_PATHCONF3args),
        reinterpret_cast<xdrproc_t>(xdr_PATHCONF3res)
    );
#else
    auto result = endpoint_.call<PATHCONF3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_PATHCONF),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }

    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

expected<COMMIT3resok, Nfs3Error> NFS3TestClient::commit(
    const nfs_fh3& file,
    uint64_t offset,
    uint32_t count) {

    COMMIT3args args{file, offset, count};
#ifdef HAVE_LIBTIRPC
    auto result = endpoint_.call_with_xdr<COMMIT3args, COMMIT3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_COMMIT),
        args,
        reinterpret_cast<xdrproc_t>(xdr_COMMIT3args),
        reinterpret_cast<xdrproc_t>(xdr_COMMIT3res)
    );
#else
    auto result = endpoint_.call<COMMIT3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_COMMIT),
        args
    );
#endif

    if (!result) {
        return unexpected(Nfs3Error::RPC_ERROR);
    }


    if (result->status != nfsstat3::NFS3_OK) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    if (!result->resok.has_value()) {
        return unexpected(Nfs3Error::SERVERFAULT);
    }

    return result->resok.value();
}

}
