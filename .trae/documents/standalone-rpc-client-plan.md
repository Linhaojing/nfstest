# 自实现 ONC RPC 客户端替代 libtirpc — 工作量评估与实施方案 (修订版)

## 一、背景与目标

项目根目录 `/home/lhj/workspace/nfstest/` 下有两个子项目：

| 子项目 | 语言 | libtirpc 依赖 | 当前状态 |
|--------|------|--------------|---------|
| `nfs3_rpc_tests/` | C++17 | 有条件依赖 (`HAVE_LIBTIRPC`) | RPC 可用，但依赖 libtirpc |
| `nfs3_rpc_tests_c/` | C11 | **无** | 仅有 XDR 编解码，无任何 RPC 传输能力 |

目标：在项目根目录实现一个 **C++ 编写的共享 RPC 客户端库**，同时提供 C++ 原生 API 和 C 语言包装 (extern "C")，使两个子项目都能在不依赖 libtirpc 的情况下完成 NFSv3 RPC 通信。

---

## 二、架构设计

### 2.1 目标目录结构

```
/home/lhj/workspace/nfstest/
├── shared_rpc/                              # 新增：共享 RPC 客户端库
│   ├── CMakeLists.txt                       # 独立构建，产出 shared_rpc 静态库
│   ├── include/
│   │   └── nfstest/
│   │       ├── rpc_protocol.hpp             # ONC RPC 消息类型定义 (C++ header-only)
│   │       ├── rpc_transport.hpp            # TCP 传输 + Record Marking (C++ header-only)
│   │       ├── rpc_client.hpp               # RPC 客户端 C++ 类
│   │       └── rpc_client.h                 # C 语言包装 API (extern "C")
│   └── src/
│       ├── rpc_transport.cpp                # TCP socket / Record Marking 实现
│       └── rpc_client.cpp                   # RPC 客户端实现 + C API 实现
│
├── nfs3_rpc_tests/                          # C++ 项目 (修改部分使用方式)
│   ├── CMakeLists.txt                       # 添加对 shared_rpc 的依赖
│   ├── src/rpc_endpoint.cpp                 # 自实现模式下使用 shared_rpc
│   ├── src/nfs3_client.cpp                  # 切换为使用 custom XdrBuffer 路径
│   └── src/mount_client.cpp                 # 迁移到 shared_rpc
│
└── nfs3_rpc_tests_c/                        # C 项目 (新增 RPC 能力)
    ├── CMakeLists.txt                       # 添加对 shared_rpc 的依赖
    ├── include/nfs3_c/
    │   └── rpc_client.h                     # 引入 shared_rpc 的 C 头文件
    └── tests/                               # 可新增 RPC 集成测试
```

### 2.2 库定位

`shared_rpc` 是一个**纯 RPC 传输库**，不包含任何 NFS 协议知识。它的职责：

- 构建 ONC RPC CALL 消息头 + AUTH_UNIX 认证
- 将调用方的 XDR 编码数据作为 RPC body 封装
- 通过 TCP + Record Marking 发送请求
- 接收并解析 RPC REPLY 消息
- 提取响应的 XDR 数据返回给调用方

每个子项目使用自己的 XDR 编解码器：
- **C++ 项目**：使用 `xdr::XdrBuffer` + `nfs3_types.hpp` 的 `serialize()/deserialize()`
- **C 项目**：使用 `xdr_buf_t` + `nfs3_xdr.h` 的 `xdr_pack_*/xdr_unpack_*` 函数

### 2.3 XDR 角色分离

```
┌─────────────────────────────────────────────────┐
│              shared_rpc (纯传输)                  │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐  │
│  │ RPC 协议  │  │  TCP +   │  │ C API 包装层  │  │
│  │ 消息构造  │  │  Record  │  │ (extern "C")  │  │
│  │ /解析    │  │  Marking │  │               │  │
│  └──────────┘  └──────────┘  └───────────────┘  │
│        ▲              ▲              ▲           │
│        │    发送/接收 raw XDR bytes     │           │
│        └──────────────┼──────────────┘           │
└───────────────────────┼──────────────────────────┘
                        │
          ┌─────────────┴─────────────┐
          │                           │
   ┌──────┴──────┐            ┌──────┴──────┐
   │ C++ 子项目   │            │  C 子项目    │
   │ XdrBuffer   │            │ xdr_buf_t   │
   │ nfs3_types  │            │ nfs3_xdr    │
   └─────────────┘            └─────────────┘
```

