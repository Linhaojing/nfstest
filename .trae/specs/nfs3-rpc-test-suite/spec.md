# NFSv3 RPC接口测试套件设计规范

## Why

当前缺乏一套直接针对NFSv3服务器RPC接口的C++测试框架。现有的cthon-nfs-tests通过POSIX系统调用间接测试（无法精确控制RPC字段），而Vaiz/nfs3虽提供参考但使用Rust实现且覆盖不全面。本规范旨在设计一套**直接构造和发送RPC包**的NFSv3接口测试套件，用于验证服务器对RFC 1813协议的合规性、错误处理能力和边界条件处理。

## What Changes

* 创建一套**C++17**实现的NFSv3 RPC测试框架

* 直接在RPC层面构造请求/验证响应（绕过内核NFS客户端）

* 覆盖NFSv3全部22个RPC过程的正常路径、错误路径和边界条件

* 提供可扩展的测试基础设施（连接管理、XDR编解码、断言宏）

* **采用最小依赖原则**：仅依赖 libtirpc（运行时RPC库）

## Impact

* **目标系统**: 任意NFSv3服务器实现（用户空间或内核态）

* **测试层次**: RPC协议层 + NFSv3操作语义层

* **通信方式**: **TCP直连**服务器2049端口（无需mount/portmap，UDP作为未来扩展）

* **语言标准**: C++17（利用std::optional, std::variant, std::string\_view等现代特性）

* **构建工具**: CMake ≥ 2.8（兼容老旧系统）

## 技术选型与依赖策略

### 核心原则：最小依赖

| 决策项        | 选择                   | 理由                                      |
| ---------- | -------------------- | --------------------------------------- |
| **编程语言**   | C++17                | 类型安全、RAII资源管理、STL容器简化代码                 |
| **构建系统**   | **CMake ≥ 2.8**      | 兼容老旧Linux发行版，广泛可用                       |
| **RPC运行时** | libtirpc             | 唯一外部依赖，提供socket级RPC API                 |
| **XDR编解码** | **手写实现** ❌ 不使用rpcgen | 零生成工具依赖，完全可控，C++类型安全                    |
| **加密认证**   | ❌ 不使用OpenSSL         | 仅测试AUTH\_NONE/AUTH\_UNIX，不涉及RPCSEC\_GSS |
| **测试框架**   | Google Test (gtest)  | 工业标准，断言丰富，CI友好（可选但推荐）                   |

### 全量依赖清单

```
必需依赖（运行时）:
├── g++              ≥ 7.0   (C++17支持)
├── cmake            ≥ 2.8   (构建系统，兼容老旧发行版)
└── libtirpc-dev             (唯一外部库: RPC/XDR运行时)

可选依赖（开发/测试）:
└── libgtest-dev             (Google Test单元测试框架)

总计: 3个必需包 + 1个可选包
对比使用rpcgen方案: 减少1个包(rpcsvc-proto)，消除代码生成步骤
```

### 为什么手写XDR而不使用rpcgen？

| 维度       | rpcgen生成              | 手写C++实现       |
| -------- | --------------------- | ------------- |
| **依赖**   | 需要rpcgen工具+协议定义文件(.x) | ✅ 零额外依赖       |
| **类型安全** | C风格void\*，容易出错        | ✅ 强类型，编译期检查   |
| **内存管理** | 手动malloc/free         | ✅ RAII自动管理    |
| **可读性**  | K\&R C风格，晦涩           | ✅ 现代C++，自文档化  |
| **调试性**  | 宏展开难以调试               | ✅ 模板代码可直接单步调试 |
| **定制性**  | 固定模式，难扩展              | ✅ 可添加自定义序列化逻辑 |
| **维护成本** | 需要维护.x文件和生成脚本         | ✅ 单一源码 truth  |

**结论**: 对于本项目（仅客户端、固定协议版本），手写XDR的工作量可控且长期收益更高。

***

## 架构设计

### 分层架构

