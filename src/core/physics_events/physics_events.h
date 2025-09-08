#pragma once

/**
 * Portal Demo 物理事件系统
 * 
 * 这个头文件汇总了所有物理事件相关的类型和组件，
 * 提供了支持2D/3D的统一物理事件接口
 */

// 基础事件类型
#include "physics_event_types.h"

// 队列事件类型
#include "queued_event_types.h"

// 实体事件组件
#include "entity_event_components.h"

// 查询和配置组件
#include "query_components.h"

namespace portal_core {

// 便于使用的类型别名
namespace PhysicsEvents {
    // 立即事件
    using CollisionStart = CollisionStartEvent;
    using CollisionEnd = CollisionEndEvent;
    using TriggerEnter = TriggerEnterEvent;
    using TriggerExit = TriggerExitEvent;
    
    // 队列事件
    using RaycastResult = RaycastResultEvent;
    using ShapeQueryResult = ShapeQueryResultEvent;
    using BodyActivation = BodyActivationEvent;
    using DistanceQueryResult = DistanceQueryResultEvent;
    using OverlapQueryResult = OverlapQueryResultEvent;
    
    // 查询请求
    using RequestRaycast = RequestRaycastEvent;
    using RequestAreaMonitoring = RequestAreaMonitoringEvent;
}

namespace PhysicsComponents {
    // 实体事件组件
    using AreaMonitor = AreaMonitorComponent;
    using Containment = ContainmentComponent;
    using PlaneIntersection = PlaneIntersectionComponent;
    using PersistentContact = PersistentContactComponent;
    using AreaStatusUpdate = AreaStatusUpdateComponent;
    
    // 查询组件
    using PhysicsQuery = PhysicsQueryComponent;
    using PendingQuery = PendingQueryTag;
}

// 工具函数
namespace PhysicsEventUtils {
    
    /**
     * 自动检测事件的维度类型
     */
    inline PhysicsEventDimension detect_dimension(const Vec3& position) {
        // 如果Z坐标为0或接近0，认为是2D
        if (std::abs(position.GetZ()) < 0.001f) {
            return PhysicsEventDimension::DIMENSION_2D;
        }
        return PhysicsEventDimension::DIMENSION_3D;
    }
    
    /**
     * 检查两个事件是否为同一维度
     */
    inline bool same_dimension(const PhysicsEventBase& event1, const PhysicsEventBase& event2) {
        if (event1.dimension == PhysicsEventDimension::AUTO_DETECT || 
            event2.dimension == PhysicsEventDimension::AUTO_DETECT) {
            return true;  // 自动检测的情况下认为兼容
        }
        return event1.dimension == event2.dimension;
    }
    
    /**
     * 检查事件是否支持指定维度
     */
    inline bool supports_dimension(const PhysicsEventBase& event, PhysicsEventDimension target_dim) {
        if (event.dimension == PhysicsEventDimension::AUTO_DETECT) {
            return true;
        }
        return event.dimension == target_dim;
    }
}

} // namespace portal_core
