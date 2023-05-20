#pragma once
#include "Host.h"

#if COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#endif

#include <glm/ext/scalar_constants.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/intersect.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/projection.hpp>
#include <glm/gtx/quaternion.hpp>

#if COMPILER_CLANG
#pragma clang diagnostic pop
#endif

#include "Debugging/Assertions.h"

struct Vec2 : public glm::vec2 {
    Vec2() {}
    Vec2(float x, float y) : glm::vec2(x, y) {}
    Vec2(float s) : Vec2(s, s) {}
    Vec2(const glm::vec2& v) : Vec2(v.x, v.y) {}
};

struct Vec3 : public glm::vec3 {
    Vec3() {}
    Vec3(float x, float y, float z) : glm::vec3(x, y, z) {}
    Vec3(float s) : Vec3(s, s, s) {}
    Vec3(const Vec2& v, float z) : Vec3(v.x, v.y, z) {}
    Vec3(const glm::vec3& v) : Vec3(v.x, v.y, v.z) {}

    inline Vec2 xy() { return Vec2(x, y); }

    static inline Vec3 one() { return Vec3(1.f); }
    static inline Vec3 zero() { return Vec3(0.f); }

    static inline Vec3 up() { return Vec3(0, 1, 0); }
    static inline Vec3 down() { return Vec3(0, -1, 0); }
    static inline Vec3 right() { return Vec3(1, 0, 0); }
    static inline Vec3 left() { return Vec3(-1, 0, 0); }
    static inline Vec3 forward() { return Vec3(0, 0, -1); }
    static inline Vec3 back() { return Vec3(0, 0, 1); }

    float* ptr() { return glm::value_ptr(*((glm::vec3*)this)); }
};

static _inline Vec3 operator+(const Vec3& left, const Vec3& right)
{
    return glm::vec3(left) + glm::vec3(right);
}

static _inline Vec3 operator-(const Vec3& left, const Vec3& right)
{
    return glm::vec3(left) - glm::vec3(right);
}

static _inline Vec3 operator*(const Vec3& left, float right)
{
    return glm::vec3(left) * right;
}

static _inline Vec3 operator*(float left, const Vec3& right)
{
    return glm::vec3(right) * left;
}

struct Vec4 : public glm::vec4 {
    Vec4() {}
    Vec4(float x, float y, float z, float w) : glm::vec4(x, y, z, w) {}
    Vec4(float s) : Vec4(s, s, s, 2) {}
    Vec4(const Vec2& xy, const Vec2& zw) : glm::vec4(xy, zw) {}
    Vec4(const Vec2& xy, float z, float w) : Vec4(xy, glm::vec2(z, w)) {}
    Vec4(const glm::vec4& v) : glm::vec4(v) {}

    Vec2 xy() const { return Vec2(x, y); }
    Vec3 xyz() const { return Vec3(x, y, z); }
};

static _inline Vec4 operator+(const Vec4& left, const Vec4& right)
{
    return glm::vec4(left) + glm::vec4(right);
}

static _inline Vec4 operator-(const Vec4& left, const Vec4& right)
{
    return glm::vec4(left) - glm::vec4(right);
}

static _inline Vec4 operator*(const Vec4& left, float right)
{
    return glm::vec4(left) * right;
}

static _inline Vec4 operator*(float left, const Vec4& right)
{
    return glm::vec4(right) * left;
}

struct Mat4 : public glm::mat4 {
    Mat4() {}
    Mat4(
        float x0,
        float y0,
        float z0,
        float w0,
        float x1,
        float y1,
        float z1,
        float w1,
        float x2,
        float y2,
        float z2,
        float w2,
        float x3,
        float y3,
        float z3,
        float w3)
        : glm::mat4(
              x0, y0, z0, w0, x1, y1, z1, w1, x2, y2, z2, w2, x3, y3, z3, w3)
    {}
    Mat4(float diagonal) : glm::mat4(diagonal) {}
    Mat4(const glm::mat4& m) : glm::mat4(m) {}

