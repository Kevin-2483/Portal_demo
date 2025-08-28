#ifndef PORTAL_H
#define PORTAL_H

/**
 * Portal Core Library - 可移植传送门系统
 *
 * 这是一个完全引擎无关的传送门核心模组，专为需要传送门功能的游戏或应用程序设计。
 *
 * 主要特性：
 * - 引擎无关：纯 C++ 实现，不依赖任何特定游戏引擎
 * - 接口驱动：通过抽象接口与宿主应用程序交互
 * - 数学精确：完整的传送门数学变换计算
 * - 高性能：优化的碰撞检测和变换计算
 *
 * 使用方法：
 * 1. 实现必需的接口（IPhysicsQuery, IPhysicsManipulator, IRenderQuery, IRenderManipulator）
 * 2. 创建 PortalManager 实例
 * 3. 调用 initialize() 初始化系统
 * 4. 创建和链接传送门
 * 5. 注册需要传送的实体
 * 6. 每帧调用 update()
 *
 * @version 1.0.0
 * @author Portal Core Team
 */

// 核心类型定义
#include "portal_types.h"

// 抽象接口定义
#include "portal_interfaces.h"

// 数学计算模块
#include "portal_math.h"

// 核心管理器
#include "portal_core.h"

// 可选：包含示例实现（用于学习和测试）
#ifdef PORTAL_INCLUDE_EXAMPLES
#include "portal_example.h"
#endif

/**
 * Portal 命名空间包含所有传送门系统相关的类和函数
 *
 * 核心类：
 * - Portal: 传送门对象
 * - PortalManager: 传送门系统管理器
 * - PortalMath: 数学计算工具类
 *
 * 接口类：
 * - IPhysicsQuery: 物理查询接口
 * - IPhysicsManipulator: 物理操作接口
 * - IRenderQuery: 渲染查询接口
 * - IRenderManipulator: 渲染操作接口
 * - IPortalEventHandler: 事件处理接口
 *
 * 数据类型：
 * - Vector3: 三维向量
 * - Quaternion: 四元数
 * - Transform: 变换矩阵
 * - PortalPlane: 传送门平面定义
 * - PhysicsState: 物理状态
 * - CameraParams: 相机参数
 */

namespace Portal
{

  // 版本信息
  constexpr int VERSION_MAJOR = 1;
  constexpr int VERSION_MINOR = 0;
  constexpr int VERSION_PATCH = 0;

  /**
   * 获取版本字符串
   * @return 格式化的版本字符串 "major.minor.patch"
   */
  inline const char *get_version_string()
  {
    static const char *version = "1.0.0";
    return version;
  }

  /**
   * 检查接口兼容性
   * 宿主应用程序可以调用此函数检查接口是否正确实现
   * @param interfaces 宿主接口实现
   * @return 兼容性检查结果
   */
  inline bool check_interface_compatibility(const HostInterfaces &interfaces)
  {
    return interfaces.is_valid();
  }

} // namespace Portal

#endif // PORTAL_H
