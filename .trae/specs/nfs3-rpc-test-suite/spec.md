# NFSv3 RPC接口测试套件设计规范

## Why

当前缺乏一套直接针对NFSv3服务器RPC接口的C/C++测试框架。现有的cthon-nfs-tests通过POSIX系统调用间接测试（无法精确控制RPC字段），而Vaiz/nfs3虽提供参考但使用Rust实现且覆盖不全面。本规范旨在设计一套**直接构造和发送RPC包**的NFSv3接口测试套件，用于验证服务器对RFC 1813协议的合规性、错误处理能力和边界条件处理。

## What Changes

- 创建一套C/C++实现的NFSv3 RPC测试框架
- 直接在RPC层面构造请求/验证响应（绕过内核NFS客户端）
- 覆盖NFSv3全部22个RPC过程的正常路径、错误路径和边界条件
- 提供可扩展的测试基础设施（连接管理、XDR编解码、断言宏）

## Impact

- **目标系统**: 任意NFSv3服务器实现（用户空间或内核态）
- **测试层次**: RPC协议层 + NFSv3操作语义层
- **通信方式**: TCP/UDP直连服务器2049端口（无需mount/portmap）
- **依赖库**: libtirpc（XDR/RPC运行时）

## 架构设计

### 分层架构

```
┌─────────────────────────────────────────────┐
│              测试用例层 (Test Cases)          │
│  rpc_null_test.c   nfs3_readwrite_test.c    │
│  rpc_error_test.c  nfs3_lookup_test.c       │
│  ...                                        │
├─────────────────────────────────────────────┤
│              测试框架层 (Test Framework)      │
│  ┌─────────────┐  ┌──────────────────────┐  │
│  │ TestContext │  │  NFS3TestClient      │  │
│  │ (生命周期)  │  │  (高级API封装)       │  │
│  └─────────────┘  └──────────┬───────────┘  │
│  ┌─────────────┐           │               │
│  │ Assert Macros│          ▼               │
│  │ (断言宏)    │  ┌───────────────────┐    │
│  └─────────────┘  │ RPCEndpoint       │    │
│                   │ (底层RPC收发)     │    │
│                   └─────────┬─────────┘    │
├─────────────────────────────┼───────────────┤
│              协议层 (Protocol Layer)        │
│  ┌────────────┐  ┌────────┴────────┐       │
│  │ XDR Codec  │  │ RFC 1813 Types  │       │
│  │ (编解码)   │  │ (NFS3数据结构)  │       │
│  └────────────┘  └─────────────────┘       │
├─────────────────────────────────────────────┤
│              传输层 (Transport)             │
│         TCP Socket / UDP Socket            │
└─────────────────────────────────────────────┘
```

### 核心组件职责

| 组件 | 文件 | 职责 |
|------|------|------|
| **RPCEndpoint** | `rpc_endpoint.h/c` | 管理TCP/UDP连接，收发RPC消息片段 |
| **NFS3TestClient** | `nfs3_client.h/c` | 封装所有22个NFSv3过程的调用API |
| **TestContext** | `test_context.h/c` | 管理测试生命周期（setup/teardown） |
| **Assert Macros** | `nfs3_assert.h` | 提供NFS特定的断言宏 |
| **XDR Helpers** | `xdr_helpers.h/c` | 辅助XDR编解码的工具函数 |

## ADDED Requirements

### Requirement: RPC连接管理

系统 SHALL 提供可靠的RPC连接管理能力，支持TCP和UDP两种传输方式。

#### Scenario: 建立TCP连接到NFS服务器
- **GIVEN** 一个运行中的NFSv3服务器（地址:port）
- **WHEN** 调用 `rpc_endpoint_create("tcp", "192.168.1.100", 2049)`
- **THEN** 成功建立TCP连接并返回有效的endpoint句柄
- **AND** 连接可用于后续RPC调用

#### Scenario: 发送RPC消息并接收响应
- **GIVEN** 一个已建立的RPC endpoint
- **WHEN** 调用 `rpc_call(endpoint, xid, proc, args, &reply)`
- **THEN** 正确封装RPC消息（fragment header + RPC msg + XDR参数）
- **AND** 成功接收并解析服务端响应
- **AND** 返回RPC状态码和NFS过程特定结果