    static _inline Mat4 make_transform(
        const Vec3& position, const struct Quat& rotation, const Vec3& scale);

    _inline Mat4 inverse() const { return glm::inverse(*this); }

    void decompose(Vec3& position, struct Quat& rotation, Vec3& scale) const;

    const float* ptr() const { return glm::value_ptr(glm::mat4(*this)); }

    static _inline Mat4 identity() { return Mat4(1.0f); }

    _inline Vec3 transform_point(Vec3 point)
    {
        Vec4 v(point.x, point.y, point.z, 1.0f);
        v = glm::mat4(*this) * v;
        return Vec3(v.xyz());
    }

    _inline Vec3 transform_direction(Vec3 direction)
    {
        Vec4 v(direction.x, direction.y, direction.z, 0.0f);
        v = glm::mat4(*this) * v;
        return Vec3(v.xyz());
    }
};

static _inline Mat4 operator*(const Mat4& left, const Mat4& right)
{
    return glm::mat4(left) * glm::mat4(right);
}

static _inline Vec3 operator*(const Mat4& left, const Vec3& right)
{
    glm::vec4 v4(right, 0.0f);
    v4 = glm::mat4(left) * v4;
    return Vec3(v4.x, v4.y, v4.z);
}

static _inline Vec3 operator*(const Vec3& left, const Mat4& right)
{
    glm::vec4 v4(left, 1.0f);
    v4 = v4 * glm::mat4(right);
    return Vec3(v4.x, v4.y, v4.z);
}

struct Quat : public glm::quat {
    Quat() {}
    Quat(float w, float x, float y, float z) : glm::quat(w, x, y, z) {}
    Quat(float w, Vec3 v) : Quat(w, v.x, v.y, v.z) {}
    Quat(const glm::quat& q) : glm::quat(q) {}

    static inline Quat identity() { return Quat(1, 0, 0, 0); }
    static inline Quat angle_axis(float angle, Vec3 axis)
    {
        return Quat(glm::angleAxis(glm::radians(angle), axis));
    }

    inline Quat inverse() const { return glm::inverse(*this); }
    inline Mat4 matrix() const { return glm::toMat4(glm::quat(*this)); }
};

static _inline Vec3 operator*(const Quat& left, const Vec3& right)
{
    return glm::quat(left) * glm::vec3(right);
}

static _inline Vec3 normalize(const Vec3& v)
{
    return glm::normalize(glm::vec3(v));
}

static _inline float dot(const Vec3& a, const Vec3& b)
{
    return glm::dot(glm::vec3(a), glm::vec3(b));
}

static _inline Vec3 cross(const Vec3& a, const Vec3& b)
{
    return glm::cross(glm::vec3(a), glm::vec3(b));
}

static _inline Vec3 project(const Vec3& v, const Vec3& n)
{
    return glm::proj(glm::vec3(v), glm::vec3(n));
}

static _inline float absolute(float v) { return glm::abs(v); }

_inline Mat4 Mat4::make_transform(
    const Vec3& position, const Quat& rotation, const Vec3& scale)
{
    return glm::translate(glm::mat4(1.0f), position) *
           glm::mat4(rotation.matrix()) * glm::scale(glm::mat4(1.0f), scale);
}

_inline void Mat4::decompose(Vec3& position, Quat& rotation, Vec3& scale) const
{
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 scl;
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(*this, scl, rot, pos, skew, perspective);

    position = pos;
    rotation = rot;
    scale    = scl;
}

struct Ray {
    Vec3 origin;
    Vec3 direction;

    Vec3 at(float t) const { return origin + direction * t; }
};

struct Plane {
    Vec3 normal;
    Vec3 center;

