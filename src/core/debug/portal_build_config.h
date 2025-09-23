#ifndef PORTAL_BUILD_CONFIG_H
#define PORTAL_BUILD_CONFIG_H

// --- 1. 主调试开关 (由 Scons 的 PORTAL_TEMPLATE_DEBUG 宏决定) ---
#ifdef PORTAL_TEMPLATE_DEBUG
  #define PORTAL_DEBUG_ENABLED 1
#else
  #define PORTAL_DEBUG_ENABLED 0
#endif


// --- 2. 精细化分项调试开关 ---
// 只有在主开关开启的情况下，这些分项开关才可能生效。
#if defined(PORTAL_DEBUG_ENABLED) && PORTAL_DEBUG_ENABLED

    // --- 开发者可以根据需要在此处注释掉某个功能 ---

    // 启用调试GUI (ImGui)
    #define PORTAL_DEBUG_GUI_ENABLED 1


#endif // PORTAL_DEBUG_ENABLED


// --- 平台配置 (保持不变) ---
#if defined(__APPLE__)
  #define PORTAL_PLATFORM_MACOS 1
#elif defined(_WIN32)
  #define PORTAL_PLATFORM_WINDOWS 1
#elif defined(__linux__)
  #define PORTAL_PLATFORM_LINUX 1
#endif

// 3. 为ImGui提供轻量级的前向声明
// 这样其他头文件只需要包含本文件就能知道ImGui的基本类型，而无需包含完整的imgui.h
#if defined(PORTAL_DEBUG_GUI_ENABLED) && PORTAL_DEBUG_GUI_ENABLED
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

#endif // PORTAL_BUILD_CONFIG_H