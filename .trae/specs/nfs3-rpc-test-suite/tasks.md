# NFSv3 RPC接口测试套件 - 任务分解（C++17 + CMake 2.8 + 手写XDR + TCP优先）

## Phase 1: 基础设施搭建

- [ ] Task 1: 项目初始化与CMake构建系统配置
  - [ ] 创建目录结构（include/nfs3/, include/nfs3/detail/, src/, src/detail/, tests/, cmake/）
  - [ ] 编写顶层CMakeLists.txt（兼容CMake 2.8，C++17标准）
  - [ ] 编写cmake/FindTirpc.cmake查找模块（pkg-config集成）
  - [ ] 配置编译选项：-std=c++17 -Wall -Wextra -Wpedantic
  - [ ] 添加.gitignore和基本README框架
  - [ ] 验证：空项目可成功 `cmake && make`

- [ ] Task 2: 手写XDR编解码器实现
  - [ ] 实现xdr_codec.hpp/cpp：XdrBuffer类（核心缓冲区管理）
    - [ ] pack/unpack基础类型：int32_t, uint32_t, uint64_t, bool
    - [ ] pack/unpack复合类型：std::string(变长), std::vector<T>(定长数组), std::span<uint8_t>(opaque)
    - [ ] 网络字节序转换辅助函数（htohl, htonl等封装）
  - [ ] 实现detail/xdr_primitive.hpp/cpp：基础类型XDR规则实现
  - [ ] 验证：单元测试确认int/string/opaque的pack→unpack往返一致性

- [ ] Task 3: NFSv3数据结构定义与序列化
  - [ ] 实现nfs3_constants.hpp：程序号、版本号、过程号、错误码枚举
  - [ ] 实现nfs3_types.hpp：RFC 1813全部数据结构体定义
    - [ ] 基础类型：nfs_fh3, nfstime3, ftype3, specdata3
    - [ ] 属性相关：fattr3, sattr3, post_op_attr, wcc_data
    - [ ] 参数结构：LOOKUP3args/res, GETATTR3args/res, READ3args/res, WRITE3args/res 等22个过程
    - [ ] 目录枚举：entry3, dirlist3, entryplus3, dirlistplus3
    - [ ] 文件系统信息：fsstat3res, fsinfo3res, pathconf3res
  - [ ] 为每个struct提供 serialize(XdrBuffer&) 和 deserialize(XdrBuffer&) 方法
  - [ ] 验证：编译通过，所有结构体可通过XDR编解码

- [ ] Task 4: RPC连接管理模块实现（TCP专用）
  - [ ] 实现rpc_endpoint.hpp/cpp：RPCEndpoint类（PIMPL模式）
    - [ ] RPCEndpoint::create(host, port) 静态工厂方法
    - [ ] 内部调用libtirpc clnt_create(host, prog, vers, "tcp")
    - [ ] RAII析构自动调用clnt_destroy()
    - [ ] 移动语义支持，禁止拷贝
  - [ ] 实现泛型RPC调用方法 template<Res> call(proc_num, args)
    - [ ] XID自增分配（从1开始，uint32_t循环）
    - [ ] AUTH_NONE认证设置（默认，后续可扩展AUTH_UNIX）
    - [ ] 调用clnt_call()发送请求并接收响应
    - [ ] 解析RPC响应状态（MSG_ACCEPTED/MSG_DENIED）
    - [ ] 返回 std::expected<ResType, RpcError>
  - [ ] 实现错误码映射：RPC错误 → RpcError枚举
  - [ ] 验证：能连接到真实NFS服务器并发送NULL请求

## Phase 2: 测试框架核心

- [ ] Task 5: NFS3TestClient高级API实现
  - [ ] 实现nfs3_client.hpp/cpp：NFS3TestClient类
    - [ ] 构造函数接收RPCEndpoint，持有root_fh_
    - [ ] 封装全部22个NFSv3过程的类型安全调用接口：
      ```cpp
      // 过程0: NULL
      std::expected<void, Nfs3Error> null();
      
      // 过程1: GETATTR
      std::expected<GETATTR3resok, Nfs3Error> getattr(const nfs_fh3& object);
      
      // 过程3: LOOKUP
      std::expected<LOOKUP3resok, Nfs3Error> lookup(const nfs_fh3& dir, std::string_view name);
      
      // 过程6: READ
      std::expected<READ3resok, Nfs3Error> read(const nfs_fh3& file, uint64 offset, uint32 count);
      
      // 过程7: WRITE
      std::expected<WRITE3resok, Nfs3Error> write(const nfs_fh3& file, uint64 offset,
                                                   uint32 count, stable_how stable, const bytes& data);
      // ... 其余18个过程类似
      ```
    - [ ] 统一错误处理：将nfsstat3错误码转换为Nfs3Error枚举
    - [ ] 提供便捷方法：set_root_handle(), root_handle()
  - [ ] 验证：每个过程API编译通过且签名正确