#### Scenario: 处理网络超时
- **GIVEN** 一个已建立的RPC endpoint
- **WHEN** 服务端在指定超时时间内未响应
- **THEN** 返回 `RPC_TIMEOUT` 错误
- **AND** 连接保持有效（可重试）

---

### Requirement: NFSv3 NULL过程测试

系统 SHALL 测试NFSPROC3_NULL（过程号0）的正确性。

#### Scenario: NULL请求正常响应
- **GIVEN** 连接到NFSv3服务器
- **WHEN** 发送NULL请求（无参数）
- **THEN** 服务器返回成功响应（无返回数据）
- **AND** XID匹配请求

#### Scenario: 顺序多个NULL请求
- **GIVEN** 连接到NFSv3服务器
- **WHEN** 连续发送10个不同XID的NULL请求
- **THEN** 每个请求都收到匹配XID的成功响应
- **AND** 响应顺序与请求顺序一致

#### Scenario: 并发NULL请求
- **GIVEN** 连接到NFSv3服务器
- **WHEN** 同时发送5个不同XID的NULL请求
- **THEN** 所有请求都收到正确响应
- **AND** 每个响应对应正确的XID

#### Scenario: NULL请求带额外数据（容错性）
- **GIVEN** 连接到NFSv3服务器
- **WHEN** 发送NULL请求并附加4字节数据
- **THEN** 服务器忽略额外数据并返回成功（符合RFC 1813 Section 3.0）

---

### Requirement: NFSv3 属性操作测试

系统 SHALL 测试GETATTR、SETATTR过程的正确性和错误处理。

#### Scenario: GETATTR获取根目录属性
- **GIVEN** 已获取根目录文件句柄（通过MOUNT或硬编码）
- **WHEN** 调用GETATTR(root_fh)
- **THEN** 返回成功的fattr3结构
- **AND** type字段为NF3DIR
- **AND** size >= 0

#### Scenario: GETATTR无效句柄错误
- **GIVEN** 一个无效的文件句柄（全零或随机）
- **WHEN** 调用GETATTR(invalid_fh)
- **THEN** 返回NFS3ERR_BADHANDLE错误

#### Scenario: SETATTR修改文件大小
- **GIVEN** 一个已存在的普通文件句柄
- **WHEN** 调用SETATTR设置size=0（截断文件）
- **THEN** 返回成功
- **AND** 后续GETATTR确认size=0

#### Scenario: SETATTR修改时间戳
- **GIVEN** 一个已存在的文件句柄
- **WHEN** 调用SETATTR设置mtime为指定时间
- **THEN** 返回成功
- **AND** GETATTR确认mtime已更新

---

### Requirement: NFSv3 查找操作测试

系统 SHALL 测试LOOKUP过程的路径解析能力。

#### Scenario: LOOKUP查找根目录自身(".")
- **GIVEN** 根目录文件句柄
- **WHEN** LOOKUP(root, ".")
- **THEN** 返回的对象句柄等于root句柄

#### Scenario: LOOKUP查找已存在文件
- **GIVEN** 根目录句柄和已知存在的文件名"a.txt"
- **WHEN** LOOKUP(root, "a.txt")
- **THEN** 返回该文件的句柄和属性
- **AND** obj_attributes.type = NF3REG

#### Scenario: LOOKUP查找不存在文件
- **GIVEN** 根目录句柄和不存在的文件名"nonexist.txt"
- **WHEN** LOOKUP(root, "nonexist.txt")
- **THEN** 返回NFS3ERR_NOENT错误

#### Scenario: LOOKUP查找跨目录文件
- **GIVEN** 根目录句柄，已知子目录"subdir"及其下文件"file.txt"
- **WHEN** 先LOOKUP(root, "subdir")获取dir_handle
- **AND** 再LOOKUP(dir_handle, "file.txt")
- **THEN** 成功返回file.txt的句柄

#### Scenario: LOOKUP空名称错误
- **GIVEN** 任意目录句柄
- **WHEN** LOOKUP(dir, "")
- **THEN** 返回NFS3ERR_INVAL或NFS3ERR_NOENT错误

