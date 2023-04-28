#include "TransformSystem.h"

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