- [ ] Task 6: TestContext测试上下文实现（GTest集成）
  - [ ] 实现test_context.hpp/cpp：NFS3TestContext类（继承::testing::Test或作为Fixture）
    - [ ] SetUp()：解析命令行参数(--server, --port)，创建RPCEndpoint，创建NFS3TestClient
    - [ ] TearDown()：shutdown连接，清理资源
    - [ ] 提供 client() 和 endpoint() 访问器
  - [ ] 支持命令行参数（使用gtest的--gmock_flag或自定义解析）：
    - [ ] --server=<host> (必需)
    - [ ] --port=<2049> (可选，默认2049)
    - [ ] --verbose (可选详细输出)
  - [ ] 日志集成：支持spdlog或简单printf日志
  - [ ] 验证：GTest Fixture正常工作，SetUp/TearDown正确执行

## Phase 3: RPC层测试用例

- [ ] Task 7: NULL过程测试套件（test_rpc_null.cpp）
  - [ ] 实现NULL请求正常响应测试（验证Void返回）
  - [ ] 实现顺序多请求测试（连续10个不同XID）
  - [ ] 实现并发请求测试（5个线程同时发送）
  - [ ] 实现NULL带额外数据容错测试（附加4字节数据）

- [ ] Task 8: RPC错误处理测试套件（test_rpc_errors.cpp）
  - [ ] 实现无效RPC版本号测试（期望RPC_MISMATCH）
  - [ ] 实现未知程序号测试（期望PROG_UNAVAIL）
  - [ ] 实现无效NFS版本号测试（期望PROG_MISMATCH, low=high=3）
  - [ ] 实现无效过程号测试（期望PROC_UNAVAIL）
  - [ ] 实现乱序响应处理测试（XID匹配验证）

## Phase 4: NFSv3操作测试 - 基础操作

- [ ] Task 9: GETATTR/SETATTR测试套件（test_nfs3_getattr.cpp）
  - [ ] 实现GETATTR根目录属性测试（type=NF3DIR）
  - [ ] 实现GETATTR文件属性测试（type=NF3REG, size>0）
  - [ ] 实现GETATTR无效句柄测试（NFS3ERR_BADHANDLE）
  - [ ] 实现SETATTR截断文件测试（size=0）
  - [ ] 实现SETATTR修改时间戳测试

- [ ] Task 10: LOOKUP查找操作测试套件（test_nfs3_lookup.cpp）
  - [ ] 实现LOOKUP "." 自身查找测试
  - [ ] 实现查找已存在文件测试
  - [ ] 实现查找不存在文件测试（NOENT）
  - [ ] 实现跨目录两级查找测试
  - [ ] 实现空名称查找错误测试
  - [ ] 实现超长名称查找错误测试

## Phase 5: NFSv3操作测试 - 数据操作

- [ ] Task 11: READ/WRITE读写操作测试套件（test_nfs3_readwrite.cpp）
  - [ ] 实现READ小文件完整内容测试（数据一致性验证）
  - [ ] 实现READ偏移读取测试
  - [ ] 实现READ超出文件末尾测试
  - [ ] 实现READ目录当文件错误测试（ISDIR）
  - [ ] 实现WRITE新文件测试（含READ回验）
  - [ ] 实现WRITE追加写入测试
  - [ ] 实现WRITE覆盖写入测试

## Phase 6: NFSv3操作测试 - 修改操作

- [ ] Task 12: CREATE/MKDIR创建操作测试套件（test_nfs3_create.cpp）
  - [ ] 实现UNCHECKED模式CREATE测试
  - [ ] 实现GUARDED模式CREATE成功+失败(EXIST)测试
  - [ ] 实现EXCLUSIVE模式CREATE测试
  - [ ] 实现MKDIR创建目录测试（type=NF3DIR验证）
  - [ ] 实现MKDIR父目录无效错误测试

