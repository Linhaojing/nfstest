# NFSv3 RPC接口测试套件 - 任务分解

## Phase 1: 基础设施搭建

- [ ] Task 1: 项目初始化与构建系统配置
  - [ ] 创建目录结构（include/, src/, tests/, xdr/）
  - [ ] 编写Makefile支持多目标编译
  - [ ] 配置编译选项（C99标准，libtirpc链接）
  - [ ] 添加.gitignore和基本README框架

- [ ] Task 2: XDR类型定义生成
  - [ ] 获取NFSv3协议定义文件（nfs_prot.x）
  - [ ] 使用rpcgen生成头文件和XDR编解码代码
  - [ ] 验证生成的nfs_prot.h包含所有22个过程的数据结构
  - [ ] 封装XDR辅助函数（xdr_helpers.h/c）

- [ ] Task 3: RPC连接管理模块实现
  - [ ] 实现rpc_endpoint.h/c：TCP/UDP连接创建与管理
  - [ ] 实现RPC消息片段(fragment)收发逻辑
  - [ ] 实现RPC消息封装/解包（XID匹配、状态码提取）
  - [ ] 添加超时处理和错误码映射

## Phase 2: 测试框架核心

- [ ] Task 4: NFS3TestClient高级API实现
  - [ ] 实现nfs3_client.h/c：封装全部22个NFSv3过程调用
  - [ ] 每个过程提供同步和异步两种接口
  - [ ] 统一错误处理（将RPC错误+NFS错误统一为错误码枚举）
  - [ ] 提供便捷的参数构造辅助函数

- [ ] Task 5: TestContext测试上下文实现
  - [ ] 实现test_context.h/c：管理测试生命周期
  - [ ] 支持setup（连接服务器）和teardown（断开连接）
  - [ ] 集成日志输出（可选verbose模式）
  - [ ] 支持命令行参数解析（server, port, test_group等）

- [ ] Task 6: 断言宏与测试报告系统
  - [ ] 实现nfs3_assert.h：NFS特定的断言宏
  - [ ] 提供ASSERT_NFS_OK(), ASSERT_NFS_ERROR(), ASSERT_FATTR_EQUAL()等
  - [ ] 实现测试结果统计（通过/失败/跳过计数）
  - [ ] 支持JUnit XML格式输出（便于CI集成）

## Phase 3: RPC层测试用例

- [ ] Task 7: NULL过程测试套件（test_rpc_null.c）
  - [ ] 实现NULL正常响应测试
  - [ ] 实现顺序多请求测试（10个连续请求）
  - [ ] 实现并发请求测试（5个并发）
  - [ ] 实现NULL带额外数据容错测试

- [ ] Task 8: RPC错误处理测试套件（test_rpc_errors.c）
  - [ ] 实现无效RPC版本号测试（期望RPC_MISMATCH）
  - [ ] 实现未知程序号测试（期望PROG_UNAVAIL）
  - [ ] 实现无效NFS版本号测试（期望PROG_MISMATCH）
  - [ ] 实现无效过程号测试（期望PROC_UNAVAIL）
  - [ ] 实现乱序响应处理测试
  - [ ] 实现重复请求幂等性测试

## Phase 4: NFSv3操作测试 - 基础操作

- [ ] Task 9: GETATTR/SETATTR测试套件（test_nfs3_getattr.c）
  - [ ] 实现获取根目录属性测试（验证type=NF3DIR）
  - [ ] 实现获取文件属性测试（验证type=NF3REG, size等）
  - [ ] 实现无效句柄GETATTR测试（期望BADHANDLE）
  - [ ] 实现SETATTR修改size测试（截断文件）
  - [ ] 实现SETATTR修改时间戳测试

- [ ] Task 10: LOOKUP查找操作测试套件（test_nfs3_lookup.c）
  - [ ] 实现LOOKUP "." 自身查找测试
  - [ ] 实现查找已存在文件测试
  - [ ] 实现查找不存在文件测试（期望NOENT）
  - [ ] 实现跨目录查找测试（两级路径）
  - [ ] 实现空名称查找测试（期望INVAL或NOENT）
  - [ ] 实现超长名称查找测试（>255字节）

## Phase 5: NFSv3操作测试 - 数据操作

- [ ] Task 11: READ/WRITE读写操作测试套件（test_nfs3_readwrite.c）
  - [ ] 实现读取整个小文件测试（验证内容一致性）
  - [ ] 实现偏移读取测试（验证offset语义）
  - [ ] 实现读取超出文件末尾测试
  - [ ] 实现读取目录当文件测试（期望ISDIR）
  - [ ] 实现WRITE写入新文件测试（含READ回验）
  - [ ] 实现追加写入测试
  - [ ] 实现覆盖写入测试
  - [ ] 实现UNSTABLE+COMMIT流程测试（如果服务器支持）

