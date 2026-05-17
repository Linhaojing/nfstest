#include "nfs3/nfs3_client.hpp"
#include "nfs3/rpc_endpoint.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::string server = "localhost";
    uint16_t port = 2049;
    
    if (argc >= 2) server = argv[1];
    if (argc >= 3) port = static_cast<uint16_t>(std::stoi(argv[2]));
    
    std::cout << "连接到 NFS 服务器: " << server << ":" << port << "\n";
    
    auto endpoint = nfs3::RPCEndpoint::create(server, port);
    if (!endpoint.is_connected()) {
        std::cerr << "错误: 无法连接到 NFS 服务器\n";
        return 1;
    }
    
    std::cout << "连接成功!\n";
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    auto null_result = client.null();
    if (null_result.has_value()) {
        std::cout << "NULL 过程调用成功\n";
    } else {
        std::cerr << "NULL 过程调用失败\n";
        return 1;
    }
    
    std::cout << "NFSv3 服务器响应正常\n";
    return 0;
}