```
┌─────────────────────────────────────────────────────┐
│                测试用例层 (Test Cases)               │
│  test_rpc_null.cpp    test_nfs3_readwrite.cpp       │
│  test_rpc_errors.cpp  test_nfs3_lookup.cpp          │
│  ...                                               │
├─────────────────────────────────────────────────────┤
│                测试框架层 (Test Framework)           │
│  ┌───────────────┐  ┌──────────────────────────┐   │
│  │ TestContext   │  │  NFS3TestClient          │   │
│  │ (生命周期)     │  │  (高级API封装)            │   │
│  └───────────────┘  └──────────┬───────────────┘   │
│  ┌───────────────┐            │                    │
│  │ GTest集成     │            ▼                    │
│  │ (断言/Fixture)│  ┌─────────────────────┐      │
│  └───────────────┘  │ RPCEndpoint          │      │
│                     │ (底层TCP收发)        │      │
│                     └──────────┬──────────┘      │
├────────────────────────────────┼───────────────────┤
│              协议层 (Protocol Layer - 手写XDR)      │
│  ┌────────────────┐  ┌───────┴──────────────┐     │
│  │ xdr_codec.hpp  │  │ nfs3_types.hpp        │     │
│  │ (XDR编解码模板) │  │ (NFSv3数据结构定义)   │     │
│  └────────────────┘  └────────────────────────┘     │
├─────────────────────────────────────────────────────┤
│                  传输层 (Transport)                  │
│                    TCP Socket                       │
│              (通过libtirpc的clnt_create)             │
│              [未来扩展: UDP支持]                     │
└─────────────────────────────────────────────────────┘
```

### 核心组件职责

| 组件                 | 文件                     | 语言  | 职责                                      |
| ------------------ | ---------------------- | --- | --------------------------------------- |
| **RPCEndpoint**    | `rpc_endpoint.hpp/cpp` | C++ | 封装libtirpc CLIENT句柄，管理TCP连接生命周期         |
| **NFS3TestClient** | `nfs3_client.hpp/cpp`  | C++ | 封装所有22个NFSv3过程调用API（类型安全）               |
| **TestContext**    | `test_context.hpp/cpp` | C++ | 管理测试生命周期（GTest Fixture封装）               |
| **XDR Codec**      | `xdr_codec.hpp/cpp`    | C++ | 模板化的XDR编解码器（替代rpcgen生成代码）               |
| **NFS3 Types**     | `nfs3_types.hpp`       | C++ | RFC 1813所有数据结构的C++定义（struct/enum/using） |

***

## 项目结构（CMake组织）

```
nfs3_rpc_tests/
├── CMakeLists.txt                 # 顶层CMake配置
├── cmake/
│   ├── FindGTest.cmake            # GTest查找模块
│   └── FindTirpc.cmake            # libtirpc查找模块
├── include/
│   ├── nfs3/
│   │   ├── rpc_endpoint.hpp       # RPC连接管理（PIMPL隐藏实现）
│   │   ├── nfs3_client.hpp        # NFSv3客户端高级API
│   │   ├── test_context.hpp       # 测试上下文（GTest Fixture）
│   │   ├── xdr_codec.hpp          # XDR编解码模板
│   │   ├── nfs3_types.hpp         # NFSv3数据结构定义
│   │   └── nfs3_constants.hpp     # 常量定义（程序号、版本号、错误码等）
│   └── nfs3/
│       └── detail/                # 内部实现细节（不暴露给用户）
│           ├── rpc_msg.hpp        # RPC消息结构（call_body, reply_body等）
│           └── xdr_primitive.hpp  # 基础XDR类型编解码
├── src/
│   ├── rpc_endpoint.cpp           # RPCEndpoint实现（基于libtirpc clnt_*）
│   ├── nfs3_client.cpp            # 22个NFSv3过程的具体实现
│   ├── test_context.cpp           # TestContext实现
│   ├── xdr_codec.cpp              # XDR编解码器实现
│   └── detail/
│       ├── rpc_msg.cpp            # RPC消息序列化
│       └── xdr_primitive.cpp      # int32, uint32, string, opaque等基础类型
├── tests/
│   ├── CMakeLists.txt            # 测试可执行文件定义
│   ├── test_rpc_null.cpp         # NULL过程测试
│   ├── test_rpc_errors.cpp       # RPC错误处理测试
│   ├── test_nfs3_getattr.cpp     # GETATTR/SETATTR测试
│   ├── test_nfs3_lookup.cpp      # LOOKUP测试
│   ├── test_nfs3_readwrite.cpp   # READ/WRITE测试
│   ├── test_nfs3_create.cpp      # CREATE/MKDIR测试
│   ├── test_nfs3_remove.cpp      # REMOVE/RMDIR测试
│   ├── test_nfs3_readdir.cpp     # READDIR/READDIRPLUS测试
│   ├── test_nfs3_other.cpp       # 其他操作测试
│   └── test_nfs3_stress.cpp      # 边界条件/压力测试
├── third_party/                   # 可选：预下载的gtest（如果系统没有）
│   └── googletest/               # Git submodule或打包携带
├── README.md                     # 使用文档
└── scripts/
    └── run_tests.sh              # 便捷运行脚本
```

