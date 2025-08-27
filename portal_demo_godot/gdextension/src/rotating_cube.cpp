#include "rotating_cube.h"

#include <godot_cpp/classes/box_mesh.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/vector3.hpp>

using namespace godot;

void RotatingCube::_bind_methods() {
    // 可以选择在这里绑定属性，例如旋转速度
    ClassDB::bind_method(D_METHOD("set_rotation_speed", "speed"), &RotatingCube::set_rotation_speed);
    ClassDB::bind_method(D_METHOD("get_rotation_speed"), &RotatingCube::get_rotation_speed);
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rotation_speed"), "set_rotation_speed", "get_rotation_speed");
}

// 构造函数：创建节点时被调用
RotatingCube::RotatingCube() {
    rotation_speed = 1.0;

    // --- [核心修正] ---
    // 不再手动创建顶点，而是直接使用 Godot 内置的 BoxMesh，这更简单也更稳定。
    Ref<BoxMesh> box_mesh;
    box_mesh.instantiate();
    // 将这个新创建的网格资源赋值给我们的节点
    set_mesh(box_mesh);

    // 使用 Ref<> 和 .instantiate() 来创建材质，这是 Godot 4 的标准做法
    Ref<StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(Color(0.8, 0.3, 0.3)); // 设置为红色

    // 将材质应用到网格上
    // set_surface_override_material 是 MeshInstance3D 的方法，用于设置材质
    set_surface_override_material(0, material);
    // --- [修正结束] ---
}

RotatingCube::~RotatingCube() {
}

void RotatingCube::set_rotation_speed(double speed) {
    rotation_speed = speed;
}

double RotatingCube::get_rotation_speed() const {
    return rotation_speed;
}

void RotatingCube::_process(double delta) {
    // 绕 Y 轴旋转
    rotate_y(rotation_speed * delta);
}