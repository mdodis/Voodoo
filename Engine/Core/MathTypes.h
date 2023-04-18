#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

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

    static inline Vec3 one() { return Vec3(1.f); }
    static inline Vec3 zero() { return Vec3(0.f); }
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
    Vec4(const glm::vec4& v) : glm::vec4(v) {}

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
};

static _inline Mat4 operator*(const Mat4& left, const Mat4& right)
{
    return glm::mat4(left) * glm::mat4(right);
}

static _inline Vec3 operator*(const Mat4& left, const Vec3& right)
{
    glm::vec4 v4(right, 1.0f);
    v4 = glm::mat4(left) * v4;
    return Vec3(v4.x, v4.y, v4.z);
}

struct Quat : public glm::quat {
    Quat() {}
    Quat(float w, float x, float y, float z) : glm::quat(w, x, y, z) {}
    Quat(float w, Vec3 v) : Quat(w, v.x, v.y, v.z) {}

    static inline Quat identity() { return Quat(1, 0, 0, 0); }

    inline Mat4 matrix() const { return glm::toMat4(glm::quat(*this)); }
};

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

static _inline float absolute(float v) { return glm::abs(v); }

_inline Mat4 Mat4::make_transform(
    const Vec3& position, const Quat& rotation, const Vec3& scale)
{
    return glm::translate(glm::mat4(1.0f), position) *
           glm::mat4(rotation.matrix()) * glm::scale(glm::mat4(1.0f), scale);
}

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

struct Plane {
    Vec3  normal;
    float d;

    Vec3 point() const
    {
        Vec3 point;
        point.x = 0;
        point.y = (-d - normal.x * point.x) / normal.y;
        point.z = (-d - normal.x * point.x - normal.y * point.y) / normal.z;
        return point;
    }

    Plane(Vec3 normal, float d) : normal(normal), d(d) {}
    Plane(Vec3 normal, Vec3 point) : normal(normal) { d = -dot(normal, point); }
};

struct OBB {
    Vec3 center;
    Quat rotation;
    Vec3 extents;
};

static inline bool intersect_ray_plane(
    const Ray& ray, const Plane& plane, Vec3& intersection_point)
{
    float denom = dot(ray.direction, plane.normal);

    if (absolute(denom) > 0.00001) {
        float t = dot(plane.point() - ray.origin, plane.normal) / denom;

        intersection_point = ray.origin + ray.direction * t;
        return true;
    }

    return false;
}

static inline bool intersect_ray_obb(const Ray& ray, const OBB& obb)
{
    Mat4 world_to_obb =
        Mat4::make_transform(obb.center, obb.rotation, Vec3::one()).inverse();
    Ray r = {
        .origin    = world_to_obb * ray.origin,
        .direction = world_to_obb * ray.direction,
    };

    Vec3 half_extents = obb.extents * 0.5f;

    float tmin_x = (-half_extents.x - r.origin.x) / r.direction.x;
    float tmax_x = (+half_extents.x - r.origin.x) / r.direction.x;
    float tmin_y = (-half_extents.y - r.origin.y) / r.direction.y;
    float tmax_y = (+half_extents.y - r.origin.y) / r.direction.y;
    float tmin_z = (-half_extents.z - r.origin.z) / r.direction.z;
    float tmax_z = (+half_extents.z - r.origin.z) / r.direction.z;

    float tmin = glm::max(
        glm::max(glm::min(tmin_x, tmax_x), glm::min(tmin_y, tmax_y)),
        glm::min(tmin_z, tmax_z));

    float tmax = glm::min(
        glm::min(glm::max(tmin_x, tmax_x), glm::max(tmin_y, tmax_y)),
        glm::max(tmin_z, tmax_z));

    return (tmax >= tmin && tmax >= 0.f);
}