***

## ADDED Requirements

### Requirement: 最小化依赖构建

系统 SHALL 在仅安装 g++, cmake, libtirpc-dev 的情况下完成核心功能编译。

#### Scenario: 基础环境安装（Ubuntu示例）

* **GIVEN** 一个全新的Ubuntu 22.04系统

* **WHEN** 执行以下命令：

  ```bash
  sudo apt update && sudo apt install -y g++ cmake pkg-config libtirpc-dev
  ```

* **THEN** 可以成功执行 `cmake -B build && cmake --build build`

* **AND** 核心库（libnfs3\_test\_core.a）编译通过

* **AND** 无需安装rpcgen, openssl, 或其他额外包

#### Scenario: 添加测试框架支持（可选）

* **GIVEN** 已安装基础依赖

* **WHEN** 执行 `sudo apt install -y libgtest-dev`

* **THEN** 测试用例可以编译并运行

* **AND** 如果未安装gtest，核心库仍可正常编译和使用

***

### Requirement: 手写XDR编解码器

系统 SHALL 提供一套完整的C++模板化XDR编解码器，无需依赖rpcgen。

#### Scenario: 基础类型编解码

* **GIVEN** XDRCodec模板类

* **WHEN** 对int32\_t, uint32\_t, uint64\_t, std::string, std::vector\<uint8\_t>进行pack/unpack

* **THEN** 输出符合RFC 4506 (XDR)标准的字节流

* **AND** 支持网络字节序转换（大端）

#### Scenario: 复合结构体编解码

* **GIVEN** 定义好的NFSv3数据结构（如fattr3, LOOKUP3args等）

* **WHEN** 调用xdr\_encode(args, buffer)或xdr\_decode(buffer, result)

* **THEN** 结构体的所有字段按XDR规则正确序列化/反序列化

* **AND** 支持嵌套结构体、变长数组、联合体(union)、可选字段

#### Scene: 与libtirpc的互操作性

* **GIVEN** 使用手写XDR编码的请求

* **WHEN** 通过libtirpc发送到标准NFS服务器

* **THEN** 服务器能正确解析请求并返回响应

* **AND** 手写XDR解码器能正确解析服务器返回的标准格式响应

***

### Requirement: RPC连接管理

系统 SHALL 提供可靠的TCP连接管理能力，底层基于libtirpc。

#### Scenario: 建立TCP连接到NFS服务器

* **GIVEN** 一个运行中的NFSv3服务器（地址:port）

* **WHEN** 调用 `RPCEndpoint::create("192.168.1.100", 2049)`

* **THEN** 成功建立TCP连接并返回RPCEndpoint对象

* **AND** 连接内部持有有效的libtirpc CLIENT\*

