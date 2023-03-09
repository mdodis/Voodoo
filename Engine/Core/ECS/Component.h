#pragma once
#include <typeinfo>

#include "Base.h"

namespace ComponentTypeProvider {
    enum Type : u16
    {
        Native = 0,
        Plugin = 1,
        Max    = 255,
    };
}
typedef ComponentTypeProvider::Type EComponentTypeProvider;

struct ComponentType {
    EComponentTypeProvider provider;
    u16                    size;
    size_t                 id;
};

struct ComponentReference {
    u32           id;
    ComponentType type;
};

static _inline bool operator==(
    const ComponentType& left, const ComponentType& right)
{
    return (left.provider == right.provider) && (left.id == right.id);
}

template <typename ComponentStorageType>
static inline ComponentType make_native_component_type()
{
    static_assert(sizeof(ComponentStorageType) < 65535);

    return ComponentType{
        .provider = ComponentTypeProvider::Native,
        .size     = (u16)sizeof(ComponentStorageType),
        .id       = typeid(ComponentStorageType).hash_code(),
    };
}

struct ComponentNativePtr {
    void*              ptr;
    ComponentReference ref;
};

template <typename T>
struct ComponentPtr {
    T*                 ptr;
    ComponentReference ref;

    ComponentPtr() {}
    ComponentPtr(const ComponentNativePtr& native_ptr)
    {
        ptr = (T*)native_ptr.ptr;
        ref = native_ptr.ref;
    }

    operator ComponentNativePtr() const
    {
        return ComponentNativePtr{
            .ptr = (void*)ptr,
            .ref = ref,
        };
    }

    T* operator->() { return ptr; }
};

struct ComponentMetadata {
    ComponentType type;
};