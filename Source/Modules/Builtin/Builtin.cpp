#include "Builtin.h"

#include "Engine/Engine.h"
#include "Engine/SubsystemManager.h"
#include "Renderer/Renderer.h"
#include "WorldRenderSubsystem.h"
DEFINE_DESCRIPTOR_OF(Mat4);

void apply_transform(
    flecs::entity             entity,
    const TransformComponent* parent,
    TransformComponent&       transform)
{
    if (parent) {
        Mat4 parent_matrix = Mat4::make_transform(
            parent->world_position,
            parent->world_rotation,
            parent->world_scale);

        Mat4 child_matrix = Mat4::make_transform(
            transform.position,
            transform.rotation,
            transform.scale);

        Mat4 result              = parent_matrix * child_matrix;
        transform.world_to_local = parent_matrix.inverse();
        glm::vec3 world_position;
        glm::quat world_rotation;
        glm::vec3 world_scale;
        glm::vec3 world_skew;
        glm::vec4 world_perspective;
        glm::decompose(
            glm::mat4(result),
            world_scale,
            world_rotation,
            world_position,
            world_skew,
            world_perspective);

        transform.world_position = Vec3(world_position);
        transform.world_rotation = Quat(world_rotation);
        transform.world_scale    = Vec3(world_scale);

    } else {
        transform.world_position = Vec3(transform.position);
        transform.world_rotation = Quat(transform.rotation);
        transform.world_scale    = Vec3(transform.scale);
        transform.world_to_local = Mat4::identity();
    }
}

TransformComponent make_transform_zero()
{
    return TransformComponent{
        .position = Vec3::zero(),
        .rotation = Quat::identity(),
        .scale    = Vec3::one(),
    };
}

TransformComponent make_transform_position(const Vec3& position)
{
    return TransformComponent{
        .position = position,
        .rotation = Quat::identity(),
        .scale    = Vec3::one(),
    };
}

Vec3 get_world_position(const TransformComponent& transform)
{
    return transform.world_position;
}
Quat get_world_rotation(const TransformComponent& transform)
{
    return transform.world_rotation;
}
Vec3 get_world_scale(const TransformComponent& transform)
{
    return transform.world_scale;
}

void set_world_position(TransformComponent& transform, Vec3 position)
{
    transform.position = transform.world_to_local.transform_point(position);
}

void set_world_rotation(TransformComponent& transform, Quat rotation)
{
    transform.rotation =
        glm::quat_cast(transform.world_to_local * rotation.matrix());
}

void set_world_scale(TransformComponent& transform, Vec3 scale)
{
    transform.scale = transform.world_to_local * transform.scale;
}

Mat4 get_local_matrix(const TransformComponent& transform)
{
    return Mat4::make_transform(
        transform.position,
        transform.rotation,
        transform.scale);
}

void builtin_init()
{
    Engine* engine = Engine::instance();

    engine->subsystems->register_subsystem<WorldRenderSubsystem>();
}