---

## 三、需要自实现的组件

### 3.1 ONC RPC 协议层 (RFC 5531 — 仅客户端侧)

| 组件 | 说明 | 复杂度 |
|------|------|--------|
| RPC CALL 消息构造 | xid + msg_type=CALL + rpcvers=2 + prog + vers + proc + cred(AUTH_SYS) + verf(AUTH_NONE) + 程序参数 | 低 |
| RPC REPLY 消息解析 | 校验 xid，解析 accept_stat/reject_stat，提取响应 body | 中 |
| AUTH_SYS 凭证构造 | flavor=AUTH_SYS + stamp + machinename + uid + gid + gids[] | 低 |
| XID 管理 | 简单递增计数器 | 低 |
| RPC 状态码映射 | SUCCESS / PROG_UNAVAIL / PROG_MISMATCH / PROC_UNAVAIL / GARBAGE_ARGS / AUTH_ERROR 等 → 统一错误码 | 低 |

### 3.2 TCP 传输层

| 组件 | 说明 | 复杂度 |
|------|------|--------|
| Socket 连接管理 | `socket()` + `connect()`，RAII 管理文件描述符 | 低 |
| Record Marking 帧封装 | 4 字节大端长度前缀，bit31=LAST_FRAG | 低 |
| 带超时收发 | `poll()` + non-blocking socket 或 `SO_RCVTIMEO` | 中 |
| 大消息分片 | 虽然 NFSv3 消息通常 < 64KB，但协议支持分片 | 低 (预留) |

### 3.3 C API 包装层

| 组件 | 说明 | 复杂度 |
|------|------|--------|
| 不透明句柄 | `typedef struct nfstest_rpc_client nfstest_rpc_client_t` | 低 |
| 连接创建/销毁 | `nfstest_rpc_connect()` / `nfstest_rpc_disconnect()` | 低 |
| RPC 调用接口 | `nfstest_rpc_call(prog, vers, proc, args_data, args_len, &resp_data, &resp_len)` | 中 |
| 错误码 (C enum) | `NFSTEST_RPC_OK`, `NFSTEST_RPC_TIMEOUT`, `NFSTEST_RPC_CONN_ERR` 等 | 低 |

---

## 四、详细实施步骤

### 第 1 步：创建 `shared_rpc/` 项目骨架

**文件**：`shared_rpc/CMakeLists.txt`

```
shared_rpc/
├── CMakeLists.txt          # 构建 shared_rpc 静态库
├── include/nfstest/        # 头文件
└── src/                    # 实现
```

CMake 配置要点：
- C++17 标准
- 产出 `shared_rpc` 静态库
- 只依赖 POSIX (socket/poll)，无需任何第三方库
- 不依赖 libtirpc

**预估**：0.25 人天

### 第 2 步：实现 ONC RPC 协议消息

**文件**：`shared_rpc/include/nfstest/rpc_protocol.hpp` (header-only, ~100 行)

```cpp
namespace nfstest::rpc {

// RPC 消息类型
constexpr uint32_t CALL  = 0;
constexpr uint32_t REPLY = 1;

// 认证 flavor
constexpr uint32_t AUTH_NONE = 0;
constexpr uint32_t AUTH_SYS  = 1;

// 接受状态
enum class AcceptStat : uint32_t {
    SUCCESS       = 0,
    PROG_UNAVAIL  = 1,
    PROG_MISMATCH = 2,
    PROC_UNAVAIL  = 3,
    GARBAGE_ARGS  = 4,
    SYSTEM_ERR    = 5,
};

// 拒绝状态
enum class RejectStat : uint32_t {
    RPC_MISMATCH = 0,
    AUTH_ERROR   = 1,
};

// 错误码
enum class RpcStatus {
    OK,
    CONN_ERR,
    TIMEOUT,
    PROG_UNAVAIL,
    PROG_MISMATCH,
    PROC_UNAVAIL,
    GARBAGE_ARGS,
    AUTH_ERROR,
    PROTO_ERR,
};

// 核心函数
std::vector<uint8_t> build_call(uint32_t xid, uint32_t prog, uint32_t vers,
                                 uint32_t proc, const std::vector<uint8_t>& cred_body,
                                 const std::vector<uint8_t>& args_body);
RpcStatus parse_reply(const std::vector<uint8_t>& reply, uint32_t expected_xid,
                      std::vector<uint8_t>& out_body);
} // namespace
```

