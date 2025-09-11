#pragma once

/**
 * 调试系统编译配置
 * 控制调试功能的编译时启用/禁用
 */

// 简化的主调试开关 - 只依赖 PORTAL_TEMPLATE_DEBUG
#ifdef PORTAL_TEMPLATE_DEBUG
    #define PORTAL_DEBUG_ENABLED
    #define PORTAL_DEBUG_GUI_ENABLED
    #define PORTAL_DEBUG_WORLD_ENABLED
    #define PORTAL_DEBUG_PROFILING_ENABLED
    #define PORTAL_DEBUG_VERBOSE_LOGGING
    #define PORTAL_PHYSICS_DEBUG_ENABLED
#endif

// Jolt物理调试渲染配置
#ifdef PORTAL_PHYSICS_DEBUG_ENABLED
    // 启用Jolt的调试渲染器
    #define JPH_DEBUG_RENDERER
    
    // Jolt调试功能开关
    #define PORTAL_PHYSICS_DEBUG_BODIES
    #define PORTAL_PHYSICS_DEBUG_CONSTRAINTS
    #define PORTAL_PHYSICS_DEBUG_CONTACTS
    #define PORTAL_PHYSICS_DEBUG_MOTION_QUALITY
    #define PORTAL_PHYSICS_DEBUG_SOFT_BODY
#endif

// 调试宏定义
#ifdef PORTAL_DEBUG_ENABLED
    #define PORTAL_DEBUG_LOG(msg) std::cout << "[DEBUG] " << msg << std::endl
    #define PORTAL_DEBUG_WARN(msg) std::cerr << "[WARNING] " << msg << std::endl
    #define PORTAL_DEBUG_ERROR(msg) std::cerr << "[ERROR] " << msg << std::endl
    #define PORTAL_DEBUG_ASSERT(condition) assert(condition)
#else
    #define PORTAL_DEBUG_LOG(msg) ((void)0)
    #define PORTAL_DEBUG_WARN(msg) ((void)0)
    #define PORTAL_DEBUG_ERROR(msg) ((void)0)
    #define PORTAL_DEBUG_ASSERT(condition) ((void)0)
#endif

// ImGui相关的包含和前向声明
#ifdef PORTAL_DEBUG_GUI_ENABLED
    // 启用 ImGui docking 功能
    #define IMGUI_HAS_DOCK
    #include "imgui.h"
#else
    // 未启用时的前向声明
    struct ImGuiContext;
    struct ImGuiIO;
    struct ImDrawData;
    struct ImVec2;
    struct ImVec4;
    typedef int ImGuiWindowFlags;
    typedef int ImGuiConfigFlags;
    typedef int ImGuiTableFlags;
    typedef int ImGuiTableColumnFlags;
    typedef int ImGuiCond;
#endif
