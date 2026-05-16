# NFSv3 RPC接口测试套件 - 验证清单

## 基础设施验证

- [ ] 项目目录结构符合设计规范（include/, src/, tests/, xdr/）
- [ ] Makefile支持`make all`, `make clean`, `make test`等目标
- [ ] 编译成功且无警告（使用-Wall -Wextra）
- [ ] 正确链接libtirpc库（无未定义符号）
- [ ] rpcgen生成的nfs_prot.h包含所有NFSv3数据结构定义
- [ ] XDR辅助函数可正确编解码基本类型和复合类型

## RPC连接管理验证

- [ ] 可通过TCP连接到NFS服务器（默认端口2049）
- [ ] 可通过UDP连接到NFS服务器（可选）
- [ ] RPC消息片段(fragment)收发正确处理EOF标志
- [ ] XID在请求/响应间正确匹配
- [ ] 超时机制正常工作（可配置超时时间）
- [ ] 连接错误有清晰的错误码和消息

## 测试框架核心验证

- [ ] NFS3TestClient封装了全部22个NFSv3过程调用API
- [ ] 每个过程的参数和返回值类型与RFC 1813一致
- [ ] TestContext的setup/teardown生命周期管理正确
- [ ] 命令行参数解析支持--server, --port, --verbose等选项
- [ ] 断言宏能准确定位失败位置（文件名、行号、表达式）
- [ ] 测试结果统计准确（总数/通过/失败/跳过）

## NULL过程测试验证

- [ ] NULL请求返回成功响应（无数据）
- [ ] 10个顺序请求全部收到匹配XID的响应
- [ ] 5个并发请求全部正确完成
- [ ] NULL请求带额外数据时服务器优雅处理

## RPC错误处理测试验证

- [ ] 无效RPC版本号(0x12345678)触发RPC_MISMATCH错误
- [ ] 未知程序号(0x12345678)触发PROG_UNAVAIL错误
- [ ] 无效NFS版本号(0x12345678)触发PROG_MISMATCH错误（low=high=3）
- [ ] 无效过程号(0x12345678)触发PROC_UNAVAIL错误
- [ ] 乱序响应对应正确的XID
- [ ] 重复XID请求正确处理（幂等性或缓存响应）

## GETATTR/SETATTR测试验证

- [ ] GETATTR(root_fh)返回type=NF3DIR
- [ ] GETATTR(file_fh)返回type=NF3REG且size>0
- [ ] GETATTR(invalid_fh)返回NFS3ERR_BADHANDLE
- [ ] SETATTR截断文件后GETATTR确认size=0
- [ ] SETATTR修改mtime后GETATTR确认时间戳更新

## LOOKUP操作测试验证

- [ ] LOOKUP(root, ".")返回的对象句柄等于root
- [ ] LOOKUP找到已存在文件并返回正确属性
- [ ] LOOKUP找不到文件返回NFS3ERR_NOENT
- [ ] 两级LOOKUP（先目录后文件）成功定位
- [ ] 空名称LOOKUP返回INVAL或NOENT错误
- [ ] 超长名称(>255字节)LOOKUP返回NAMETOOLONG或IO错误

## READ/WRITE操作测试验证

- [ ] READ小文件返回完整内容且数据一致
- [ ] READ(offset=3, count=4)返回正确的子串
- [ ] READ超出文件末尾返回eof=true或空数据
- [ ] READ目录句柄返回NFS3ERR_ISDIR
- [ ] WRITE新文件返回count等于写入字节数
- [ ] WRITE后READ回验数据完全一致
- [ ] 追加WRITE增加文件总大小
- [ ] 覆盖WRITE替换原有数据
- [ ] UNSTABLE+COMMIT流程完整执行（如果支持）

## CREATE/MKDIR操作测试验证

- [ ] UNCHECKED CREATE创建新文件成功
- [ ] UNCHECKED CREATE已存在文件则截断
- [ ] GUARDED CREATE新文件成功
- [ ] GUARDED CREATE已存在文件返回EXIST
- [ ] EXCLUSIVE CREATE行为符合实现规范
- [ ] MKDIR创建目录且GETATTR显示type=NF3DIR
- [ ] MKDIR无效父目录返回适当错误

## REMOVE/RMDIR操作测试验证

- [ ] REMOVE删除已存在文件成功
- [ ] 后续LOOKUP被删文件返回NOENT
- [ ] REMOVE不存在文件返回NOENT
- [ ] RMDIR空目录成功
- [ ] RMDIR非空目录返回NOTEMPTY
- [ ] RMDIR不存在目录返回NOENT

## READDIR/READDIRPLUS测试验证

- [ ] READDIR根目录包含"."和".."条目
- [ ] READDIR包含已知文件/子目录条目
- [ ] 分页READDIR使用cookie遍历完大目录无遗漏
- [ ] cookieverf在多次读取中保持一致
- [ ] count太小(<512)返回TOOSMALL错误
- [ ] READDIRPLUS返回条目及属性信息
- [ ] dircount/maxcount太小返回TOOSMALL错误

## 其他操作测试验证

- [ ] ACCESS返回包含请求权限位的掩码
- [ ] READLINK(symlink)返回目标路径
- [ ] READLINK(non-symlink)返回INVAL错误
- [ ] SYMLINK创建后READLINK验证目标
- [ ] MKNOD创建FIFO特殊文件
- [ ] RENAME源消失目标出现
- [ ] LINK两路径fileid相同
- [ ] PATHCONF返回合理的系统限制值
- [ ] FSSTAT返回非负的统计值
- [ ] FSINFO返回合理的能力参数
- [ ] COMMIT持久化UNSTABLE写入的数据

## 边界条件与压力测试验证

- [ ] 零长度文件名触发适当的错误
- [ ] 255字符文件名成功创建（如果FS支持）
- [ ] >1024字节超长文件名返回NAMETOOLONG或降级处理
- [ ] null字节文件名返回INVAL错误
- [ ] 100并发请求全部完成无死锁崩溃
- [ ] 重复XID请求幂等处理正确

## 集成与文档验证

- [ ] run_tests.sh可运行全部或指定测试组
- [ ] 测试输出清晰显示每个用例结果（PASS/FAIL/SKIP）
- [ ] 失败用例显示详细错误信息和堆栈
- [ ] README.md包含完整的安装和使用说明
- [ ] API文档覆盖所有公共函数
- [ ] 示例代码可直接编译运行

## 性能与质量指标

- [ ] 全部测试套件执行时间 < 5分钟（单机局域网环境）
- [ ] 内存泄漏检查通过（valgrind或类似工具）
- [ ] 代码覆盖率 > 80%（关键路径100%）
- [ ] 无硬编码IP地址或端口号（可通过参数配置）
- [ ] 支持IPv4和IPv6地址格式