**关键设计**：`build_call()` 和 `parse_reply()` 不关心上层传下来的 `args_body` 是什么内容，它只负责封装 ONC RPC 消息头。上层负责将自己的 XDR 编码数据传入。

**预估**：0.5 人天

### 第 3 步：实现 TCP 传输层

**文件**：
- `shared_rpc/include/nfstest/rpc_transport.hpp` (~70 行)
- `shared_rpc/src/rpc_transport.cpp` (~200 行)

```cpp
namespace nfstest::rpc {

class TcpTransport {
public:
    bool connect(const std::string& host, uint16_t port, int timeout_ms);
    void disconnect();
    bool is_connected() const;
    
    // 带超时发送一个 RPC 消息 (自动添加 Record Marking)
    bool send(const std::vector<uint8_t>& message, int timeout_ms);
    
    // 带超时接收一个 RPC 消息 (自动去除 Record Marking)
    bool recv(std::vector<uint8_t>& message, int timeout_ms);
    
private:
    int fd_ = -1;
    // Record Marking 辅助
    static constexpr uint32_t LAST_FRAG = 0x80000000;
    static constexpr uint32_t LEN_MASK  = 0x7FFFFFFF;
};

} // namespace
```

**关键细节**：
- 发送前在数据前添加 4 字节 Record Marking 头 (`htonl(len | LAST_FRAG)`)
- 接收时先读 4 字节获取帧长度，再读 payload
- 使用 `poll()` 实现超时，同时处理 `EINTR`
- 处理 TCP 分片/粘包 (循环读写直到预期字节数)

**预估**：0.5 人天

### 第 4 步：实现 RPC 客户端类 + C API

**文件**：
- `shared_rpc/include/nfstest/rpc_client.hpp` (~60 行)
- `shared_rpc/include/nfstest/rpc_client.h` (~50 行)
- `shared_rpc/src/rpc_client.cpp` (~200 行)

**C++ API**：

```cpp
namespace nfstest::rpc {

class RpcClient {
public:
    using Result = std::pair<RpcStatus, std::vector<uint8_t>>;
    
    bool connect(const std::string& host, uint16_t port, int timeout_ms = 3000);
    void disconnect();
    bool is_connected() const;
    
    // 核心 RPC 调用：传入程序号/版本号/过程号 + XDR 编码的 args
    // 返回 (状态码, XDR 编码的 response data)
    Result call(uint32_t prog, uint32_t vers, uint32_t proc,
                const std::vector<uint8_t>& xdr_args, int timeout_ms);
    
private:
    TcpTransport transport_;
    uint32_t xid_ = 1;
};

} // namespace
```

**C API** (`extern "C"`)：

```c
typedef struct nfstest_rpc_client nfstest_rpc_client_t;

// 返回 handle，失败返回 NULL
nfstest_rpc_client_t* nfstest_rpc_connect(const char* host, uint16_t port, int timeout_ms);

// 断开连接并释放
void nfstest_rpc_disconnect(nfstest_rpc_client_t* client);

// 返回值: 0 成功, <0 失败 (对应 RpcStatus 的错误码)
// resp_data 由库分配，调用者负责 free()
int nfstest_rpc_call(nfstest_rpc_client_t* client,
                     uint32_t prog, uint32_t vers, uint32_t proc,
                     const uint8_t* args_data, size_t args_len,
                     uint8_t** resp_data, size_t* resp_len,
                     int timeout_ms);
```

**预估**：0.5 人天

### 第 5 步：修改 C++ 子项目 (`nfs3_rpc_tests/`)

#### 5.1 CMake 集成

**修改** `nfs3_rpc_tests/CMakeLists.txt`：

- `add_subdirectory(../shared_rpc shared_rpc_build)` 引入 shared_rpc
- 当 `TIRPC_NOT_FOUND` 或 `USE_BUILTIN_RPC=ON` 时，链接 `shared_rpc` 替代 `libtirpc`
- 移除 `pthread` 链接 (shared_rpc 不需要)

#### 5.2 重构 `RPCEndpoint`

**修改** `src/rpc_endpoint.cpp`, `include/nfs3/rpc_endpoint.hpp`：