#### Scene: LOOKUP超长文件名（>255字节）
- **GIVEN** 目录句柄
- **WHEN** LOOKUP(dir, "256字节长的文件名...")
- **THEN** 返回NFS3ERR_NAMETOOLONG或NFS3ERR_IO错误

---

### Requirement: NFSv3 读写操作测试

系统 SHALL 测试READ/WRITE过程的数据完整性。

#### Scenario: READ读取整个小文件
- **GIVEN** 一个包含已知内容（如"hello world\n"，12字节）的文件句柄
- **WHEN** READ(fh, offset=0, count=1024)
- **THEN** 返回data长度=12
- **AND** data内容与写入时一致
- **AND** eof=TRUE（如果offset+count >= file_size）

#### Scenario: READ偏移读取
- **GIVEN** 一个包含"abcdefghij"(10字节)的文件
- **WHEN** READ(fh, offset=3, count=4)
- **THEN** 返回data="defg"

#### Scenario: READ超出文件末尾
- **GIVEN** 一个100字节的文件
- **WHEN** READ(fh, offset=200, count=50)
- **THEN** 返回NFS3ERR_INVAL或eof=TRUE且data为空

#### Scenario: READ目录当作文件
- **GIVEN** 一个目录句柄
- **WHEN** READ(dir_fh, offset=0, count=1024)
- **THEN** 返回NFS3ERR_ISDIR错误

#### Scenario: WRITE写入新文件
- **GIVEN** 通过CREATE创建的新文件句柄
- **WHEN** WRITE(fh, offset=0, count=1024, stable=DATA_SYNC, data)
- **THEN** 返回count=1024
- **AND** committed = FILE_SYNC 或 DATA_SYNC
- **AND** 后续READ验证数据一致性

#### Scenario: WRITE追加写入
- **GIVEN** 已有100字节的文件句柄
- **WHEN** WRITE(fh, offset=100, count=50, data)
- **THEN** 返回count=50
- **AND** 文件总大小变为150字节

#### Scenario: WRITE覆盖写入
- **GIVEN** 已有数据的文件句柄
- **WHEN** WRITE(fh, offset=0, count=原数据长度, 新数据)
- **THEN** 原数据被完全覆盖
- **AND** READ验证新数据

#### Scenario: UNSTABLE写入后COMMIT
- **GIVEN** 支持UNSTABLE写入的服务器
- **WHEN** WRITE(..., stable=UNSTABLE)
- **AND** COMMIT(fh, offset=0, count=written_bytes)
- **THEN** COMMIT返回成功表示数据已持久化

---

### Requirement: NFSv3 创建删除操作测试

系统 SHALL 测试CREATE/MKDIR/REMOVE/RMDIR等修改操作的语义正确性。

#### Scenario: CREATE创建新文件(UNCHECKED模式)
- **GIVEN** 目录句柄和新文件名"newfile.txt"
- **WHEN** CREATE(dir, "newfile.txt", UNCHECKED, attrs)
- **THEN** 返回新文件句柄
- **AND** LOOKUP可找到该文件
- **AND** 如果文件已存在则截断为0长度

#### Scenario: CREATE创建新文件(GUARDED模式)
- **GIVEN** 目录句柄和不存在的文件名
- **WHEN** CREATE(dir, "newfile.txt", GUARDED, attrs)
- **THEN** 成功创建并返回句柄

#### Scenario: GUARDED模式文件已存在失败
- **GIVEN** 目录句柄和已存在的文件名
- **WHEN** CREATE(dir, "existing.txt", GUARDED, attrs)
- **THEN** 返回NFS3ERR_EXIST错误

#### Scenario: EXCLUSIVE模式创建
- **GIVEN** 目录句柄
- **WHEN** CREATE(dir, "exclusive.txt", EXCLUSIVE, verf)
- **THEN** 如果不存在则创建（行为可能因实现而异）

#### Scenario: MKDIR创建目录
- **GIVEN** 目录句柄
- **WHEN** MKDIR(dir, "newdir", attrs)
- **THEN** 返回新目录句柄
- **AND** GETATTR显示type=NF3DIR

