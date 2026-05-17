#include "nfs3/nfs3_client.hpp"

namespace nfs3 {

NFS3TestClient::NFS3TestClient(RPCEndpoint endpoint)
    : endpoint_(std::move(endpoint)) {}

std::expected<void, Nfs3Error> NFS3TestClient::null() {
    auto result = endpoint_.call_void<void>(
        static_cast<uint32_t>(proc_num::NFSPROC3_NULL)
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    return {};
}

std::expected<GETATTR3resok, Nfs3Error> NFS3TestClient::getattr(const nfs_fh3& object) {
    GETATTR3args args{object};
    auto result = endpoint_.call<GETATTR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_GETATTR),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<SETATTR3resok, Nfs3Error> NFS3TestClient::setattr(
    const nfs_fh3& object,
    const sattr3& new_attributes) {
    
    SETATTR3args args{object, new_attributes, std::nullopt};
    auto result = endpoint_.call<SETATTR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_SETATTR),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<LOOKUP3resok, Nfs3Error> NFS3TestClient::lookup(
    const nfs_fh3& dir,
    std::string_view name) {
    
    LOOKUP3args args{dir, std::string(name)};
    auto result = endpoint_.call<LOOKUP3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_LOOKUP),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<ACCESS3resok, Nfs3Error> NFS3TestClient::access(
    const nfs_fh3& object,
    uint32_t access_mask) {
    
    ACCESS3args args{object, access_mask};
    auto result = endpoint_.call<ACCESS3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_ACCESS),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<READLINK3resok, Nfs3Error> NFS3TestClient::readlink(const nfs_fh3& symlink) {
    READLINK3args args{symlink};
    auto result = endpoint_.call<READLINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READLINK),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<READ3resok, Nfs3Error> NFS3TestClient::read(
    const nfs_fh3& file,
    uint64_t offset,
    uint32_t count) {
    
    READ3args args{file, offset, count};
    auto result = endpoint_.call<READ3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READ),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<WRITE3resok, Nfs3Error> NFS3TestClient::write(
    const nfs_fh3& file,
    uint64_t offset,
    stable_how stable,
    const bytes& data) {
    
    WRITE3args args{file, offset, static_cast<uint32_t>(data.size()), stable, data};
    auto result = endpoint_.call<WRITE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_WRITE),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<CREATE3resok, Nfs3Error> NFS3TestClient::create(
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
    
    auto result = endpoint_.call<CREATE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_CREATE),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<MKDIR3resok, Nfs3Error> NFS3TestClient::mkdir(
    const nfs_fh3& dir,
    std::string_view name,
    const sattr3& attributes) {
    
    MKDIR3args args{dir, std::string(name), attributes};
    auto result = endpoint_.call<MKDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_MKDIR),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<SYMLINK3resok, Nfs3Error> NFS3TestClient::symlink(
    const nfs_fh3& dir,
    std::string_view name,
    const sattr3& attributes,
    const std::string& data) {
    
    SYMLINK3args args{dir, std::string(name), attributes, data};
    auto result = endpoint_.call<SYMLINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_SYMLINK),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<MKNOD3resok, Nfs3Error> NFS3TestClient::mknod(
    const nfs_fh3& dir,
    std::string_view name,
    ftype4 type,
    const sattr3& attributes) {
    
    MKNOD3args args{dir, std::string(name), type, attributes};
    auto result = endpoint_.call<MKNOD3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_MKNOD),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<REMOVE3resok, Nfs3Error> NFS3TestClient::remove(
    const nfs_fh3& dir,
    std::string_view name) {
    
    REMOVE3args args{dir, std::string(name)};
    auto result = endpoint_.call<REMOVE3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_REMOVE),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<RMDIR3resok, Nfs3Error> NFS3TestClient::rmdir(
    const nfs_fh3& dir,
    std::string_view name) {
    
    RMDIR3args args{dir, std::string(name)};
    auto result = endpoint_.call<RMDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_RMDIR),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<RENAME3resok, Nfs3Error> NFS3TestClient::rename_op(
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
    
    auto result = endpoint_.call<RENAME3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_RENAME),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<LINK3resok, Nfs3Error> NFS3TestClient::link(
    const nfs_fh3& file,
    const nfs_fh3& link_dir,
    std::string_view link_name) {
    
    LINK3args args{file, link_dir, std::string(link_name)};
    auto result = endpoint_.call<LINK3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_LINK),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<READDIR3resok, Nfs3Error> NFS3TestClient::readdir(
    const nfs_fh3& dir,
    uint64_t cookie,
    uint64_t cookieverf,
    uint32_t count) {
    
    READDIR3args args{dir, cookie, cookieverf, count};
    auto result = endpoint_.call<READDIR3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READDIR),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return std::move(result->resok.value());
}

std::expected<READDIRPLUS3resok, Nfs3Error> NFS3TestClient::readdirplus(
    const nfs_fh3& dir,
    uint64_t cookie,
    uint64_t cookieverf,
    uint32_t dircount,
    uint32_t maxcount) {
    
    READDIRPLUS3args args{dir, cookie, cookieverf, dircount, maxcount};
    auto result = endpoint_.call<READDIRPLUS3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_READDIRPLUS),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return std::move(result->resok.value());
}

std::expected<FSSTAT3resok, Nfs3Error> NFS3TestClient::fsstat(const nfs_fh3& fsroot) {
    FSSTAT3args args{fsroot};
    auto result = endpoint_.call<FSSTAT3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_FSSTAT),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<FSINFO3resok, Nfs3Error> NFS3TestClient::fsinfo(const nfs_fh3& fsroot) {
    FSINFO3args args{fsroot};
    auto result = endpoint_.call<FSINFO3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_FSINFO),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<PATHCONF3resok, Nfs3Error> NFS3TestClient::pathconf(const nfs_fh3& object) {
    PATHCONF3args args{object};
    auto result = endpoint_.call<PATHCONF3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_PATHCONF),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }
    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

std::expected<COMMIT3resok, Nfs3Error> NFS3TestClient::commit(
    const nfs_fh3& file,
    uint64_t offset,
    uint32_t count) {
    
    COMMIT3args args{file, offset, count};
    auto result = endpoint_.call<COMMIT3res>(
        static_cast<uint32_t>(proc_num::NFSPROC3_COMMIT),
        args
    );
    
    if (!result) {
        return std::unexpected(Nfs3Error::RPC_ERROR);
    }

    
    if (result->status != nfsstat3::NFS3_OK) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    if (!result->resok.has_value()) {
        return std::unexpected(Nfs3Error::SERVERFAULT);
    }
    
    return result->resok.value();
}

} 