    Plane() {}
    Plane(Vec3 normal, Vec3 point) : normal(normal), center(point) {}
#if 0
    Vec3 point() const
    {
        Vec3 point(1, 2, 3);

        float distance =
            point.x * normal.x + point.y * normal.y + point.z * normal.z - d;

        if (absolute(distance) > FLT_EPSILON) {
            return Vec3{
                point.x - distance * normal.x,
                point.y - distance * normal.y,
                point.z - distance * normal.z,
            };

        } else {
            return point;
        }

        return point;
    }
#endif
    Quat orientation() const
    {
        Vec3  up = Vec3::up();
        float d  = dot(up, normal);
        if (d > 0.99f || d < -0.99f) {
            up = Vec3::right();
        }

        Vec3 right = normalize(cross(up, normal));
        up         = normalize(cross(normal, right));

        return Quat(glm::quatLookAt(normal, up));
    }
};

struct AABB {
    Vec3 center;
    Vec3 extents;

    inline Vec3 min() const { return center - (extents * 0.5f); }
    inline Vec3 max() const { return center + (extents * 0.5f); }
};

struct OBB {
    Vec3 center;
    Quat rotation;
    Vec3 extents;
};

static inline bool intersect_ray_plane(
    const Ray& ray, const Plane& plane, float& t)
{
#if 1
    bool result = glm::intersectRayPlane(
        glm::vec3(ray.origin),
        glm::vec3(ray.direction),
        glm::vec3(plane.center),
        glm::vec3(plane.normal),
        t);
    return result;
#else
    float denom = dot(plane.normal, ray.direction);
    if (absolute(denom) > 0.0001f) {
        float t =
    }
#endif
}

static thread_local float Intersect_Ray_AABB_Dummy;

static inline bool intersect_ray_aabb(
    const Ray&  ray,
    const AABB& aabb,
    float&      intersection0 = Intersect_Ray_AABB_Dummy,
    float&      intersection1 = Intersect_Ray_AABB_Dummy)
{
    float tmin = -INFINITY;
    float tmax = +INFINITY;

    if (ray.direction.x != 0.0f) {
        float tx1 = (aabb.min().x - ray.origin.x) / ray.direction.x;
        float tx2 = (aabb.max().x - ray.origin.x) / ray.direction.x;

        tmin = glm::max(tmin, glm::min(tx1, tx2));
        tmax = glm::min(tmax, glm::max(tx1, tx2));
    }

    if (ray.direction.y != 0.0f) {
        float ty1 = (aabb.min().y - ray.origin.y) / ray.direction.y;
        float ty2 = (aabb.max().y - ray.origin.y) / ray.direction.y;

        tmin = glm::max(tmin, glm::min(ty1, ty2));
        tmax = glm::min(tmax, glm::max(ty1, ty2));
    }

    if (ray.direction.z != 0.0f) {
        float tz1 = (aabb.min().z - ray.origin.z) / ray.direction.z;
        float tz2 = (aabb.max().z - ray.origin.z) / ray.direction.z;

        tmin = glm::max(tmin, glm::min(tz1, tz2));
        tmax = glm::min(tmax, glm::max(tz1, tz2));
    }

    intersection0 = tmin;
    intersection1 = tmax;
    return (tmax >= tmin);
}

static thread_local float Intersect_Ray_OBB_Dummy;

static inline bool intersect_ray_obb(
    const Ray& ray,
    const OBB& obb,
    float&     intersection0 = Intersect_Ray_OBB_Dummy,
    float&     intersection1 = Intersect_Ray_OBB_Dummy)
{
    Mat4 obb_to_world =
        Mat4::make_transform(obb.center, obb.rotation, Vec3::one());
    Mat4 world_to_obb = obb_to_world.inverse();

    Ray r = {
        .origin    = world_to_obb * ray.origin,
        .direction = obb.rotation.inverse() * ray.direction,
    };

    AABB aabb = {
        .center  = obb.center,
        .extents = obb.extents,
    };

    bool success = intersect_ray_aabb(r, aabb, intersection0, intersection1);
    return success;
}
