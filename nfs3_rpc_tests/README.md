# NFSv3 RPC 测试套件

一个 C++17 实现的 NFSv3 (RFC 1813) RPC 接口测试框架，使用手写 XDR 编解码器，不依赖 rpcgen。

## 特性

- **完整 NFSv3 协议支持**：覆盖全部 22 个 NFSv3 过程
- **手写 XDR 编解码器**：无 rpcgen 依赖，类型安全
- **最小依赖**：仅需 libtirpc、Google Test
- **CMake 构建**：兼容 CMake 2.8+
- **TCP 优先**：TCP 传输优先，UDP 支持计划中

## 依赖

- C++17 编译器 (GCC 9+, Clang 10+)
- CMake 2.8+
- libtirpc-dev
- libgtest-dev

### Ubuntu/Debian 安装

```bash
sudo apt-get install cmake g++ libtirpc-dev libgtest-dev
```

## 构建

```bash
cd nfs3_rpc_tests
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## 运行测试

```bash
# 运行所有测试
./tests/test_nfs3_suite

# 运行特定测试
./tests/test_nfs3_suite --gtest_filter="RpcNullTest.*"

# 连接到指定 NFS 服务器
./tests/test_nfs3_suite --server=192.168.1.100 --port=2049
```

## 项目结构

```
nfs3_rpc_tests/
├── include/nfs3/
│   ├── xdr_codec.hpp        # XDR 编解码器
│   ├── nfs3_types.hpp       # RFC 1813 数据结构
│   ├── nfs3_constants.hpp   # 程序号、版本号、错误码
│   ├── rpc_endpoint.hpp     # RPC 连接管理
│   ├── nfs3_client.hpp      # 22 个 NFSv3 过程 API
│   └── test_context.hpp     # GTest Fixture
├── src/
│   ├── xdr_codec.cpp
│   ├── rpc_endpoint.cpp
│   ├── nfs3_client.cpp
│   └── test_context.cpp
├── tests/
│   ├── test_xdr.cpp         # XDR 编解码单元测试
│   ├── test_rpc_null.cpp    # NULL 过程测试
│   ├── test_rpc_errors.cpp  # RPC 错误处理测试
│   ├── test_nfs3_getattr.cpp
│   ├── test_nfs3_lookup.cpp
│   ├── test_nfs3_readwrite.cpp
│   ├── test_nfs3_create.cpp
│   ├── test_nfs3_remove.cpp
│   ├── test_nfs3_readdir.cpp
│   ├── test_nfs3_other.cpp
│   └── test_nfs3_stress.cpp
└── scripts/
    └── run_tests.sh
```

## API 使用示例

### 基本连接

```cpp
#include "nfs3/nfs3_client.hpp"
#include "nfs3/rpc_endpoint.hpp"

// 创建 RPC 连接
auto endpoint = nfs3::RPCEndpoint::create("192.168.1.100", 2049);
if (!endpoint.is_connected()) {
    std::cerr << "连接失败\n";
    return 1;
}

// 创建客户端
nfs3::NFS3TestClient client(std::move(endpoint));

// 调用 NULL 过程
auto result = client.null();
if (result.has_value()) {
    std::cout << "NFS NULL 成功\n";
}
```

### GETATTR 示例

```cpp
nfs3::nfs_fh3 file_handle;
file_handle.data = {0x01, 0x02, 0x03, 0x04};  // 从服务器获取

auto attr_result = client.getattr(file_handle);
if (attr_result.has_value()) {
    const auto& attrs = attr_result.value();
    std::cout << "文件大小: " << attrs.obj_attributes.size << "\n";
    std::cout << "类型: " << static_cast<int>(attrs.obj_attributes.type_) << "\n";
}
```

### LOOKUP 示例

```cpp
auto lookup_result = client.lookup(dir_handle, "filename.txt");
if (lookup_result.has_value()) {
    auto file_handle = lookup_result->object;
    std::cout << "找到文件，句柄长度: " << file_handle.data.size() << "\n";
}
```

### READ/WRITE 示例

```cpp
// 读取文件
auto read_result = client.read(file_handle, 0, 4096);
if (read_result.has_value()) {
    const auto& data = read_result->data;
    std::cout << "读取 " << data.size() << " 字节\n";
}

// 写入文件
nfs3::bytes write_data = {'H', 'e', 'l', 'l', 'o'};
auto write_result = client.write(file_handle, 0, nfs3::stable_how::FILE_SYNC, write_data);
if (write_result.has_value()) {
    std::cout << "写入 " << write_result->count << " 字节\n";
}
```

## 测试覆盖

| 类别 | 测试数量 | 说明 |
|------|----------|------|
| XDR 编解码 | 3 | 基础类型、NFS 结构、RPC 参数 |
| NULL 过程 | 4 | 基本调用、连接验证、多次调用 |
| RPC 错误 | 5 | 连接拒绝、超时、认证错误 |
| GETATTR | 5 | 参数/响应 XDR 往返 |
| LOOKUP | 6 | 参数/响应 XDR 往返 |
| READ/WRITE | 9 | 参数/响应 XDR 往返 |
| CREATE | 7 | UNCHECKED/GUARDED/EXCLUSIVE 模式 |
| REMOVE | 8 | 文件删除、目录删除 |
| READDIR | 10 | 目录枚举、FSSTAT/FSINFO |
| 其他操作 | 16 | SETATTR/ACCESS/LINK/RENAME 等 |
| 压力测试 | 6 | 大数据量、并行、1000 次往返 |

**总计：76 个测试用例**

## XDR 编解码器

手写 XDR 编解码器支持：

- 基础类型：`int32_t`, `uint32_t`, `uint64_t`, `bool`
- 变长类型：`std::string`, `std::vector<uint8_t>`
- 复合类型：结构体、可选值 (`std::optional`)
- 网络字节序转换

```cpp
nfs3::xdr::XdrBuffer buf;

// 编码
buf.pack(my_nfs_structure);

// 获取字节
auto bytes = buf.data();

// 解码
nfs3::xdr::XdrBuffer back(bytes);
MyStruct restored;
back.unpack(restored);
```

## 错误处理

所有 API 使用 `std::expected<T, E>` 返回结果：

```cpp
auto result = client.getattr(handle);
if (!result.has_value()) {
    nfs3::Nfs3Error error = result.error();
    // 处理错误
}
```

## 容器环境限制

在容器中运行 NFS 测试有以下限制：

1. **无法 mount 文件系统**：缺少 `CAP_SYS_ADMIN`
2. **Ganesha VFS 不可用**：overlay 文件系统不支持 `open_by_handle_at`
3. **内核 NFS 服务器不可用**：无法访问 `/proc/fs/nfsd`

解决方案：
- 使用特权容器 (`--privileged`)
- 使用物理机或虚拟机
- 使用 NULL FSAL 测试 RPC 协议层

## 许可证

MIT License

## 参考

- [RFC 1813 - NFS Version 3 Protocol Specification](https://tools.ietf.org/html/rfc1813)
- [libtirpc Documentation](https://docs.oracle.com/cd/E36784_01/html/E36880/tirpc-3nsl.html)
