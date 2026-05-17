#pragma once

#include <gtest/gtest.h>
#include "nfs3/nfs3_client.hpp"
#include "nfs3/rpc_endpoint.hpp"
#include <string>
#include <memory>
#include <optional>

namespace nfs3 {

class NFS3TestContext : public ::testing::Test {
public:
    static inline std::string server_host = "localhost";
    static inline uint16_t server_port = 2049;

protected:
    void SetUp() override {
        const auto& argvs = ::testing::internal::GetArgvs();
        const int argc = static_cast<int>(argvs.size());

        for (int i = 1; i < argc; ++i) {
            std::string arg = argvs[i];
            if (arg == "--server" && i + 1 < argc) {
                server_host = argvs[++i];
            } else if (arg == "--port" && i + 1 < argc) {
                server_port = static_cast<uint16_t>(std::stoi(argvs[++i]));
            }
        }

        auto endpoint = RPCEndpoint::create(server_host, server_port);
        client_ = std::make_unique<NFS3TestClient>(std::move(endpoint));
    }

    void TearDown() override {
        if (client_) {
            client_->endpoint().shutdown();
        }
        client_.reset();
    }

    NFS3TestClient& client() { return *client_; }
    RPCEndpoint& endpoint() { return client_->endpoint(); }

private:
    std::unique_ptr<NFS3TestClient> client_;
};

}