- 当 `HAVE_LIBTIRPC` 未定义时，`RPCEndpointImpl` 使用 `nfstest::rpc::RpcClient`
- `call()` 方法：XdrBuffer 序列化 args → `RpcClient::call()` → XdrBuffer 反序列化 response
- `call_void()` 方法：发送空 body → 不解析响应
- 移除 `call_with_xdr()` 对 libtirpc XDR filter 的依赖（内部转为 XdrBuffer 路径）

#### 5.3 迁移 `nfs3_client.cpp`

当前 22 个 NFS3 操作方法均使用 `call_with_xdr<Args, Res>(proc, args, xdr_args_func, xdr_res_func)`。

改为使用 `call<Args, Res>(proc, args)`：

```cpp
// 修改前
auto result = endpoint_.call_with_xdr<GETATTR3args, GETATTR3res>(
    proc, args,
    reinterpret_cast<xdrproc_t>(xdr_GETATTR3args),
    reinterpret_cast<xdrproc_t>(xdr_GETATTR3res));

// 修改后
auto result = endpoint_.call<GETATTR3res>(proc, args);
```

`call()` 内部使用 `XdrBuffer::pack(args)` 序列化请求，`XdrBuffer::unpack(response)` 反序列化响应。每个方法改动约 3 行，22 个方法共 ~66 行机械替换。

#### 5.4 迁移 `mount_client.cpp`

MountClient 需要自实现 MOUNT 协议的 XDR 编解码（当前依赖 libtirpc XDR）。

- `fhstatus` 和 `exports_list` 改用 `XdrBuffer` 序列化
- `clnt_create`/`clnt_call` 替换为 `RpcClient::call()`
- 代码量约 ~200 行 (改写)

**预估**：1.5 人天 (含 C++ 侧全部改造)

### 第 6 步：集成到 C 子项目 (`nfs3_rpc_tests_c/`)

#### 6.1 CMake 集成

**修改** `nfs3_rpc_tests_c/CMakeLists.txt`：

- `add_subdirectory(../shared_rpc shared_rpc_build)`
- 链接 `shared_rpc` 到 `nfs3_test_core_c`
- 添加 RPC 相关的测试编译

#### 6.2 新增 C 侧 RPC 辅助代码

**新文件** `nfs3_rpc_tests_c/include/nfs3_c/rpc_wrapper.h` (~50 行)：

```c
// 便捷封装：调用 NFS3 过程
int nfs3_rpc_call(nfstest_rpc_client_t* client,
                  uint32_t nfs_proc,
                  const uint8_t* args_data, size_t args_len,
                  uint8_t** resp_data, size_t* resp_len);
```

#### 6.3 C 侧使用示例

```c
nfstest_rpc_client_t* rpc = nfstest_rpc_connect("127.0.0.1", 2049, 5000);

// 构造 GETATTR 请求
xdr_buf_t args;
xdr_buf_init(&args, 256);
xdr_pack_GETATTR3args(&args, &fh);

// 发送 RPC
uint8_t* resp = NULL;
size_t resp_len = 0;
int rc = nfstest_rpc_call(rpc, NFSPROC3_GETATTR, args.data, args.len, &resp, &resp_len);

// 解析响应
if (rc == 0) {
    xdr_buf_t resp_buf;
    xdr_buf_init_read(&resp_buf, resp, resp_len);
    GETATTR3res_t result;
    xdr_unpack_GETATTR3res(&resp_buf, &result);
    // 使用 result...
}

free(resp);
nfstest_rpc_disconnect(rpc);
```

**预估**：0.75 人天 (含 C 侧集成 + 辅助代码 + 示例测试)

### 第 7 步：测试验证

| 测试项 | 说明 |
|--------|------|
| 单元测试：RPC 协议构造 | 验证 `build_call()` 和 `parse_reply()` 的字节级正确性 |
| 单元测试：Record Marking | 验证帧封装/解封 |
| 集成测试：NULL RPC | 同时验证 C++ 和 C 两侧 |
| 集成测试：NFS GETATTR | 验证完整 RPC 往返路径 |
| 回归测试：libtirpc 模式 | 确保现有 libtirpc 路径不受影响 |
| 跨服务器测试 | 测试 Linux knfsd 兼容性 |

**预估**：1 人天

---

## 五、工作量总览

