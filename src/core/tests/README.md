# Physics Event System Test Build Configuration

## 测试文件概览

本目录包含物理事件系统的完整测试套件：

### 测试文件说明

1. **test_physics_event_system.cpp** - 核心功能测试
   - 事件类型定义和分发
   - 碰撞、触发器、查询事件
   - 基础事件系统功能验证

2. **test_2d_3d_intersection.cpp** - 2D/3D相交检测专门测试
   - 平面相交（2D）vs 空间相交（3D）
   - 水面、地面、墙面检测
   - 相交维度检测准确性

3. **test_lazy_loading.cpp** - 懒加载机制测试
   - 组件按需创建
   - 生命周期管理
   - 内存效率和缓存

4. **test_physics_event_performance.cpp** - 性能测试
   - 大规模实体处理
   - 高频事件处理
   - 内存和CPU性能分析

5. **test_integration.cpp** - 系统集成测试
   - 端到端工作流程
   - 并发事件处理
   - 系统协调和错误恢复

6. **test_runner.cpp** - 测试运行器
   - 统一测试执行
   - 测试结果汇总
   - 选择性测试执行

## 编译说明

### 前置条件

确保以下依赖已正确配置：
- Jolt Physics 库
- entt ECS 框架
- 物理事件系统核心文件

### 手动编译

如果你想手动编译单个测试：

```bash
# 编译2D/3D相交测试
g++ -std=c++17 -I../../ -I../../../vendor test_2d_3d_intersection.cpp -o test_2d_3d_intersection

# 编译懒加载测试
g++ -std=c++17 -I../../ -I../../../vendor test_lazy_loading.cpp -o test_lazy_loading

# 编译集成测试
g++ -std=c++17 -I../../ -I../../../vendor test_integration.cpp -o test_integration

# 编译测试运行器
g++ -std=c++17 -I../../ test_runner.cpp -o test_runner
```

### 使用SCons构建

建议将测试添加到SCons构建系统中。在项目根目录的SConstruct文件中添加：

```python
# 测试构建配置
test_env = env.Clone()

# 测试源文件
test_sources = [
    'src/core/tests/test_physics_event_system.cpp',
    'src/core/tests/test_2d_3d_intersection.cpp', 
    'src/core/tests/test_lazy_loading.cpp',
    'src/core/tests/test_physics_event_performance.cpp',
    'src/core/tests/test_integration.cpp',
    'src/core/tests/test_runner.cpp'
]

# 构建测试可执行文件
for test_source in test_sources:
    test_name = os.path.splitext(os.path.basename(test_source))[0]
    test_env.Program(
        target=f'build/{test_name}',
        source=[test_source] + core_sources + physics_sources
    )
```

## 运行测试

### 运行所有测试

```bash
./build/test_runner --all
```

### 运行特定测试

```bash
# 2D/3D相交检测测试
./build/test_runner --test 2d_3d_intersection

# 懒加载测试
./build/test_runner --test lazy_loading

# 性能测试
./build/test_runner --test performance

# 集成测试
./build/test_runner --test integration
```

### 查看可用测试

```bash
./build/test_runner --list
```

## 测试覆盖范围

### 功能覆盖

✅ **事件类型定义** - 所有物理事件类型
✅ **2D/3D检测** - 平面vs空间相交区分  
✅ **懒加载机制** - 组件按需创建和管理
✅ **事件分发** - 完整的事件流程
✅ **查询系统** - 射线、重叠、扫描查询
✅ **监控组件** - 区域和平面监控
✅ **性能测试** - 大规模和高频场景
✅ **集成测试** - 端到端系统协调

### 性能指标

- **实体规模**: 支持1000+实体同时处理
- **事件频率**: 支持高频碰撞事件
- **内存效率**: 懒加载减少不必要的内存分配
- **响应时间**: 平均帧时间 < 50ms（正常负载）

### 错误处理

- **无效实体**: 优雅处理已销毁或无效实体
- **系统恢复**: 支持子系统重启和恢复
- **内存管理**: 自动清理和生命周期管理

## 调试和故障排除

### 常见问题

1. **编译错误**
   - 检查include路径是否正确
   - 确认所有依赖库已链接
   - 验证C++17标准支持

2. **运行时错误**
   - 检查Jolt Physics初始化
   - 验证事件管理器配置
   - 确认实体和组件正确创建

3. **性能问题**
   - 启用调试模式查看详细日志
   - 检查内存使用和泄漏
   - 分析事件处理频率

### 调试模式

在测试中启用调试模式：

```cpp
physics_event_system_->set_debug_mode(true);
lazy_query_manager_->set_debug_mode(true);
```

这将输出详细的运行时信息，帮助诊断问题。

## 扩展测试

### 添加新测试

1. 创建新的测试文件（如`test_my_feature.cpp`）
2. 实现测试逻辑
3. 在`test_runner.cpp`中注册新测试
4. 更新构建配置

### 测试最佳实践

- **独立性**: 每个测试应该独立运行
- **清理**: 测试后正确清理资源
- **断言**: 使用明确的成功/失败标准
- **文档**: 为测试添加清晰的说明

这个测试套件为物理事件系统提供了全面的验证，确保系统的正确性、性能和可靠性。
