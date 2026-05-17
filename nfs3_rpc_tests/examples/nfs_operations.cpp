#include "nfs3/nfs3_client.hpp"
#include "nfs3/rpc_endpoint.hpp"
#include "nfs3/nfs3_types.hpp"
#include <iostream>
#include <iomanip>
#include <cstdlib>

void print_attributes(const nfs3::fattr3& attr) {
    std::cout << "  类型: ";
    switch (attr.type_) {
        case nfs3::ftype3::NF3REG: std::cout << "文件\n"; break;
        case nfs3::ftype3::NF3DIR: std::cout << "目录\n"; break;
        case nfs3::ftype3::NF3BLK: std::cout << "块设备\n"; break;
        case nfs3::ftype3::NF3CHR: std::cout << "字符设备\n"; break;
        case nfs3::ftype3::NF3LNK: std::cout << "符号链接\n"; break;
        case nfs3::ftype3::NF3SOCK: std::cout << "Socket\n"; break;
        case nfs3::ftype3::NF3FIFO: std::cout << "FIFO\n"; break;
        default: std::cout << "未知\n"; break;
    }
    std::cout << "  大小: " << attr.size << " 字节\n";
    std::cout << "  权限: " << std::oct << std::setw(4) << attr.mode << std::dec << "\n";
    std::cout << "  UID: " << attr.uid << ", GID: " << attr.gid << "\n";
    std::cout << "  链接数: " << attr.nlink << "\n";
}

int main(int argc, char* argv[]) {
    std::string server = "localhost";
    uint16_t port = 2049;
    
    if (argc >= 2) server = argv[1];
    if (argc >= 3) port = static_cast<uint16_t>(std::stoi(argv[2]));
    
    std::cout << "=== NFSv3 测试客户端 ===\n\n";
    
    auto endpoint = nfs3::RPCEndpoint::create(server, port);
    if (!endpoint.is_connected()) {
        std::cerr << "无法连接到 " << server << ":" << port << "\n";
        return 1;
    }
    
    nfs3::NFS3TestClient client(std::move(endpoint));
    
    auto null_result = client.null();
    if (!null_result.has_value()) {
        std::cerr << "NULL 过程失败\n";
        return 1;
    }
    std::cout << "✓ NULL 过程成功\n";
    
    nfs3::nfs_fh3 root_handle;
    root_handle.data = {0x00};
    
    std::cout << "\n尝试 GETATTR (根目录句柄)...\n";
    auto getattr_result = client.getattr(root_handle);
    if (getattr_result.has_value()) {
        std::cout << "✓ GETATTR 成功\n";
        print_attributes(getattr_result->obj_attributes);
    } else {
        std::cout << "✗ GETATTR 失败 (可能需要有效的文件句柄)\n";
    }
    
    std::cout << "\n尝试 FSINFO...\n";
    auto fsinfo_result = client.fsinfo(root_handle);
    if (fsinfo_result.has_value()) {
        std::cout << "✓ FSINFO 成功\n";
        std::cout << "  最大读取: " << fsinfo_result->rtmax << " 字节\n";
        std::cout << "  最大写入: " << fsinfo_result->wtmax << " 字节\n";
    } else {
        std::cout << "✗ FSINFO 失败\n";
    }
    
    std::cout << "\n尝试 FSSTAT...\n";
    auto fsstat_result = client.fsstat(root_handle);
    if (fsstat_result.has_value()) {
        std::cout << "✓ FSSTAT 成功\n";
        std::cout << "  总空间: " << fsstat_result->tbytes << " 字节\n";
        std::cout << "  空闲空间: " << fsstat_result->fbytes << " 字节\n";
        std::cout << "  总文件数: " << fsstat_result->tfiles << "\n";
    } else {
        std::cout << "✗ FSSTAT 失败\n";
    }
    
    std::cout << "\n=== 测试完成 ===\n";
    return 0;
}
