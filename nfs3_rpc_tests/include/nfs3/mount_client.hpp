#pragma once

#include "nfs3/nfs3_types.hpp"
#include "nfs3/rpc_endpoint.hpp"
#include "nfs3/expected.hpp"
#include <string>
#include <vector>

namespace nfs3 {

constexpr uint32_t MOUNT_PROGRAM = 100005;
constexpr uint32_t MOUNT_V3 = 3;

enum class MountError {
    OK = 0,
    RPC_ERROR,
    PERM = 1,
    NOENT = 2,
    IO = 5,
    ACCES = 13,
    NOTDIR = 20,
    NAMETOOLONG = 63
};

struct MountResult {
    nfs_fh3 root_handle;
    std::vector<std::string> auth_flavors;
};

class MountClient {
public:
    explicit MountClient(const std::string& host);
    ~MountClient();
    
    MountClient(const MountClient&) = delete;
    MountClient& operator=(const MountClient&) = delete;
    
    expected<MountResult, MountError> mnt(const std::string& path);
    expected<void, MountError> umnt(const std::string& path);
    expected<std::vector<std::string>, MountError> list_exports();
    expected<std::vector<std::string>, MountError> dump();
    
    bool is_connected() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}
