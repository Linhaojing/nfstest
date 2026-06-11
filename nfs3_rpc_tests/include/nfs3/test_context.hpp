#pragma once

#include <gtest/gtest.h>
#include "nfs3/nfs3_client.hpp"
#include "nfs3/mount_client.hpp"
#include "nfs3/rpc_endpoint.hpp"
#include <chrono>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

namespace nfs3 {

class NFS3TestContext : public ::testing::Test {
public:
    static inline std::string server_host;
    static inline uint16_t server_port = 2049;
    static inline std::string export_path;
    static inline bool require_server = false;

protected:
    void SetUp() override {
        load_configuration();

        if (server_host.empty() || export_path.empty()) {
            const char* message = "NFS test server/export is not configured; set NFS_TEST_SERVER and NFS_TEST_EXPORT or pass --server and --export";
            if (require_server) {
                FAIL() << message;
            } else {
                GTEST_SKIP() << message;
            }
        }

        auto endpoint = RPCEndpoint::create(server_host, server_port);
        client_ = std::make_unique<NFS3TestClient>(std::move(endpoint));

        if (!has_root_fh) {
            MountClient mounter(server_host);
            ASSERT_TRUE(mounter.is_connected()) << "Failed to connect to MOUNT service";

            auto result = mounter.mnt(export_path);
            ASSERT_TRUE(result.has_value()) << "MNT call failed, error: " << static_cast<int>(result.error());

            root_fh = result->root_handle;
            has_root_fh = true;
        }

        client_->set_root_handle(root_fh);
    }

    void TearDown() override {
        if (client_) {
            client_->endpoint().shutdown();
        }
        client_.reset();
    }

    NFS3TestClient& client() { return *client_; }
    RPCEndpoint& endpoint() { return client_->endpoint(); }
    const nfs_fh3& root() const { return root_fh; }

    std::string unique_name(const std::string& prefix) const {
        return prefix + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }

private:
    static void load_configuration() {
        server_host.clear();
        export_path.clear();
        server_port = 2049;
        require_server = false;

        if (const char* env = std::getenv("NFS_TEST_SERVER")) {
            server_host = env;
        }
        if (const char* env = std::getenv("NFS_TEST_PORT")) {
            server_port = static_cast<uint16_t>(std::stoi(env));
        }
        if (const char* env = std::getenv("NFS_TEST_EXPORT")) {
            export_path = env;
        }
        if (const char* env = std::getenv("NFS_TEST_REQUIRE_SERVER")) {
            require_server = std::string(env) == "1";
        }

        const auto& argvs = ::testing::internal::GetArgvs();
        const int argc = static_cast<int>(argvs.size());

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argvs[i];
            if (arg == "--server" && i + 1 < argc) {
                server_host = argvs[++i];
            } else if (arg.rfind("--server=", 0) == 0) {
                server_host = arg.substr(std::string("--server=").size());
            } else if (arg == "--port" && i + 1 < argc) {
                server_port = static_cast<uint16_t>(std::stoi(argvs[++i]));
            } else if (arg.rfind("--port=", 0) == 0) {
                server_port = static_cast<uint16_t>(std::stoi(arg.substr(std::string("--port=").size())));
            } else if (arg == "--export" && i + 1 < argc) {
                export_path = argvs[++i];
            } else if (arg.rfind("--export=", 0) == 0) {
                export_path = arg.substr(std::string("--export=").size());
            }
        }
    }

    std::unique_ptr<NFS3TestClient> client_;
    static inline nfs_fh3 root_fh;
    static inline bool has_root_fh = false;
};

}
