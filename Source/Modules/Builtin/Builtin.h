#include "Core/MathTypes.h"

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

@component(nodefine)
Mat4: {
    m: Array: [f32, 16];
}

@component(nodefine)
AssetID: {}

@component(nodefine)
AssetReference: {
    module: String;
    path: String;
}

@component(nodefine)
AssetProxy: {
    ref: AssetReference;
    cached_id: AssetID;
}

@component()
TransformComponent: {

    position: Vec3;
    rotation: Quat;
    scale: Vec3;

    world_position: Vec3;
    world_rotation: Quat;
    world_scale: Vec3;
    world_to_local: Mat4;
}

@component()
StaticMeshComponent: {
    asset: AssetProxy;
}

@component()
MaterialComponent: {
    asset: AssetProxy;
}

// clang-format on
#endif