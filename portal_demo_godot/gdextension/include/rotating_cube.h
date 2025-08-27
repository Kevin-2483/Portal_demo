#ifndef ROTATING_CUBE_H
#define ROTATING_CUBE_H

#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/core/binder_common.hpp>

// 命名空间在之前的版本中可能缺失，加上可以避免潜在的冲突
namespace godot {

class RotatingCube : public MeshInstance3D {
    GDCLASS(RotatingCube, MeshInstance3D)

private:
    // 旋转速度属性
    double rotation_speed;

protected:
    static void _bind_methods();

public:
    RotatingCube();
    ~RotatingCube();

    void _process(double delta) override;

    // --- 以下是补上的函数声明 ---
    void set_rotation_speed(double speed);
    double get_rotation_speed() const; // getter 函数通常声明为 const
    // --- 声明结束 ---
};

} // namespace godot

#endif // ROTATING_CUBE_H