#### Scenario: 发送RPC消息并接收响应

* **GIVEN** 一个已建立的RPCEndpoint

* **WHEN** 调用 `endpoint.call<NFS3_RES>(proc_num, args)`

* **THEN** 内部正确封装RPC消息（XID分配、auth设置）

* **AND** 通过libtirpc clnt\_call()发送并接收响应

* **AND** 返回类型安全的NFS3结果对象（包含状态码和数据）

#### Scenario: 连接生命周期管理（RAII）

* **GIVEN** 创建的RPCEndpoint对象

* **WHEN** 对象离开作用域被销毁

* **THEN** 自动调用clnt\_destroy()释放资源

* **AND** 支持显式shutdown()提前关闭

***

### Requirement: NFSv3 NULL过程测试

系统 SHALL 测试NFSPROC3\_NULL（过程号0）的正确性。

#### Scenario: NULL请求正常响应

* **GIVEN** 连接到NFSv3服务器

* **WHEN** 发送NULL请求（无参数，返回Void）

* **THEN** 服务器返回成功响应

* **AND** XID匹配请求

#### Scenario: 顺序多个NULL请求

* **GIVEN** 连接到NFSv3服务器

* **WHEN** 连续发送10个不同XID的NULL请求

* **THEN** 每个请求都收到匹配XID的成功响应

#### Scenario: 并发NULL请求

* **GIVEN** 连接到NFSv3服务器

* **WHEN** 同时发送5个不同XID的NULL请求（多线程或异步）

* **THEN** 所有请求都收到正确响应

#### Scenario: NULL请求带额外数据（容错性）

* **GIVEN** 连接到NFSv3服务器

* **WHEN** 发送NULL请求并附加4字节数据

* **THEN** 服务器忽略额外数据并返回成功（符合RFC 1813 Section 3.0）

***

### Requirement: NFSv3 属性操作测试

系统 SHALL 测试GETATTR、SETATTR过程的正确性和错误处理。

#### Scenario: GETATTR获取根目录属性

* **GIVEN** 已获取根目录文件句柄（零长度特殊handle或MOUNT获取）

* **WHEN** 调用GETATTR(root\_fh)

* **THEN** 返回成功的fattr3结构

* **AND** type字段为NF3DIR

* **AND** size >= 0

#### Scenario: GETATTR无效句柄错误

* **GIVEN** 一个无效的文件句柄（全零或随机64字节）

* **WHEN** 调用GETATTR(invalid\_fh)

* **THEN** 返回NFS3ERR\_BADHANDLE错误

#### Scenario: SETATTR修改文件大小

* **GIVEN** 一个已存在的普通文件句柄

* **WHEN** 调用SETATTR设置size=0（截断文件）

* **THEN** 返回成功

* **AND** 后续GETATTR确认size=0

***

### Requirement: NFSv3 查找操作测试

系统 SHALL 测试LOOKUP过程的路径解析能力。

#### Scenario: LOOKUP查找根目录自身(".")

* **GIVEN** 根目录文件句柄

* **WHEN** LOOKUP(root, ".")

* **THEN** 返回的对象句柄等于root句柄

#### Scenario: LOOKUP查找已存在文件

* **GIVEN** 根目录句柄和已知存在的文件名"a.txt"

* **WHEN** LOOKUP(root, "a.txt")

* **THEN** 返回该文件的句柄和属性

* **AND** obj\_attributes.type = NF3REG

#### Scenario: LOOKUP查找不存在文件

* **GIVEN** 根目录句柄和不存在的文件名"nonexist.txt"

* **WHEN** LOOKUP(root, "nonexist.txt")

* **THEN** 返回NFS3ERR\_NOENT错误

#### Scenario: LOOKUP空名称/超长名称错误

* **GIVEN** 目录句柄

* **WHEN** LOOKUP(dir, "") 或 LOOKUP(dir, ">255字节名称")

* **THEN** 返回NFS3ERR\_INVAL/NFS3ERR\_NOENT/NFS3ERR\_NAMETOOLONG