#### Scenario: MKDIR父目录不存在
- **GIVEN** 不存在的目录句柄
- **WHEN** MKDIR(invalid_dir, "sub", attrs)
- **THEN** 返回NFS3ERR_STALE或NFS3ERR_IO等错误

#### Scenario: REMOVE删除已存在文件
- **GIVEN** 包含"a.txt"的目录句柄
- **WHEN** REMOVE(dir, "a.txt")
- **THEN** 返回成功
- **AND** 后续LOOKUP返回NFS3ERR_NOENT

#### Scenario: REMOVE删除不存在的文件
- **GIVEN** 目录句柄
- **WHEN** REMOVE(dir, "ghost.txt")
- **THEN** 返回NFS3ERR_NOENT错误

#### Scenario: RMDIR删除空目录
- **GIVEN** 先创建的空目录句柄及其父目录
- **WHEN** RMDIR(parent_dir, "empty_dir")
- **THEN** 删除成功

#### Scenario: RMDIR非空目录失败
- **GIVEN** 包含子项的目录
- **WHEN** RMDIR(parent, "nonempty_dir")
- **THEN** 返回NFS3ERR_NOTEMPTY错误

#### Scenario: RMDIR不存在的目录
- **GIVEN** 目录句柄
- **WHEN** RMDIR(dir, "no_such_dir")
- **THEN** 返回NFS3ERR_NOENT错误

---

### Requirement: NFSv3 目录枚举操作测试

系统 SHALL 测试READDIR/READDIRPLUS的分页和cookie机制。

#### Scenario: READDIR读取根目录
- **GIVEN** 根目录句柄（已知包含a.txt, b.txt, subdir/）
- **WHEN** READDIR(root, cookie=0, cookieverf=0, count=8192)
- **THEN** 返回entry列表包含"."和".."
- **AND** 包含已知文件/子目录
- **AND** eof=TRUE（如果一次读完）

#### Scenario: READDIR分页读取
- **GIVEN** 包含200+文件的目录
- **WHEN** 第一次READDIR(cookie=0, count=1024)
- **AND** 使用返回的last cookie继续READDIR直到eof
- **THEN** 所有文件都被枚举
- **AND** 无重复条目

#### Scenario: READDIR cookieverf验证
- **GIVEN** 目录句柄
- **WHEN** 第一次READDIR获取cookieverf
- **AND** 第二次使用相同cookieverf继续读取
- **THEN** 成功返回后续条目

#### Scenario: READDIR count太小错误
- **GIVEN** 任意目录
- **WHEN** READDIR(dir, cookie=0, count=10) // 太小无法容纳header
- **THEN** 返回NFS3ERR_TOOSMALL错误

#### Scenario: READDIRPLUS带属性读取
- **GIVEN** 包含文件的目录
- **WHEN** READDIRPLUS(dir, cookie=0, dircount=1024, maxcount=8192)
- **THEN** 返回entry列表及每个条目的文件属性（fh, name, attrs）
- **AND** 属性信息与独立GETATTR结果一致

#### Scenario: READDIRPLUS dircount/maxcount太小
- **GIVEN** 目录句柄
- **WHEN** READDIRPLUS(dir, dircount=10, maxcount=8192) // dircount太小
- **THEN** 返回NFS3ERR_TOOSMALL

---

### Requirement: NFSv3 其他操作测试

系统 SHALL 测试ACCESS/READLINK/SYMLINK/MKNOD/RENAME/LINK/PATHCONF/FSSTAT/FSINFO/COMMIT。

#### Scenario: ACCESS检查读权限
- **GIVEN** 文件句柄
- **WHEN** ACCESS(fh, ACCESS3_READ)
- **THEN** 返回access掩码包含ACCESS3_READ位

#### Scenario: READLINK读取符号链接
- **GIVEN** 符号链接句柄
- **WHEN** READLINK(symlink_fh)
- **THEN** 返回链接目标路径

#### Scenario: READLINK对非符号链接
- **GIVEN** 普通文件或目录句柄
- **WHEN** READLINK(non_symlink)
- **THEN** 返回NFS3ERR_INVAL错误

