#pragma once

#include "../math_types.h"
#include <cstdint>
#include <cstddef>
#include <string>

namespace portal_core {
namespace render {

// 使用数学类型
using portal_core::Vector2;
using portal_core::Vector3;
using portal_core::Quaternion;

// 通用渲染命令类型
enum class RenderCommandType : uint32_t {
    // 3D 世界空间绘制
    DRAW_LINE_3D = 0x1000,      // 3D线段
    DRAW_BOX_3D,                // 3D包围盒
    DRAW_SPHERE_3D,             // 3D球体
    DRAW_MESH_3D,               // 3D网格
    DRAW_ARROW_3D,              // 3D箭头
    DRAW_COORDINATE_AXES_3D,    // 3D坐标轴
    
    // 2D 屏幕空间UI绘制
    DRAW_UI_RECT = 0x2000,      // UI矩形
    DRAW_UI_TEXT,               // UI文本
    DRAW_UI_TEXTURE,            // UI贴图
    DRAW_UI_LINE,               // UI线段
    DRAW_UI_CIRCLE,             // UI圆形
    DRAW_UI_WINDOW,             // UI窗口
    DRAW_UI_BUTTON,             // UI按钮
    DRAW_UI_SLIDER,             // UI滑动条
    DRAW_UI_PROGRESS_BAR,       // UI进度条
    DRAW_UI_GRAPH_LINE,         // UI图表线条
    
    // 自定义命令
    CUSTOM_COMMAND = 0x8000     // 自定义命令
};

// 渲染层级枚举
enum class RenderLayer : uint32_t {
    BACKGROUND = 0,
    WORLD_GEOMETRY = 100,
    WORLD_DEBUG = 200,
    UI_BACKGROUND = 1000,
    UI_CONTENT = 1100,
    UI_OVERLAY = 1200,
    UI_TOP_MOST = 1300
};

// 渲染标志位
enum RenderFlags : uint32_t {
    RENDER_FLAG_NONE = 0,
    RENDER_FLAG_DEPTH_TEST = 1 << 0,     // 启用深度测试
    RENDER_FLAG_ALPHA_BLEND = 1 << 1,    // 启用Alpha混合
    RENDER_FLAG_WIREFRAME = 1 << 2,      // 线框模式
    RENDER_FLAG_NO_CULL = 1 << 3,        // 禁用背面剔除
    RENDER_FLAG_PERSISTENT = 1 << 4,     // 持续渲染（不会被自动清理）
    RENDER_FLAG_ONE_FRAME = 1 << 5       // 仅渲染一帧
};

// 颜色结构
struct Color4f {
    float r, g, b, a;
    
    Color4f() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    Color4f(float red, float green, float blue, float alpha = 1.0f) 
        : r(red), g(green), b(blue), a(alpha) {}
    
    // 预定义颜色
    static const Color4f WHITE;
    static const Color4f BLACK;
    static const Color4f RED;
    static const Color4f GREEN;
    static const Color4f BLUE;
    static const Color4f YELLOW;
    static const Color4f CYAN;
    static const Color4f MAGENTA;
    static const Color4f TRANSPARENT;
};

// 3D线段数据
struct Line3DData {
    Vector3 start;
    Vector3 end;
    Color4f color;
    float thickness;
    
    Line3DData() : thickness(1.0f) {}
    Line3DData(const Vector3& s, const Vector3& e, const Color4f& c, float t = 1.0f)
        : start(s), end(e), color(c), thickness(t) {}
};

// 3D盒子数据
struct Box3DData {
    Vector3 center;
    Vector3 size;
    Color4f color;
    bool filled;
    
    Box3DData() : filled(false) {}
    Box3DData(const Vector3& c, const Vector3& s, const Color4f& col, bool f = false)
        : center(c), size(s), color(col), filled(f) {}
};

// 3D球体数据
struct Sphere3DData {
    Vector3 center;
    float radius;
    Color4f color;
    int segments;
    bool filled;
    
    Sphere3DData() : radius(1.0f), segments(16), filled(false) {}
    Sphere3DData(const Vector3& c, float r, const Color4f& col, int seg = 16, bool f = false)
        : center(c), radius(r), color(col), segments(seg), filled(f) {}
};

// UI矩形数据
struct UIRectData {
    Vector2 position;
    Vector2 size;
    Color4f color;
    bool filled;
    float border_width;
    
    UIRectData() : filled(true), border_width(1.0f) {}
    UIRectData(const Vector2& pos, const Vector2& sz, const Color4f& c, bool f = true, float bw = 1.0f)
        : position(pos), size(sz), color(c), filled(f), border_width(bw) {}
};

// UI文本数据
struct UITextData {
    Vector2 position;
    std::string text;
    Color4f color;
    float font_size;
    int align; // 0=left, 1=center, 2=right
    
    UITextData() : font_size(14.0f), align(0) {}
    UITextData(const Vector2& pos, const std::string& txt, const Color4f& c, float size = 14.0f, int a = 0)
        : position(pos), text(txt), color(c), font_size(size), align(a) {}
};

// UI线段数据
struct UILineData {
    Vector2 start;
    Vector2 end;
    Color4f color;
    float thickness;
    
    UILineData() : thickness(1.0f) {}
    UILineData(const Vector2& s, const Vector2& e, const Color4f& c, float t = 1.0f)
        : start(s), end(e), color(c), thickness(t) {}
};

// 统一渲染命令结构
struct UnifiedRenderCommand {
    RenderCommandType type;
    void* data;              // 具体数据指针
    size_t data_size;        // 数据大小
    uint32_t layer;          // 渲染层级
    uint32_t flags;          // 渲染标志
    float duration;          // 持续时间（秒，-1表示永久）
    uint64_t frame_id;       // 创建帧ID
    
    UnifiedRenderCommand() 
        : type(RenderCommandType::CUSTOM_COMMAND)
        , data(nullptr)
        , data_size(0)
        , layer(static_cast<uint32_t>(RenderLayer::WORLD_DEBUG))
        , flags(RENDER_FLAG_NONE)
        , duration(-1.0f)
        , frame_id(0) {}
        
    // 便利构造函数
    template<typename T>
    UnifiedRenderCommand(RenderCommandType cmd_type, const T& cmd_data, uint32_t cmd_layer = static_cast<uint32_t>(RenderLayer::WORLD_DEBUG), uint32_t cmd_flags = RENDER_FLAG_NONE)
        : type(cmd_type)
        , data(const_cast<T*>(&cmd_data))
        , data_size(sizeof(T))
        , layer(cmd_layer)
        , flags(cmd_flags)
        , duration(-1.0f)
        , frame_id(0) {}
};

// 渲染统计信息
struct RenderStats {
    uint32_t total_commands;
    uint32_t commands_3d;
    uint32_t commands_ui;
    uint32_t commands_custom;
    uint32_t total_vertices;
    float frame_time_ms;
    
    RenderStats() : total_commands(0), commands_3d(0), commands_ui(0), commands_custom(0), total_vertices(0), frame_time_ms(0.0f) {}
};

}} // namespace portal_core::render
