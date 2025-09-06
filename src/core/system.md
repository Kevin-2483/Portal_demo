## REGISTER_SYSTEM_SIMPLE vs REGISTER_SYSTEM

### REGISTER_SYSTEM_SIMPLE (简化版)
```cpp
#define REGISTER_SYSTEM_SIMPLE(SystemClass, Priority) \
    REGISTER_SYSTEM(SystemClass, {}, {}, Priority)
```

**特点：**
- ✅ **无依赖关系**：`{}` 表示不依赖任何其他系统
- ✅ **无冲突关系**：`{}` 表示不与任何系统冲突  
- ✅ **只需指定优先级**：更简洁的语法
- ✅ **适合独立系统**：自包含的系统使用

**使用示例：**
```cpp
// 旋转系统，完全独立，不依赖其他系统
REGISTER_SYSTEM_SIMPLE(XRotationSystem, 100);
REGISTER_SYSTEM_SIMPLE(YRotationSystem, 101);
REGISTER_SYSTEM_SIMPLE(ZRotationSystem, 102);
```

### REGISTER_SYSTEM (完整版)
```cpp
#define REGISTER_SYSTEM(SystemClass, Dependencies, Conflicts, Priority)
```

**特点：**
- ✅ **完整控制**：可以指定依赖和冲突关系
- ✅ **依赖管理**：`Dependencies` 数组指定必须在此系统前执行的系统
- ✅ **冲突管理**：`Conflicts` 数组指定不能同时运行的系统
- ✅ **适合复杂系统**：有依赖关系的系统使用

**使用示例：**
```cpp
// 物理系统有复杂的依赖关系
REGISTER_SYSTEM(PhysicsCommandSystem, {}, {}, 10);                    // 无依赖，最先执行
REGISTER_SYSTEM(PhysicsSystem, {"PhysicsCommandSystem"}, {}, 20);     // 依赖命令系统
REGISTER_SYSTEM(PhysicsQuerySystem, {"PhysicsSystem"}, {}, 30);       // 依赖物理系统
```

## 实际项目中的使用对比

### 当前项目中的应用：

#### 独立系统使用 REGISTER_SYSTEM_SIMPLE：
```cpp
// X/Y/Z 旋转系统 - 完全独立
REGISTER_SYSTEM_SIMPLE(XRotationSystem, 100);
REGISTER_SYSTEM_SIMPLE(YRotationSystem, 101); 
REGISTER_SYSTEM_SIMPLE(ZRotationSystem, 102);
```

#### 有依赖的系统使用 REGISTER_SYSTEM：
```cpp
// 物理系统 - 有明确的执行顺序要求
REGISTER_SYSTEM(PhysicsCommandSystem, {}, {}, 10);
REGISTER_SYSTEM(PhysicsSystem, {"PhysicsCommandSystem"}, {}, 20);
REGISTER_SYSTEM(PhysicsQuerySystem, {"PhysicsSystem"}, {}, 30);
```

## 依赖关系的作用

### 执行顺序保证：
1. **PhysicsCommandSystem** (优先级10) - 处理物理命令
2. **PhysicsSystem** (优先级20) - 执行物理模拟，依赖命令系统
3. **PhysicsQuerySystem** (优先级30) - 查询物理结果，依赖物理系统
4. **XRotationSystem** (优先级100) - 独立执行，无依赖
5. **YRotationSystem** (优先级101) - 独立执行，无依赖  
6. **ZRotationSystem** (优先级102) - 独立执行，无依赖

### SystemManager 会根据依赖关系：
- 🔄 **检测循环依赖**：防止 A→B→A 的情况
- 📊 **构建执行图**：确保依赖系统先执行
- ⚡ **并行优化**：无依赖的系统可以并行执行

## 选择建议

### 使用 REGISTER_SYSTEM_SIMPLE 当：
- ✅ 系统完全独立
- ✅ 不需要特定的执行顺序
- ✅ 代码更简洁

### 使用 REGISTER_SYSTEM 当：
- ✅ 系统有依赖关系
- ✅ 执行顺序很重要
- ✅ 需要冲突检测

**本质上**：`REGISTER_SYSTEM_SIMPLE` 就是 `REGISTER_SYSTEM` 的简化版本，内部调用的是同一个实现！