- [ ] Task 13: REMOVE/RMDIR删除操作测试套件（test_nfs3_remove.cpp）
  - [ ] 实现REMOVE删除已存在文件测试
  - [ ] 实现REMOVE不存在文件错误测试（NOENT）
  - [ ] 实现RMDIR空目录删除测试
  - [ ] 实现RMDIR非空目录失败测试（NOTEMPTY）
  - [ ] 实现RMDIR不存在目录错误测试（NOENT）

## Phase 7: NFSv3操作测试 - 目录与其他

- [ ] Task 14: READDIR/READDIRPLUS目录枚举测试套件（test_nfs3_readdir.cpp）
  - [ ] 实现READDIR基本枚举测试（验证.和..条目）
  - [ ] 实现分页遍历大目录测试（cookie机制）
  - [ ] 实现count太小错误测试（TOOSMALL）
  - [ ] 实现READDIRPLUS带属性测试
  - [ ] 实现dircount/maxcount太小错误测试

- [ ] Task 15: 其他操作测试套件（test_nfs3_other.cpp）
  - [ ] 实现ACCESS权限检查测试
  - [ ] 实现READLINK/SYMLINK符号链接测试
  - [ ] 实现MKNOD创建FIFO测试
  - [ ] 实现RENAME重命名测试
  - [ ] 实现LINK硬链接测试
  - [ ] 实现PATHCONF/FSSTAT/FSINFO查询测试
  - [ ] 实现COMMIT提交测试（如果服务器支持）

## Phase 8: 边界条件与压力测试

- [ ] Task 16: 边界条件和异常输入测试套件（test_nfs3_stress.cpp）
  - [ ] 实现零长度文件名测试
  - [ ] 实现最大长度文件名测试（255字符）
  - [ ] 实现超长文件名测试（>1024字节）
  - [ ] 实现特殊字符文件名测试（null字节等）
  - [ ] 实现大量并发请求压力测试（100并发TCP连接）
  - [ ] 实现重复XID请求幂等性测试

## Phase 9: 集成与文档

- [ ] Task 17: 测试运行脚本与CI配置
  - [ ] 编写run_tests.sh主控脚本（支持参数化运行）
  - [ ] 实现ctest集成（CMake自带测试发现）
  - [ ] 可选：编写GitHub Actions CI配置示例

- [ ] Task 18: 文档完善
  - [ ] 更新README.md（安装依赖、构建、运行、架构说明）
  - [ ] API文档（Doxygen注释或头文件内联注释）
  - [ ] 提供2-3个典型使用示例代码

# Task Dependencies

**Phase 1 (基础设施)** → 串行依赖:
- [Task 1] 必须最先完成（项目骨架）
- [Task 2] 依赖 [Task 1]（需要构建系统）
- [Task 3] 依赖 [Task 2]（需要XDR编解码器来序列化数据结构）
- [Task 4] 依赖 [Task 3]（需要NFS3类型定义才能构造RPC参数）

**Phase 2 (框架核心)** → 串行依赖Phase 1:
- [Task 5] 依赖 [Task 4]（需要RPCEndpoint来发送RPC调用）
- [Task 6] 依赖 [Task 5]（需要NFS3TestClient来封装进Context）

**Phase 3-8 (测试用例)** → 并行开发（均依赖Phase 2完成）:
- [Task 7], [Task 8] 可并行（RPC层测试）
- [Task 9], [Task 10] 可并行（基础操作）
- [Task 11] 依赖 [Task 10]（WRITE需要先LOOKUP获取句柄）
- [Task 12], [Task 13] 可并行（修改操作）
- [Task 14], [Task 15] 可并行（目录与其他）
- [Task 16] 应在其他测试完成后进行（综合验证）

**Phase 9 (集成)** → 最后执行:
- [Task 17], [Task 18] 依赖所有测试用例完成

# 未来扩展（非当前范围）

- [ ] **UDP传输支持**: 扩展RPCEndpoint支持"udp"协议（需处理消息边界和重传）
- [ ] **AUTH_UNIX认证**: 扩展认证方式支持Unix风格UID/GID
- [ ] **异步IO**: 基于libevent或epoll的高性能并发模型
- [ ] **性能基准测试**: 吞吐量、延迟、QPS统计报告