| 步骤 | 内容 | 预估工作量 | 新增文件 | 修改文件 |
|------|------|-----------|---------|---------|
| 步骤 1 | shared_rpc 项目骨架 | 0.25 人天 | 2 (CMakeLists.txt + 目录) | 0 |
| 步骤 2 | RPC 协议消息 | 0.5 人天 | 1 (rpc_protocol.hpp) | 0 |
| 步骤 3 | TCP 传输层 | 0.5 人天 | 2 (rpc_transport.hpp/cpp) | 0 |
| 步骤 4 | RPC 客户端类 + C API | 0.5 人天 | 3 (rpc_client.hpp/h/cpp) | 0 |
| 步骤 5 | C++ 子项目改造 | 1.5 人天 | 0 | 4 (CMakeLists, rpc_endpoint, nfs3_client, mount_client) |
| 步骤 6 | C 子项目集成 | 0.75 人天 | 2 (rpc_wrapper.h, 测试) | 1 (CMakeLists) |
| 步骤 7 | 测试验证 | 1 人天 | 2-3 (测试文件) | 0 |
| **合计** | | **5 人天** | **10-12 个文件** | **5 个文件** |

> 保守估计：**5-6 人天** (含集成调试和边界情况)

---

## 六、核心代码量估算

| 组件 | 文件 | 行数 |
|------|------|------|
| RPC 协议头 | `shared_rpc/include/nfstest/rpc_protocol.hpp` | ~100 |
| TCP 传输头 | `shared_rpc/include/nfstest/rpc_transport.hpp` | ~70 |
| TCP 传输实现 | `shared_rpc/src/rpc_transport.cpp` | ~200 |
| RPC 客户端头 (C++) | `shared_rpc/include/nfstest/rpc_client.hpp` | ~60 |
| RPC 客户端头 (C) | `shared_rpc/include/nfstest/rpc_client.h` | ~50 |
| RPC 客户端实现 | `shared_rpc/src/rpc_client.cpp` | ~200 |
| shared_rpc CMake | `shared_rpc/CMakeLists.txt` | ~30 |
| C 项目 wrapper | `nfs3_rpc_tests_c/include/nfs3_c/rpc_wrapper.h` | ~50 |
| **新增总计** | | **~760 行** |
| C++ 项目修改 | CMakeLists + rpc_endpoint + nfs3_client + mount_client | ~300 行改动 |
| C 项目修改 | CMakeLists | ~20 行 |

---

## 七、风险评估

| 风险 | 等级 | 缓解措施 |
|------|------|---------|
| ONC RPC 协议正确性 | 中 | RFC 5531 规范明确；参考 Linux kernel SUNRPC / nfs-utils |
| TCP 分片/粘包 | 中 | Record Marking 分帧规则明确；循环读写确保完整性 |
| AUTH_SYS 兼容性 | 低 | 格式固定，参照 RFC 5531 §9.2 |
| C API 内存管理 | 中 | C API 使用调用者分配/库分配 + 调用者释放的明确约定；resp_data 由库 malloc，调用者 free |
| 与不同 NFS 服务器兼容性 | 中 | 多服务器测试 (Linux knfsd / Ganesha / FreeNAS) |
| libtirpc 路径回归 | 低 | libtirpc 路径代码不做改动，仅增加 `#else` 分支 |

---

## 八、简化方案 (最小可行版本)

如果希望更快交付，可做以下简化：

| 简化项 | 影响 | 节约 |
|--------|------|------|
| 不做 MountClient 迁移 | C++ 项目的 mount 功能需 libtirpc | 0.25 人天 |
| 不做 C 项目集成测试 | C 侧 RPC 功能可用但未验证 | 0.5 人天 |
| 不实现 Record Marking 分片 | 单帧 NFSv3 消息 (< 64KB) 已足够 | 0.1 人天 |

简化后预估：**3.5-4 人天**

---

## 九、与 libtirpc 的共存策略

```
┌────────────────────────────────────────┐
│           CMake 构建选项                 │
│                                        │
│  if(TIRPC_FOUND && !USE_BUILTIN_RPC)   │
│    链接 libtirpc (当前行为)              │
│  else                                  │
│    链接 shared_rpc (新增, 零外部依赖)    │
│  endif                                 │
└────────────────────────────────────────┘
```

- libtirpc 可用时仍优先使用
- `-DUSE_BUILTIN_RPC=ON` 强制使用自实现
- libtirpc 不可用时自动 fallback
- 两个子项目共享同一个 CMake 逻辑
