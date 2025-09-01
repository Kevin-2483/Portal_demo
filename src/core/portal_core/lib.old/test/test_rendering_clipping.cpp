#define PORTAL_INCLUDE_EXAMPLES
#include "../lib/include/portal.h"
#include <iostream>
#include <iomanip>

using namespace Portal;
using namespace Portal::Example;

void print_render_pass(const RenderPassDescriptor &desc, int index)
{
  std::cout << "=== Render Pass " << index << " ===" << std::endl;
  std::cout << "  Source Portal ID: " << desc.source_portal_id << std::endl;
  std::cout << "  Recursion Depth: " << desc.recursion_depth << std::endl;
  std::cout << "  Should Clip: " << (desc.should_clip ? "YES" : "NO") << std::endl;

  if (desc.should_clip)
  {
    const auto &plane = desc.clipping_plane;
    std::cout << "  Clipping Plane:" << std::endl;
    std::cout << "    Normal: (" << plane.normal.x << ", " << plane.normal.y << ", " << plane.normal.z << ")" << std::endl;
    std::cout << "    Distance: " << plane.distance << std::endl;
  }

  std::cout << "  Use Stencil: " << (desc.use_stencil_buffer ? "YES" : "NO") << std::endl;
  if (desc.use_stencil_buffer)
  {
    std::cout << "  Stencil Ref: " << desc.stencil_ref_value << std::endl;
  }

  const auto &cam = desc.virtual_camera;
  std::cout << "  Virtual Camera:" << std::endl;
  std::cout << "    Position: (" << cam.position.x << ", " << cam.position.y << ", " << cam.position.z << ")" << std::endl;
  std::cout << "    FOV: " << cam.fov << std::endl;
}

int main()
{
  std::cout << "=== Portal Rendering & Clipping Support Demo ===" << std::endl;

  // 创建示例接口实现
  ExamplePhysicsQuery physics_query;
  ExamplePhysicsManipulator physics_manipulator(&physics_query);
  ExampleRenderQuery render_query;
  ExampleRenderManipulator render_manipulator;
  ExampleEventHandler event_handler;

  // 组装接口
  HostInterfaces interfaces;
  interfaces.physics_query = &physics_query;
  interfaces.physics_manipulator = &physics_manipulator;
  interfaces.render_query = &render_query;
  interfaces.render_manipulator = &render_manipulator;
  interfaces.event_handler = &event_handler;

  // 创建Portal管理器
  PortalManager portal_manager(interfaces);

  // 创建两个传送门
  PortalPlane plane1, plane2;

  // Portal 1: 在原点
  plane1.center = Vector3(0, 0, 0);
  plane1.normal = Vector3(0, 0, 1);
  plane1.right = Vector3(1, 0, 0);
  plane1.up = Vector3(0, 1, 0);

  // Portal 2: 在远处
  plane2.center = Vector3(10, 0, 10);
  plane2.normal = Vector3(0, 0, -1);
  plane2.right = Vector3(-1, 0, 0);
  plane2.up = Vector3(0, 1, 0);

  PortalId portal1 = portal_manager.create_portal(plane1);
  PortalId portal2 = portal_manager.create_portal(plane2);

  // 链接传送门
  bool linked = portal_manager.link_portals(portal1, portal2);
  std::cout << "\nPortals linked: " << (linked ? "SUCCESS" : "FAILED") << std::endl;

  // 创建主相机
  CameraParams main_camera;
  main_camera.position = Vector3(0, 1, -5); // 相机在Portal 1前面
  main_camera.fov = 90.0f;
  main_camera.near_plane = 0.1f;
  main_camera.far_plane = 100.0f;

  std::cout << "\nMain Camera Position: ("
            << main_camera.position.x << ", "
            << main_camera.position.y << ", "
            << main_camera.position.z << ")" << std::endl;

  // 计算渲染通道
  std::cout << "\n=== Computing Render Passes ===" << std::endl;
  std::vector<RenderPassDescriptor> render_passes = portal_manager.calculate_render_passes(main_camera, 2);

  std::cout << "Generated " << render_passes.size() << " render pass(es)" << std::endl;

  // 显示每个渲染通道
  for (size_t i = 0; i < render_passes.size(); ++i)
  {
    std::cout << std::endl;
    print_render_pass(render_passes[i], static_cast<int>(i + 1));
  }

  // 演示引擎渲染流程
  std::cout << "\n=== Simulated Engine Render Loop ===" << std::endl;

  for (size_t i = 0; i < render_passes.size(); ++i)
  {
    const auto &pass = render_passes[i];
    std::cout << "\n--- Rendering Pass " << (i + 1) << " ---" << std::endl;

    // 配置模板缓冲
    render_manipulator.configure_stencil_buffer(pass.use_stencil_buffer, pass.stencil_ref_value);

    // 设置裁剪平面
    if (pass.should_clip)
    {
      render_manipulator.set_clipping_plane(pass.clipping_plane);
    }

    // 设置虚拟相机并渲染
    std::cout << "Rendering scene with virtual camera..." << std::endl;

    // 重置渲染状态
    render_manipulator.reset_render_state();
  }

  // 演示实体裁切功能
  std::cout << "\n=== Entity Clipping Demo ===" << std::endl;

  // 注册一个实体
  EntityId entity = 12345;
  portal_manager.register_entity(entity);

  // 模拟实体开始传送
  portal_manager.update(0.016f); // 16ms 一帧

  // 检查实体是否需要裁切
  ClippingPlane entity_clip;
  bool needs_clipping = portal_manager.get_entity_clipping_plane(entity, entity_clip);

  std::cout << "Entity " << entity << " needs clipping: " << (needs_clipping ? "YES" : "NO") << std::endl;

  if (needs_clipping)
  {
    std::cout << "Entity clipping plane:" << std::endl;
    std::cout << "  Normal: (" << entity_clip.normal.x << ", " << entity_clip.normal.y << ", " << entity_clip.normal.z << ")" << std::endl;
    std::cout << "  Distance: " << entity_clip.distance << std::endl;

    // 引擎會在渲染此實體時使用這個裁切平面
    render_manipulator.set_clipping_plane(entity_clip);
    std::cout << "Rendering entity with clipping..." << std::endl;
    render_manipulator.disable_clipping_plane();
  }

  std::cout << "\n=== Demo Complete ===" << std::endl;
  std::cout << "Your design is excellent! The portal system now supports:" << std::endl;
  std::cout << "✓ Automatic render pass calculation with clipping planes" << std::endl;
  std::cout << "✓ Stencil buffer configuration for portal masking" << std::endl;
  std::cout << "✓ Entity-specific clipping for seamless portal transitions" << std::endl;
  std::cout << "✓ Clean interface separation between engine and library" << std::endl;

  return 0;
}
