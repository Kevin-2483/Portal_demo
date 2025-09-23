#pragma once

#include "ecs_component_resource.h"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

// 引入C++ ECS組件定義
#include "core/components/y_rotation_component.h"

using namespace godot;

/**
 * Y軸旋轉組件資源
 * 讓設計師可以在 Godot 編輯器中配置 Y 軸旋轉行為
 * 現在繼承自ECSComponentResource，支持多態操作
 */
class YRotationComponentResource : public ECSComponentResource
{
  GDCLASS(YRotationComponentResource, ECSComponentResource)

private:
  float speed;

protected:
  static void _bind_methods();

public:
  YRotationComponentResource();
  ~YRotationComponentResource();

  // 屬性訪問器
  void set_speed(float p_speed);
  float get_speed() const;

  // 實現基類的純虛函數
  virtual bool apply_to_entity(entt::registry& registry, entt::entity entity) override;
  virtual bool remove_from_entity(entt::registry& registry, entt::entity entity) override;
  virtual bool has_component(const entt::registry& registry, entt::entity entity) const override;
  virtual String get_component_type_name() const override;
  
  // 🌟 實現新的同步方法 - 支持任意類型的 Godot 節點！
  virtual void sync_to_node(entt::registry& registry, entt::entity entity, Node* target_node) override;
};