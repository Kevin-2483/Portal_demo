#ifndef COMPONENT_REGISTRAR_H
#define COMPONENT_REGISTRAR_H

#include <godot_cpp/core/class_db.hpp>
#include <functional>
#include <vector>

// 定义一个函数指针类型，用于注册
using RegistrationFunction = std::function<void()>;

// 声明一个全局列表来存储所有注册函数
// 我们将其声明为 inline，以避免多重定义链接错误
inline std::vector<RegistrationFunction> &get_registration_functions()
{
  static std::vector<RegistrationFunction> functions;
  return functions;
}

// 修改注册器，现在它只添加函数到列表，而不直接调用
template <typename T>
struct ComponentRegistrar
{
  ComponentRegistrar()
  {
    // 将一个 lambda 函数（它捕获了类型 T）添加到列表中
    get_registration_functions().push_back([]()
                                           { godot::ClassDB::register_class<T>(); });
  }
};

// 宏保持不变
#define REGISTER_COMPONENT_RESOURCE(ClassName)                  \
  namespace                                                     \
  {                                                             \
    static ComponentRegistrar<ClassName> registrar_##ClassName; \
  }

#endif // COMPONENT_REGISTRAR_H