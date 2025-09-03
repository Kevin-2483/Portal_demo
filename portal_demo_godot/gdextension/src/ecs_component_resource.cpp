#include "ecs_component_resource.h"
#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void ECSComponentResource::_bind_methods()
{
    // 這是一個抽象基類，不需要綁定具體的屬性
    // 子類會實現自己的 _bind_methods
    
    // 可以綁定一些通用的方法供GDScript調用（如果需要的話）
    ClassDB::bind_method(D_METHOD("get_component_type_name"), &ECSComponentResource::get_component_type_name);
}

ECSComponentResource::ECSComponentResource()
{
    UtilityFunctions::print("ECSComponentResource: Base constructor called");
}

ECSComponentResource::~ECSComponentResource()
{
    UtilityFunctions::print("ECSComponentResource: Base destructor called");
}

String ECSComponentResource::get_component_type_name() const
{
    // 默認實現：返回類名
    return get_class();
}
