# ECS Node API 设计文档

## 概述

本文档定义了ECS Node系统的新API接口，用于解决以下核心问题：
1. ECS世界的实体组件变化无法同步回Godot节点
2. Godot组件变化导致完全重建而非增量更新
3. ECS实体删除时无法自动删除对应的Godot节点

## 当前架构分析

### 现有问题

1. **单向同步**：只有Godot → ECS的同步，缺少ECS → Godot的反向同步
2. **粗粒度更新**：组件变化时调用`clear_all_non_basic_components()`完全重建
3. **映射管理不完整**：PortalGameWorld有双向映射，但缺少反向删除机制
4. **变更源区分不足**：无法区分编辑器UI变更和ECS系统变更

### 现有优势

1. **双向映射基础**：PortalGameWorld已有`godot_to_entt_`和`entt_to_godot_`映射
2. **多态组件系统**：ECSComponentResource提供了良好的扩展性
3. **信号系统**：GameCoreManager提供了事件通知机制
4. **生命周期管理**：完善的实体创建和销毁流程

## API 设计方案

### 1. ECS组件查询API

#### 1.1 PortalGameWorld扩展

```cpp
// 在 portal_game_world.h 中添加
class PortalGameWorld {
public:
    // 组件查询API
    template<typename T>
    bool has_component(entt::entity entity) const;
    
    template<typename T>
    T* get_component(entt::entity entity);
    
    template<typename T>
    const T* get_component(entt::entity entity) const;
    
    // 获取实体的所有组件类型
    std::vector<entt::type_info> get_entity_components(entt::entity entity) const;
    
    // 组件变更监听
    template<typename T>
    void register_component_listener(std::function<void(entt::entity, const T&)> on_added,
                                   std::function<void(entt::entity, const T&)> on_updated,
                                   std::function<void(entt::entity)> on_removed);
};
```

#### 1.2 ECSNode查询接口

```cpp
// 在 ecs_node.h 中添加
class ECSNode {
public:
    // 查询当前实体的组件
    bool has_ecs_component(const String& component_type) const;
    Array get_ecs_component_types() const;
    Variant get_ecs_component_data(const String& component_type) const;
    
    // 查询Godot组件资源
    bool has_godot_component(const String& component_class) const;
    Array get_godot_component_types() const;
    Resource* get_godot_component(const String& component_class) const;
};
```

### 2. ECS组件增加/删除API

#### 2.1 动态组件管理

```cpp
// 在 ecs_node.h 中添加
class ECSNode {
public:
    // 动态添加组件（从ECS系统调用）
    bool add_ecs_component_from_system(const String& component_type, const Dictionary& data);
    
    // 动态删除组件（从ECS系统调用）
    bool remove_ecs_component_from_system(const String& component_type);
    
    // 动态添加Godot组件资源（从编辑器调用）
    bool add_godot_component_from_editor(Resource* component_resource);
    
    // 动态删除Godot组件资源（从编辑器调用）
    bool remove_godot_component_from_editor(const String& component_class);
    
private:
    // 变更源标识
    enum ChangeSource {
        CHANGE_FROM_EDITOR,
        CHANGE_FROM_ECS_SYSTEM,
        CHANGE_FROM_INTERNAL
    };
    
    // 内部组件管理
    bool _add_component_internal(Resource* component_resource, ChangeSource source);
    bool _remove_component_internal(const String& component_class, ChangeSource source);
};
```

#### 2.2 增量更新机制

```cpp
// 在 ecs_node.h 中添加
class ECSNode {
private:
    // 替换完全重建的方法
    void update_single_component(Resource* component_resource, ChangeSource source);
    void remove_single_component(const String& component_class, ChangeSource source);
    
    // 组件差异检测
    struct ComponentDiff {
        Array added_components;
        Array removed_components;
        Array modified_components;
    };
    
    ComponentDiff detect_component_changes() const;
    void apply_component_diff(const ComponentDiff& diff, ChangeSource source);
};
```

### 3. 双向链接维护API

#### 3.1 反向删除机制

```cpp
// 在 portal_game_world.h 中添加
class PortalGameWorld {
public:
    // 实体删除时的回调注册
    void register_entity_destruction_callback(std::function<void(entt::entity, uint64_t)> callback);
    
    // 增强的实体销毁方法
    void destroy_entity_with_godot_cleanup(entt::entity entity);
    
private:
    std::vector<std::function<void(entt::entity, uint64_t)>> destruction_callbacks_;
};
```

#### 3.2 GameCoreManager扩展

```cpp
// 在 game_core_manager.h 中添加
class GameCoreManager {
public:
    // 注册ECS实体删除监听
    void register_ecs_entity_deletion_listener();
    
    // ECS实体删除回调
    static void on_ecs_entity_destroyed(entt::entity entity, uint64_t godot_id);
    
    // 查找并删除对应的Godot节点
    static void cleanup_godot_node_for_entity(uint64_t godot_id);
    
private:
    static std::unordered_map<uint64_t, Node*> godot_id_to_node_; // 新增：反向查找映射
};
```

### 4. 变更源区分机制

#### 4.1 变更上下文

```cpp
// 新文件：ecs_change_context.h
class ECSChangeContext {
public:
    enum Source {
        EDITOR_UI,      // 编辑器界面操作
        ECS_SYSTEM,     // ECS系统内部变更
        SCRIPT_API,     // GDScript API调用
        SCENE_LOADING   // 场景加载
    };
    
    static void set_current_source(Source source);
    static Source get_current_source();
    static bool is_editor_change();
    static bool is_ecs_system_change();
    
private:
    static thread_local Source current_source_;
};
```

#### 4.2 上下文感知的更新逻辑

```cpp
// 在 ecs_node.cpp 中修改
void ECSNode::_on_resource_changed() {
    if (ECSChangeContext::is_editor_change()) {
        // 编辑器变更：清除重建（保持现有行为）
        _update_ecs_components();
    } else if (ECSChangeContext::is_ecs_system_change()) {
        // ECS系统变更：增量更新
        auto diff = detect_component_changes();
        apply_component_diff(diff, CHANGE_FROM_ECS_SYSTEM);
    }
}
```

## 实现计划

### 阶段1：基础API实现
1. 扩展PortalGameWorld的组件查询API
2. 实现ECSNode的查询接口
3. 添加变更源区分机制

### 阶段2：增量更新机制
1. 实现组件差异检测
2. 替换完全重建逻辑为增量更新
3. 添加组件变更监听

### 阶段3：双向链接维护
1. 实现反向删除机制
2. 扩展GameCoreManager的实体管理
3. 添加实体删除回调系统

### 阶段4：测试和优化
1. 单元测试覆盖
2. 性能优化
3. 文档完善

## 兼容性考虑

1. **向后兼容**：现有API保持不变，新API作为扩展
2. **渐进迁移**：可以逐步从完全重建迁移到增量更新
3. **性能影响**：新机制应该提升而非降低性能
4. **调试支持**：提供详细的变更日志和调试信息

## 总结

这个API设计解决了ECS Node系统的三个核心问题：
1. 通过组件监听和反向同步实现ECS → Godot的数据流
2. 通过变更源区分和增量更新避免不必要的重建
3. 通过双向链接维护实现实体删除的自动清理

设计保持了现有架构的优势，同时提供了更精细的控制和更好的性能。