#### Scenario: SYMLINK创建符号链接
- **GIVEN** 目录句柄
- **WHEN** SYMLINK(dir, "link_name", target_path, attrs)
- **THEN** 创建符号链接
- **AND** READLINK返回target_path

#### Scenario: MKNOD创建FIFO
- **GIVEN** 目录句柄
- **WHEN** MKNOD(dir, "pipe", NF3FIFO, attrs)
- **THEN** 创建FIFO特殊文件

#### Scenario: RENAME重命名文件
- **GIVEN** 源目录含"old.txt"，目标目录
- **WHEN** RENAME(from_dir, "old.txt", to_dir, "new.txt")
- **THEN** from_dir中"old.txt"消失
- **AND** to_dir中出现"new.txt"
- **AND** 文件句柄保持不变（某些实现）

#### Scenario: LINK创建硬链接
- **GIVEN** 已存在的文件句柄和目标目录
- **WHEN** LINK(file_fh, target_dir, "hardlink")
- **THEN** 创建硬链接指向同一inode
- **AND** 两路径GETATTR返回相同fileid

#### Scenario: PATHCONF查询路径配置
- **GIVEN** 任意文件或目录句柄
- **WHEN PATHCONF(obj)
- **THEN** 返回pathconf信息（linkmax, namemax, notrunc, chownrestricted, etc.）

#### Scenario: FSSTAT查询文件系统统计
- **GIVEN** 根目录句柄
- **WHEN** FSSTAT(root)
- **THEN** 返回total_bytes, free_bytes, avail_bytes, total_files, free_files, avail_files, invarsec

#### Scenario: FSINFO查询文件系统能力
- **GIVEN** 根目录句柄
- **WHEN** FSINFO(root)
- **THEN** 返回rtmax, rtpref, wtmmax, wtpref, dtpref, maxfilesize, time_delta, properties

#### Scenario: COMMIT提交数据
- **GIVEN** 通过UNSTABLE WRITE写入的文件
- **WHEN** COMMIT(fh, offset=0, count=file_size)
- **THEN** 数据已持久化到存储

---

### Requirement: RPC错误处理测试

系统 SHALL 全面测试RPC层的各种错误场景（参考Vaiz/nfs3 rpc_tests.rs）。

#### Scenario: 无效RPC版本号
- **GIVEN** 连接到NFS服务器
- **WHEN** 发送rpcvers=0x12345678的CALL消息
- **THEN** 服务器返回MSG_DENIED + RPC_MISMATCH
- **AND** low/high指示支持的版本范围（应为2）

#### Scenario: 未知程序号
- **GIVEN** 连接到NFS服务器
- **WHEN** 发送prog=0x12345678的CALL消息
- **THEN** 返回MSG_ACCEPTED + PROG_UNAVAIL

#### Scenario: 无效NFS版本号
- **GIVEN** 连接到NFS服务器
- **WHEN** 发送vers=0x12345678的NFS CALL
- **THEN** 返回MSG_ACCEPTED + PROG_MISMATCH
- **AND** low/high指示支持版本（应为3）

#### Scenario: 无效过程号
- **GIVEN** 连接到NFS服务器
- **WHEN** 发送proc=0x12345678的NFSv3 CALL
- **THEN** 返回MSG_ACCEPTED + PROC_UNAVAIL

#### Scenario: XID不匹配检测
- **GIVEN** 连接到NFS服务器
- **WHEN** 发送请求后故意修改期望的XID进行比对
- **THEN** 框架能检测到XID不匹配并报告错误

#### Scenario: 乱序响应处理
- **GIVEN** 连接到NFS服务器
- **WHEN** 快速连续发送XID=100和XID=1的两个请求
- **THEN** 能正确匹配每个响应对应的XID（即使乱序到达）

---

### Requirement: 边界条件和压力测试

系统 SHALL 测试边界值和异常输入的处理能力。

#### Scenario: 零长度文件名
- **GIVEN** 目录句柄
- **WHEN** LOOKUP/CREATE/REMOVE("", ...)
- **THEN** 返回适当的错误码（NFS3ERR_INVAL或NOENT）