## Phase 6: NFSv3操作测试 - 修改操作

- [ ] Task 12: CREATE/MKDIR创建操作测试套件（test_nfs3_create.c）
  - [ ] 实现UNCHECKED模式CREATE测试
  - [ ] 实现GUARDED模式CREATE成功测试
  - [ ] 实现GUARDED模式已存在失败测试（期望EXIST）
  - [ ] 实现EXCLUSIVE模式CREATE测试
  - [ ] 实现MKDIR创建目录测试（验证type=NF3DIR）
  - [ ] 实现MKDIR父目录不存在错误测试

- [ ] Task 13: REMOVE/RMDIR删除操作测试套件（test_nfs3_remove.c）
  - [ ] 实现REMOVE删除已存在文件测试
  - [ ] 实现REMOVE删除不存在文件测试（期望NOENT）
  - [ ] 实现RMDIR删除空目录测试
  - [ ] 实现RMDIR非空目录失败测试（期望NOTEMPTY）
  - [ ] 实现RMDIR不存在目录测试（期望NOENT）

## Phase 7: NFSv3操作测试 - 目录与其他

- [ ] Task 14: READDIR/READDIRPLUS目录枚举测试套件（test_nfs3_readdir.c）
  - [ ] 实现读取根目录测试（验证.和..存在）
  - [ ] 实现分页读取大目录测试（200+文件，cookie机制）
  - [ ] 实现cookieverf验证测试
  - [ ] 实现count太小错误测试（期望TOOSMALL）
  - [ ] 实现READDIRPLUS带属性读取测试
  - [ ] 实现READDIRPLUS dircount/maxcount太小测试

- [ ] Task 15: 其他操作测试套件（test_nfs3_other.c）
  - [ ] 实现ACCESS权限检查测试
  - [ ] 实现READLINK符号链接读取测试
  - [ ] 实现READLINK非符号链接错误测试
  - [ ] 实现SYMLINK创建符号链接测试
  - [ ] 实现MKNOD创建FIFO测试
  - [ ] 实现RENAME重命名测试
  - [ ] 实现LINK硬链接测试
  - [ ] 实现PATHCONF查询测试
  - [ ] 实现FSSTAT统计信息测试
  - [ ] 实现FSINFO能力查询测试
  - [ ] 实现COMMIT提交测试

## Phase 8: 边界条件与压力测试

- [ ] Task 16: 边界条件和异常输入测试套件（test_nfs3_stress.c）
  - [ ] 实现零长度文件名测试
  - [ ] 实现最大长度文件名测试（255字符）
  - [ ] 实现超长文件名测试（>1024字节）
  - [ ] 实现特殊字符文件名测试（空格/tab/null字节）
  - [ ] 实现大量并发请求压力测试（100并发）
  - [ ] 实现快速重复请求幂等性测试

## Phase 9: 集成与文档

- [ ] Task 17: 测试运行脚本与CI集成
  - [ ] 编写run_tests.sh主控脚本（支持参数化运行）
  - [ ] 实现测试组选择和过滤功能
  - [ ] 生成测试报告（控制台 + 可选XML）
  - [ ] 编写GitHub Actions CI配置示例（可选）

- [ ] Task 18: 文档完善与示例
  - [ ] 完善README.md（安装、使用、架构说明）
  - [ ] 添加API参考文档（Doxygen注释）
  - [ ] 提供2-3个典型使用示例代码
  - [ ] 编写常见问题FAQ

# Task Dependencies

**Phase 1 (基础设施) → Phase 2 (框架核心)**
- [Task 2] 必须在 [Task 4] 之前完成（需要XDR类型定义）
- [Task 3] 必须在 [Task 4] 之前完成（需要RPC连接能力）

**Phase 2 (框架核心) → Phase 3-8 (测试用例)**
- [Task 4], [Task 5], [Task 6] 必须在所有测试任务之前完成

**Phase 3-8 (测试用例)** 可并行开发：
- [Task 7], [Task 8] 可并行（RPC层测试）
- [Task 9], [Task 10] 可并行（基础操作）
- [Task 11] 依赖 [Task 10]（WRITE需要先LOOKUP获取句柄）
- [Task 12], [Task 13] 可并行（修改操作）
- [Task 14], [Task 15] 可并行（目录与其他）
- [Task 16] 应在其他测试完成后进行（综合验证）

**Phase 9 (集成)** 在所有测试完成后：
- [Task 17], [Task 18] 依赖所有测试用例完成
