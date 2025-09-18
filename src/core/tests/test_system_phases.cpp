#include "core/system_base.h"
#include "core/system_manager.h"
#include <iostream>

using namespace portal_core;

// 测试系统1：PRE_UPDATE阶段，优先级10
class PreUpdateTestSystem : public ISystem
{
public:
    bool initialize() override
    {
        std::cout << "PreUpdateTestSystem initialized" << std::endl;
        return true;
    }

    void update(entt::registry &registry, float delta_time) override
    {
        std::cout << "PreUpdateTestSystem::update (priority: " << get_phase_priority() << ")" << std::endl;
    }

    SystemExecutionPhase get_execution_phase() const override
    {
        return SystemExecutionPhase::PRE_UPDATE;
    }

    int get_phase_priority() const override
    {
        return 10;
    }

    const char* get_name() const override
    {
        return "PreUpdateTestSystem";
    }
};

// 测试系统2：UPDATE阶段，优先级5
class UpdateTestSystem : public ISystem
{
public:
    bool initialize() override
    {
        std::cout << "UpdateTestSystem initialized" << std::endl;
        return true;
    }

    void update(entt::registry &registry, float delta_time) override
    {
        std::cout << "UpdateTestSystem::update (priority: " << get_phase_priority() << ")" << std::endl;
    }

    SystemExecutionPhase get_execution_phase() const override
    {
        return SystemExecutionPhase::UPDATE;
    }

    int get_phase_priority() const override
    {
        return 5;
    }

    const char* get_name() const override
    {
        return "UpdateTestSystem";
    }
};

// 测试系统3：POST_UPDATE阶段，优先级1
class PostUpdateTestSystem : public ISystem
{
public:
    bool initialize() override
    {
        std::cout << "PostUpdateTestSystem initialized" << std::endl;
        return true;
    }

    void update(entt::registry &registry, float delta_time) override
    {
        std::cout << "PostUpdateTestSystem::update (priority: " << get_phase_priority() << ")" << std::endl;
    }

    SystemExecutionPhase get_execution_phase() const override
    {
        return SystemExecutionPhase::POST_UPDATE;
    }

    int get_phase_priority() const override
    {
        return 1;
    }

    const char* get_name() const override
    {
        return "PostUpdateTestSystem";
    }
};

// 测试系统4：PRE_UPDATE阶段，优先级5（应该在PreUpdateTestSystem之前执行）
class HighPriorityPreUpdateSystem : public ISystem
{
public:
    bool initialize() override
    {
        std::cout << "HighPriorityPreUpdateSystem initialized" << std::endl;
        return true;
    }

    void update(entt::registry &registry, float delta_time) override
    {
        std::cout << "HighPriorityPreUpdateSystem::update (priority: " << get_phase_priority() << ")" << std::endl;
    }

    SystemExecutionPhase get_execution_phase() const override
    {
        return SystemExecutionPhase::PRE_UPDATE;
    }

    int get_phase_priority() const override
    {
        return 5;
    }

    const char* get_name() const override
    {
        return "HighPriorityPreUpdateSystem";
    }
};

// 注册系统
REGISTER_SYSTEM_PRE_UPDATE(PreUpdateTestSystem, 0, 10);
REGISTER_SYSTEM_UPDATE(UpdateTestSystem, 0, 5);
REGISTER_SYSTEM_POST_UPDATE(PostUpdateTestSystem, 0, 1);
REGISTER_SYSTEM_PRE_UPDATE(HighPriorityPreUpdateSystem, 0, 5);

int main()
{
    std::cout << "=== 测试阶段执行系统 ===" << std::endl;
    
    // 创建系统管理器
    SystemManager manager;
    
    // 添加测试系统
    manager.add_system("PreUpdateTestSystem", std::make_unique<PreUpdateTestSystem>());
    manager.add_system("UpdateTestSystem", std::make_unique<UpdateTestSystem>());
    manager.add_system("PostUpdateTestSystem", std::make_unique<PostUpdateTestSystem>());
    manager.add_system("HighPriorityPreUpdateSystem", std::make_unique<HighPriorityPreUpdateSystem>());
    
    // 初始化系统管理器
    manager.initialize();
    
    std::cout << "\n=== 使用阶段执行 ===" << std::endl;
    
    // 创建ECS注册表
    entt::registry registry;
    float delta_time = 0.016f; // 60 FPS
    
    // 按阶段执行系统
    manager.execute_phase_systems(registry, delta_time, SystemExecutionPhase::PRE_UPDATE);
    manager.execute_phase_systems(registry, delta_time, SystemExecutionPhase::UPDATE);
    manager.execute_phase_systems(registry, delta_time, SystemExecutionPhase::POST_UPDATE);
    
    std::cout << "\n=== 使用传统执行（对比） ===" << std::endl;
    
    // 传统方式执行（应该按照依赖关系执行）
    manager.update_systems(registry, delta_time);
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    
    return 0;
}