***

### Requirement: NFSv3 读写操作测试

系统 SHALL 测试READ/WRITE过程的数据完整性。

#### Scenario: READ读取整个小文件

* **GIVEN** 包含已知内容的文件句柄

* **WHEN** READ(fh, offset=0, count=1024)

* **THEN** 返回data长度等于实际文件大小

* **AND** data内容一致，eof=TRUE

#### Scenario: WRITE写入新文件并回验

* **GIVEN** CREATE创建的新文件句柄

* **WHEN** WRITE(fh, offset=0, count=1024, stable=DATA\_SYNC, data)

* **THEN** 返回count=1024

* **AND** 后续READ验证数据完全一致

#### Scenario: READ/WRITE错误路径

* **GIVEN** 目录句柄当作文件读取

* **WHEN** READ(dir\_fh, ...)

* **THEN** 返回NFS3ERR\_ISDIR

***

### Requirement: NFSv3 创建删除操作测试

系统 SHALL 测试CREATE/MKDIR/REMOVE/RMDIR的语义正确性。

#### Scenario: CREATE三种模式

* **UNCHECKED**: 创建新文件或截断已存在文件

* **GUARDED**: 仅创建新文件，已存在则返回NFS3ERR\_EXIST

* **EXCLUSIVE**: 幂等创建（行为因实现而异）

#### Scenario: MKDIR/REMOVE/RMDIR正常与错误路径

* **MKDIR**: 成功创建目录（type=NF3DIR），父目录无效时返回STALE/IO

* **REMOVE**: 删除成功，不存在时返回NOENT

* **RMDIR**: 删除空目录成功，非空返回NOTEMPTY，不存在返回NOENT

***

### Requirement: NFSv3 目录枚举操作测试

系统 SHALL 测试READDIR/READDIRPLUS的分页和cookie机制。

#### Scenario: READDIR基本枚举

* **GIVEN** 根目录句柄

* **WHEN** READDIR(cookie=0, cookieverf=0, count=8192)

* **THEN** 返回entry列表包含"."和".."

* **AND** eof=TRUE（如果一次读完）

#### Scenario: READDIR分页与错误

* **分页**: 大目录使用cookie遍历完所有条目无遗漏重复

* **TOOSMALL**: count太小(<512)返回NFS3ERR\_TOOSMALL

#### Scenario: READDIRPLUS带属性

* **WHEN** READDIRPLUS(dir, cookie=0, dircount=1024, maxcount=8192)

* **THEN** 返回entry列表及每个条目的fh, name, attrs

* **AND** dircount/maxcount太小时返回TOOSMALL

***

### Requirement: NFSv3 其他操作测试

系统 SHALL 测试ACCESS/READLINK/SYMLINK/MKNOD/RENAME/LINK/PATHCONF/FSSTAT/FSINFO/COMMIT。

每个过程至少覆盖：

* 正常路径（成功调用）

* 主要错误路径（1-2个典型错误码）

***

### Requirement: RPC错误处理测试

系统 SHALL 全面测试RPC层的各种错误场景。

#### Scenario: 协议级错误

* **无效RPC版本号**(0x12345678): 期望 MSG\_DENIED + RPC\_MISMATCH

* **未知程序号**(0x12345678): 期望 PROG\_UNAVAIL

* **无效NFS版本号**(0x12345678): 期望 PROG\_MISMATCH (low=high=3)

* **无效过程号**(0x12345678): 期望 PROC\_UNAVAIL

#### Scenario: 传输层行为

* **乱序响应**: 快速发送XID=100和XID=1，正确匹配每个响应

* **XID不匹配检测**: 框架能检测并报告XID不一致

* **超时重试**: 可配置超时时间，超时后返回错误

***

### Requirement: 边界条件和压力测试

系统 SHALL 测试边界值和异常输入的处理能力。

#### Scenario: 文件名边界

* \*\*零长度名称"": 返回 INVAL/NOENT