#### Scenario: 最大长度文件名（255字符）
- **GIVEN** 目录句柄
- **WHEN** CREATE(dir, "255字符长的合法文件名...", attrs)
- **THEN** 成功创建（如果文件系统支持）

#### Scenario: 超长文件名（>1024字节）
- **GIVEN** 目录句柄
- **WHEN** LOOKUP(dir, "超长文件名...")
- **THEN** 返回NFS3ERR_NAMETOOLONG或优雅降级

#### Scenario: 特殊字符文件名
- **GIVEN** 目录句柄
- **WHEN** CREATE(dir, "file with spaces/tabs/\0", attrs)
- **THEN** 根据RFC 1813，文件名为UTF-8字符串（不含null字节）
- **AND** null字节应导致NFS3ERR_INVAL

#### Scenario: 大量并发请求（压力测试）
- **GIVEN** 连接到NFS服务器
- **WHEN** 同时发起100个并发READ/WRITE请求
- **THEN** 所有请求最终完成（无死锁/崩溃）
- **AND** 响应时间在合理范围内

#### Scenario: 快速重复请求（幂等性）
- **GIVEN** 连接到NFS服务器
- **WHEN** 相同XID的请求发送两次（网络重传模拟）
- **THEN** 服务器正确处理（幂等操作返回相同结果）

---

## MODIFIED Requirements

（无 - 这是全新项目）

## REMOVED Requirements

（无 - 这是全新项目）

## 技术选型说明

### 为什么选择libtirpc而非传统libc RPC？

| 特性 | libc内置RPC | libtirpc |
|------|------------|----------|
| 维护状态 | ❌ 已从glibc移除 | ✅ 活跃维护 |
| IPv6支持 | ❌ 仅IPv4 | ✅ 完整支持 |
| RPCSEC_GSS | ❌ 不支持 | ✅ 支持 |
| 现代编译器兼容 | ⚠️ 警告较多 | ✅ C99/C11兼容 |
| 可移植性 | ⚠️ 依赖glibc版本 | ✅ 独立包 |

### 测试组织结构

```
nfs3_rpc_tests/
├── include/
│   ├── rpc_endpoint.h          # RPC连接管理
│   ├── nfs3_client.h           # NFSv3客户端API
│   ├── test_context.h          # 测试上下文
│   ├── nfs3_assert.h           # 断言宏
│   ├── xdr_helpers.h           # XDR辅助函数
│   └── nfs3_types.h            # NFSv3数据类型（从nfs_prot.x生成）
├── src/
│   ├── rpc_endpoint.c          # RPC连接实现
│   ├── nfs3_client.c           # NFSv3过程封装
│   ├── test_context.c          # 测试生命周期
│   └── xdr_helpers.c           # XDR辅助实现
├── tests/
│   ├── test_rpc_null.c         # NULL过程测试
│   ├── test_rpc_errors.c       # RPC错误处理测试
│   ├── test_nfs3_getattr.c     # GETATTR/SETATTR测试
│   ├── test_nfs3_lookup.c      # LOOKUP测试
│   ├── test_nfs3_readwrite.c   # READ/WRITE测试
│   ├── test_nfs3_create.c      # CREATE/MKDIR测试
│   ├── test_nfs3_remove.c      # REMOVE/RMDIR测试
│   ├── test_nfs3_readdir.c     # READDIR/READDIRPLUS测试
│   ├── test_nfs3_other.c       # 其他操作测试
│   └── test_nfs3_stress.c      # 边界条件/压力测试
├── xdr/                        # XDR生成代码
│   ├── nfs_prot.h              # rpcgen生成的头文件
│   ├── nfs_prot_xdr.c          # XDR编解码函数
│   └── nfs_prot_clnt.c         # 客户端桩代码
├── Makefile                    # 构建系统
├── README.md                   # 使用文档
└── run_tests.sh                # 测试运行脚本
```

### 编译与运行

```bash
# 编译
make all

# 运行全部测试
./run_tests.sh --server <IP> --port 2049

# 运行特定测试组
./run_tests.sh --server <IP> --tests null,lookup,readwrite

# 运行单个测试（详细输出）
./test_nfs3_readwrite --server <IP> --port 2049 --verbose
```
