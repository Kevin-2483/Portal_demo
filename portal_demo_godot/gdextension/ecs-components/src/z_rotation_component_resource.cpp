#include "z_rotation_component_resource.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/control.hpp>
#include "component_registrar.h"

using namespace godot;

void ZRotationComponentResource::_bind_methods()
{
  ClassDB::bind_method(D_METHOD("set_speed", "speed"), &ZRotationComponentResource::set_speed);
  ClassDB::bind_method(D_METHOD("get_speed"), &ZRotationComponentResource::get_speed);

  ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed", PROPERTY_HINT_RANGE, "-10.0,10.0,0.1"), "set_speed", "get_speed");
}

ZRotationComponentResource::ZRotationComponentResource()
{
  speed = 1.0f;
}

ZRotationComponentResource::~ZRotationComponentResource()
{
  // Cleanup if needed
}

void ZRotationComponentResource::set_speed(float p_speed)
{
  if (speed != p_speed)
  {
    speed = p_speed;
    emit_changed(); // 發出 changed 信號
    UtilityFunctions::print("ZRotation: Speed changed to ", speed);
  }
}

float ZRotationComponentResource::get_speed() const
{
  return speed;
}

// 實現基類的純虛函數
bool ZRotationComponentResource::apply_to_entity(entt::registry& registry, entt::entity entity)
{
  auto &comp = registry.emplace_or_replace<portal_core::ZRotationComponent>(entity, speed);
  UtilityFunctions::print("ZRotationComponentResource: Applied ZRotationComponent with speed: ", speed);
  return true;
}

bool ZRotationComponentResource::remove_from_entity(entt::registry& registry, entt::entity entity)
{
  if (registry.any_of<portal_core::ZRotationComponent>(entity)) {
    registry.remove<portal_core::ZRotationComponent>(entity);
    UtilityFunctions::print("ZRotationComponentResource: Removed ZRotationComponent");
    return true;
  }
  return false; // 組件不存在
}

bool ZRotationComponentResource::has_component(const entt::registry& registry, entt::entity entity) const
{
  return registry.any_of<portal_core::ZRotationComponent>(entity);
}

String ZRotationComponentResource::get_component_type_name() const
{
  return "ZRotationComponent";
}

// 🌟 實現新的同步方法 - 展示新架構的強大能力！
void ZRotationComponentResource::sync_to_node(entt::registry& registry, entt::entity entity, Node* target_node)
{
//   // 獲取 ECS 組件數據
//   auto* z_rotation_comp = registry.try_get<portal_core::ZRotationComponent>(entity);
//   if (!z_rotation_comp)
//   {
//     return; // 沒有找到對應的ECS組件，跳過同步
//   }

//   // 🎯 新架構的核心優勢：支持多種不同類型的 Godot 節點！
  
//   // 1. 如果目標是 Node3D (3D 物件)
//   Node3D* target_3d = Object::cast_to<Node3D>(target_node);
//   if (target_3d)
//   {
//     Vector3 current_rotation = target_3d->get_rotation();
//     current_rotation.z = z_rotation_comp->current_rotation; // 只更新 Z 軸
//     target_3d->set_rotation(current_rotation);
//     return;
//   }

//   // 2. 如果目標是 Node2D (2D 精靈) - Z軸是2D中的主要旋轉軸
//   Node2D* target_2d = Object::cast_to<Node2D>(target_node);
//   if (target_2d)
//   {
//     target_2d->set_rotation(z_rotation_comp->current_rotation);
//     return;
//   }

//   // 3. 如果目標是 Control (UI 元素) - UI也主要使用Z軸旋轉
//   Control* target_ui = Object::cast_to<Control>(target_node);
//   if (target_ui)
//   {
//     // 對於 UI 元素，將弧度轉為角度
//     float rotation_degrees = Math::rad_to_deg(z_rotation_comp->current_rotation);
//     target_ui->set_rotation_degrees(rotation_degrees);
//     return;
//   }
}

REGISTER_COMPONENT_RESOURCE(ZRotationComponentResource)