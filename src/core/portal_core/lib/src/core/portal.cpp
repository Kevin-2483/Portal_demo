#include "../../include/core/portal.h"
#include "../../include/math/portal_math.h"
#include <iostream>

namespace Portal {

Portal::Portal(PortalId id)
    : id_(id)
    , linked_portal_id_(INVALID_PORTAL_ID)
    , is_active_(true)
    , is_recursive_(false)
    , max_recursion_depth_(3)
{
    std::cout << "Portal " << id_ << " created" << std::endl;
}

bool Portal::is_point_in_bounds(const Vector3& point) const {
    return Math::PortalMath::is_point_in_portal_bounds(point, plane_);
}

void Portal::get_corner_points(Vector3 corners[4]) const {
    Math::PortalMath::get_portal_corners(plane_, corners);
}

bool Portal::is_facing_position(const Vector3& position, PortalFace face) const {
    Vector3 face_normal = get_face_normal(face);
    Vector3 to_position = position - plane_.center;
    return face_normal.dot(to_position) > 0.0f;
}

} // namespace Portal