* **最大合法名称(255字符)**: 成功（如果FS支持）

* **超长名称(>1024字节)**: 返回 NAMETOOLONG

* **含null字节名称**: 返回 INVAL

#### Scenario: 并发压力

* **100并发请求**: 全部完成无死锁崩溃

* **重复XID请求**: 幂等处理正确

***

## MODIFIED Requirements

（无 - 这是全新项目）

## REMOVED Requirements

（无 - 这是全新项目）

## 关键实现细节

### XDR编解码器设计（核心创新点）

```cpp
// 示例：模板化XDR编解码器（xdr_codec.hpp）
namespace nfs3 {
namespace xdr {

class XdrBuffer {
    std::vector<uint8_t> buffer_;
    size_t pos_ = 0;
public:
    // 基础类型编解码
    void pack(int32_t val);       // 有符号整数
    void pack(uint32_t val);      // 无符号整数
    void pack(uint64_t val);      // 64位整数
    void pack(const std::string& s);  // 变长字符串(4字节长度+数据)
    void pack(std::span<const uint8_t>);  // 不透明数据(opaque)
    
    void unpack(int32_t& val);
    void unpack(uint32_t& val);
    void unpack(uint64_t& val);
    void unpack(std::string& s);
    void unpack(std::vector<uint8_t>&);
    
    // 复合类型特化（在nfs3_types中为每种结构体提供）
    template<typename T>
    void pack(const T& obj);  // 要求T有serialize()方法
    
    template<typename T>
    void unpack(T& obj);      // 要求T有deserialize()方法
    
    const auto& data() const { return buffer_; }
    size_t size() const { return buffer_.size(); }
};

} // namespace xdr
} // namespace nfs3
```

```cpp
// 示例：NFSv3数据结构定义（nfs3_types.hpp）
namespace nfs3 {

enum class ftype3 : uint32_t {
    NF3REG = 1, NF3DIR = 2, NF3BLK = 3,
    NF3CHR = 4, NF3LNK = 5, NF3SOCK = 6,
    NF3FIFO = 7
};

struct nfstime3 {
    uint32_t seconds;
    uint32_t nseconds;
    
    void serialize(xdr::XdrBuffer& buf) const {
        buf.pack(seconds);
        buf.pack(nseconds);
    }
    
    void deserialize(xdr::XdrBuffer& buf) {
        buf.unpack(seconds);
        buf.unpack(nseconds);
    }
};

struct fattr3 {
    ftype3 type_;
    uint32_t mode;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t used;
    specdata3 rdev;  // 嵌套结构体
    uint64_t fsid;
    uint64_t fileid;
    nfstime3 atime, mtime, ctime;
    
    void serialize(xdr::XdrBuffer& buf) const;
    void deserialize(xdr::XdrBuffer& buf);
};

// ... 所有RFC 1813定义的结构体类似实现

} // namespace nfs3
```

### RPCEndpoint 设计（基于libtirpc，TCP专用）

```cpp
// rpc_endpoint.hpp - PIMPL模式隐藏libtirpc细节（仅支持TCP）
namespace nfs3 {

class RPCEndpointImpl;  // 前向声明实现类

class RPCEndpoint {
    std::unique_ptr<RPCEndpointImpl> impl_;
public:
    static RPCEndpoint create(
        const std::string& host,
        uint16_t port = 2049,
        std::chrono::milliseconds timeout = std::chrono::seconds(25)
    );
    
    ~RPCEndpoint();
    
    // 禁止拷贝，允许移动
    RPCEndpoint(const RPCEndpoint&) = delete;
    RPCEndpoint& operator=(const RPCEndpoint&) = delete;
    RPCEndpoint(RPCEndpoint&&) noexcept;
    RPCEndpoint& operator=(RPCEndpoint&&) noexcept;
    
    // 泛型RPC调用
    template<typename ResType>
    std::expected<ResType, RpcError> call(
        uint32_t proc_num,
        const auto& args  // 任何可序列化的参数
    );
    
    void shutdown();
};

} // namespace nfs3
```

