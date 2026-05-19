#pragma once

#include "nfs3/nfs3_types.hpp"
#include "nfs3/rpc_endpoint.hpp"
#include <string_view>

namespace nfs3 {

enum class Nfs3Error {
    OK = 0,
    BADHANDLE,
    NOT_SYNC,
    BAD_COOKIE,
    NOTSUPP,
    TOOSMALL,
    SERVERFAULT,
    BADTYPE,
    JUKEBOX,
    RPC_ERROR
};

class NFS3TestClient {
public:
    explicit NFS3TestClient(RPCEndpoint endpoint);
    
    NFS3TestClient(const NFS3TestClient&) = delete;
    NFS3TestClient& operator=(const NFS3TestClient&) = delete;
    
    RPCEndpoint& endpoint() { return endpoint_; }
    const RPCEndpoint& endpoint() const { return endpoint_; }
    
    const nfs_fh3& root_handle() const { return root_fh_; }
    void set_root_handle(const nfs_fh3& fh) { root_fh_ = fh; }
    
    expected<void, Nfs3Error> null();
    
    expected<GETATTR3resok, Nfs3Error> getattr(const nfs_fh3& object);
    
    expected<SETATTR3resok, Nfs3Error> setattr(
        const nfs_fh3& object,
        const sattr3& new_attributes
    );
    
    expected<LOOKUP3resok, Nfs3Error> lookup(
        const nfs_fh3& dir,
        std::string_view name
    );
    
    expected<ACCESS3resok, Nfs3Error> access(
        const nfs_fh3& object,
        uint32_t access_mask
    );
    
    expected<READLINK3resok, Nfs3Error> readlink(const nfs_fh3& symlink);
    
    expected<READ3resok, Nfs3Error> read(
        const nfs_fh3& file,
        uint64_t offset,
        uint32_t count
    );
    
    expected<WRITE3resok, Nfs3Error> write(
        const nfs_fh3& file,
        uint64_t offset,
        stable_how stable,
        const bytes& data
    );
    
    expected<CREATE3resok, Nfs3Error> create(
        const nfs_fh3& dir,
        std::string_view name,
        createmode3 mode,
        const sattr3& attributes
    );
    
    expected<MKDIR3resok, Nfs3Error> mkdir(
        const nfs_fh3& dir,
        std::string_view name,
        const sattr3& attributes
    );
    
    expected<SYMLINK3resok, Nfs3Error> symlink(
        const nfs_fh3& dir,
        std::string_view name,
        const sattr3& attributes,
        const std::string& data
    );
    
    expected<MKNOD3resok, Nfs3Error> mknod(
        const nfs_fh3& dir,
        std::string_view name,
        ftype4 type,
        const sattr3& attributes
    );
    
    expected<REMOVE3resok, Nfs3Error> remove(
        const nfs_fh3& dir,
        std::string_view name
    );
    
    expected<RMDIR3resok, Nfs3Error> rmdir(
        const nfs_fh3& dir,
        std::string_view name
    );
    
    expected<RENAME3resok, Nfs3Error> rename_op(
        const nfs_fh3& from_dir,
        std::string_view from_name,
        const nfs_fh3& to_dir,
        std::string_view to_name
    );
    
    expected<LINK3resok, Nfs3Error> link(
        const nfs_fh3& file,
        const nfs_fh3& link_dir,
        std::string_view link_name
    );
    
    expected<READDIR3resok, Nfs3Error> readdir(
        const nfs_fh3& dir,
        uint64_t cookie,
        uint64_t cookieverf,
        uint32_t count
    );
    
    expected<READDIRPLUS3resok, Nfs3Error> readdirplus(
        const nfs_fh3& dir,
        uint64_t cookie,
        uint64_t cookieverf,
        uint32_t dircount,
        uint32_t maxcount
    );
    
    expected<FSSTAT3resok, Nfs3Error> fsstat(const nfs_fh3& fsroot);
    
    expected<FSINFO3resok, Nfs3Error> fsinfo(const nfs_fh3& fsroot);
    
    expected<PATHCONF3resok, Nfs3Error> pathconf(const nfs_fh3& object);
    
    expected<COMMIT3resok, Nfs3Error> commit(
        const nfs_fh3& file,
        uint64_t offset,
        uint32_t count
    );

private:
    RPCEndpoint endpoint_;
    nfs_fh3 root_fh_;
};

} 
