#include "ipresettable_resource.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void IPresettableResource::_bind_methods() {
    // 绑定约束检查方法
    ClassDB::bind_method(D_METHOD("get_constraint_warnings"), &IPresettableResource::get_constraint_warnings);
    
    // 绑定自动填充相关方法 - 只绑定 IPresettableResource 自己的虚方法
    ClassDB::bind_method(D_METHOD("get_auto_fill_capabilities"), &IPresettableResource::get_auto_fill_capabilities);
    ClassDB::bind_method(D_METHOD("can_auto_fill_from_node", "target_node", "capability_name"), &IPresettableResource::can_auto_fill_from_node, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("auto_fill_from_node", "target_node", "capability_name"), &IPresettableResource::auto_fill_from_node, DEFVAL(""));
    
    // 绑定预设相关方法
    ClassDB::bind_method(D_METHOD("get_preset_directory_name"), &IPresettableResource::get_preset_directory_name);
    ClassDB::bind_method(D_METHOD("get_preset_display_name"), &IPresettableResource::get_preset_display_name);
}