### NFS3TestClient API设计

```cpp
// nfs3_client.hpp - 类型安全的22个过程API
namespace nfs3 {

class NFS3TestClient {
    RPCEndpoint endpoint_;
    nfs_fh3 root_fh_;  // 零长度或预设的根句柄
    
public:
    explicit NFS3TestClient(RPCEndpoint ep);
    
    // ====== 过程0: NULL ======
    std::expected<void, Nfs3Error> null();
    
    // ====== 过程1: GETATTR ======
    std::expected<GETATTR3resok, Nfs3Error> getattr(const nfs_fh3& object);
    
    // ====== 过程2: SETATTR ======
    std::expected<SETATTR3resok, Nfs3Error> setattr(
        const nfs_fh3& object,
        const sattr3& new_attributes,
        const SetattrGuard& guard
    );
    
    // ====== 过程3: LOOKUP ======
    std::expected<LOOKUP3resok, Nfs3Error> lookup(
        const nfs_fh3& dir,
        std::string_view name
    );
    
    // ====== 过程4-21: 类似接口... ======
    std::expected<ACCESS3resok, Nfs3Error> access(/* ... */);
    std::expected<READLINK3resok, Nfs3Error> readlink(/* ... */);
    std::expected<READ3resok, Nfs3Error> read(/* ... */);
    std::expected<WRITE3resok, Nfs3Error> write(/* ... */);
    std::expected<CREATE3resok, Nfs3Error> create(/* ... */);
    // ... 其余过程
    
    // 便捷方法
    const nfs_fh3& root_handle() const { return root_fh_; }
    void set_root_handle(const nfs_fh3& fh) { root_fh_ = fh; }
};

} // namespace nfs3
```

### CMake 构建系统设计（CMake 2.8兼容）

```cmake
# CMakeLists.txt (顶层) - 兼容CMake 2.8+
cmake_minimum_required(VERSION 2.8)
project(nfs3_rpc_tests VERSION 1.0 LANGUAGES CXX)

# C++17标准 (CMake 2.8兼容写法)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -Wextra -Wpedantic")

# Debug/Release配置
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -DDEBUG")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -DNDEBUG")
endif()

# 查找libtirpc依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBTIRPC REQUIRED libtirpc)

# 可选：Google Test
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        message(STATUS "GTest not found, using bundled version")
        add_subdirectory(third_party/googletest)
    endif()
endif()

# 核心库
add_library(nfs3_test_core STATIC
    src/rpc_endpoint.cpp
    src/nfs3_client.cpp
    src/test_context.cpp
    src/xdr_codec.cpp
    src/detail/rpc_msg.cpp
    src/detail/xdr_primitive.cpp
)

include_directories(nfs3_test_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${LIBTIRPC_INCLUDE_DIRS}
)
target_link_libraries(nfs3_test_core PUBLIC
    ${LIBTIRPC_LIBRARIES}
    pthread
)

# 测试可执行文件
if(BUILD_TESTS AND TARGET GTest::gtest)
    add_subdirectory(tests)
endif()
```

***

## 编译与运行

### 环境准备

```bash
# Ubuntu/Debian (最小依赖)
sudo apt update && sudo apt install -y \
    g++ cmake pkg-config libtirpc-dev

# 可选：测试框架
sudo apt install -y libgtest-dev
```

### 构建

```bash
# 配置
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# 编译核心库（不需要gtest也能工作）
cmake --build build --target nfs3_test_core

# 编译并运行测试（需要gtest）
cmake --build build
cd build && ctest --output-on-failure
```

### 运行

```bash
# 运行全部测试
./build/test_nfs3_suite

# 运行特定测试
./build/test_nfs3_suite --gtest_filter="RpcNullTest.*"

# 详细输出
./build/test_nfs3_suite --gtest_print_time=1

# 参数化：指定服务器地址
./build/test_nfs3_lookup --server=192.168.1.100 --port=2049
```

