#pragma once
#include "Core/Handle.h"
#include "Core/MathTypes.h"
#include "ECS/ECS.h"
#include "Engine/AssetSystem.h"

struct StaticMeshComponent2 {
    AssetProxy    asset;
    THandle<Mesh> mesh;
};

struct Mat4Descriptor : FixedArrayDescriptor<Mat4, float, 16> {
    constexpr Mat4Descriptor(u64 offset = 0, Str name = Str::NullStr)
        : FixedArrayDescriptor<Mat4, float, 16>(offset, name)
    {}
};
DECLARE_DESCRIPTOR_OF(Mat4);

#include "Builtin.generated.h"

#if METADESK
// clang-format off

@component(nodefine)
Vec3: {
    x: f32;
    y: f32;
    z: f32;
}

@component(nodefine)
Vec4: {
    x: f32;
    y: f32;
    z: f32;
    w: f32;
}

@component(nodefine)
Quat: {
    w: f32;
    x: f32;
    y: f32;
    z: f32;
}

@component(nodefine, nodescriptor)
Mat4: {}

@component(nodefine)
AssetID: {
    id: SflUUID;
}

@component(nodefine)
AssetReference: {
    module: Str;
    path: Str;
}

@component(nodefine)
AssetProxy: {
    ref: AssetReference;
    cached_id: AssetID;
}

@component()
TransformComponent2: {

    position: Vec3;
    rotation: Quat;
    scale: Vec3;

    world_position: Vec3;
    world_rotation: Quat;
    world_scale: Vec3;
    world_to_local: Mat4;
}

@component(nodefine)
StaticMeshComponent2: {
    asset: AssetProxy;
}

@component()
MaterialComponent2: {
    asset: AssetProxy;
}

@system(OnUpdate)
apply_transform: {
    @access(in)
    @source(id: this, trav: ChildOf, flags: (up, cascade))
    @op(optional)
    parent: TransformComponent2;

    transform: TransformComponent2;
}

@hook(init) builtin_init;

// clang-format on
#endif

Vec3 get_world_position(const TransformComponent2& transform);
Quat get_world_rotation(const TransformComponent2& transform);
Vec3 get_world_scale(const TransformComponent2& transform);

void set_world_position(TransformComponent2& transform, Vec3 position);
void set_world_rotation(TransformComponent2& transform, Quat rotation);
void set_world_scale(TransformComponent2& transform, Vec3 scale);
Mat4 get_local_matrix(const TransformComponent2& transform);