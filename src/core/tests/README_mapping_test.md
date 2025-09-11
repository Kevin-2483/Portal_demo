# PhysicsEventAdapter 映射优化测试

## 测试文件说明
- `test_mapping_optimization.cpp` - 映射性能优化测试（单文件）

## 构建方法

### 方法1: 使用SCons（推荐）
在项目根目录下的 SConstruct 文件中添加测试目标：

```python
# 添加到 SConstruct 文件
test_mapping_env = env.Clone()
test_mapping_program = test_mapping_env.Program(
    target='build/test_mapping_optimization',
    source=['src/core/tests/test_mapping_optimization.cpp']
)
```

然后运行：
```bash
scons test_mapping_optimization
```

### 方法2: 手动编译
```bash
# 使用MSVC编译器
cl /std:c++17 /I"path/to/entt/include" /I"path/to/jolt/include" /I"src" ^
   src/core/tests/test_mapping_optimization.cpp ^
   /Fe:build/test_mapping_optimization.exe

# 或使用g++
g++ -std=c++17 -I"path/to/entt/include" -I"path/to/jolt/include" -I"src" ^
    src/core/tests/test_mapping_optimization.cpp ^
    -o build/test_mapping_optimization.exe
```

## 运行测试
```bash
build/test_mapping_optimization.exe
```

## 测试内容

### 1. 增量映射功能测试
- 验证实体创建时映射自动添加
- 验证组件更新时映射自动更新
- 验证实体销毁时映射自动清理

### 2. 性能对比测试
- 比较增量更新 vs 每帧重建的性能
- 测试5000个实体，100个更新周期
- 计算性能提升比例

### 3. 大规模场景测试
- 测试20000个实体的处理能力
- 验证平均帧时间是否满足性能要求
- 确保在大规模场景下的稳定性

## 预期结果
- **功能测试**: 所有映射操作应该自动正确处理
- **性能测试**: 增量更新应比每帧重建快 2-50倍
- **大规模测试**: 平均帧时间应保持在合理范围内（< 16.67ms for 60FPS）

测试将验证优化方案的有效性，确保在不破坏功能的前提下显著提升性能。
