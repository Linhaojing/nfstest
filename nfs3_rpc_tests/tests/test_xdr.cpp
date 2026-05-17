#include <iostream>
#include "nfs3/xdr_codec.hpp"
#include "nfs3/nfs3_types.hpp"
#include "nfs3/nfs3_constants.hpp"

using namespace nfs3;
using namespace nfs3::xdr;

int main() {
    std::cout << "=== NFSv3 XDR 编解码器测试 ===" << std::endl;
    
    // 测试 1: 基础类型
    std::cout << "测试 1: 基础类型..." << std::endl;
    XdrBuffer buf;
    int32_t i32 = 42;
    uint32_t u32 = 0x12345678;
    uint64_t u64 = 0x123456789abcdef0;
    std::string str = "Hello NFSv3!";
    
    buf.pack(i32);
    buf.pack(u32);
    buf.pack(u64);
    buf.pack(str);
    
    int32_t out_i32;
    uint32_t out_u32;
    uint64_t out_u64;
    std::string out_str;
    
    buf.reset_read();
    buf.unpack(out_i32);
    buf.unpack(out_u32);
    buf.unpack(out_u64);
    buf.unpack(out_str);
    
    bool test1_ok = (out_i32 == i32 && out_u32 == u32 && out_u64 == u64 && out_str == str);
    std::cout << "  结果: " << (test1_ok ? "OK" : "FAIL") << std::endl;
    
    // 测试 2: NFS 数据结构
    std::cout << "测试 2: NFS 数据结构..." << std::endl;
    XdrBuffer attr_buf;
    fattr3 attr;
    attr.type_ = ftype3::NF3REG;
    attr.mode = 0644;
    attr.size = 12345;
    
    attr.serialize(attr_buf);
    
    fattr3 out_attr;
    attr_buf.reset_read();
    out_attr.deserialize(attr_buf);
    
    bool test2_ok = (out_attr.type_ == attr.type_ && out_attr.mode == attr.mode && out_attr.size == attr.size);
    std::cout << "  结果: " << (test2_ok ? "OK" : "FAIL") << std::endl;
    
    // 测试 3: LOOKUP 参数
    std::cout << "测试 3: RPC 参数..." << std::endl;
    XdrBuffer lookup_buf;
    LOOKUP3args args;
    args.what_dir.data = {0x00, 0x01, 0x02, 0x03}; // 简单的句柄
    args.what_name = "test.txt";
    
    args.serialize(lookup_buf);
    
    LOOKUP3args out_args;
    lookup_buf.reset_read();
    out_args.deserialize(lookup_buf);
    
    bool test3_ok = (out_args.what_name == args.what_name && out_args.what_dir.data.size() == args.what_dir.data.size());
    std::cout << "  结果: " << (test3_ok ? "OK" : "FAIL") << std::endl;
    
    // 总结
    int passed = 0;
    int total = 3;
    if (test1_ok) passed++;
    if (test2_ok) passed++;
    if (test3_ok) passed++;
    
    std::cout << std::endl << "=== 测试总结: " << passed << "/" << total << " 个通过 ===" << std::endl;
    return total - passed;
}
