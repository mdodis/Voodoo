#pragma once
#include <typeinfo>

#include "Base.h"
#include "BlockList.h"
#include "Hashing.h"

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

static _inline bool operator<(
    const ComponentType& left, const ComponentType& right)
{
    return (left.provider + left.id) < (right.provider + right.id);
}

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

static _inline u64 hash_of(const ComponentType& type, uint32 seed)
{
    u64   hash   = type.provider + type.id;
    char* as_str = (char*)&hash;
    return murmur_hash2(as_str, 8, seed);
}

struct ComponentReference {
    u32           id;
    ComponentType type;
};

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

struct ComponentContainer {
    using List = BlockList<u32, 64>;
    ComponentContainer(
        IAllocator& allocator, const ComponentType& component_type)
        : component_type(component_type), list(allocator, component_type.size)
    {}

    ComponentType component_type;
    List          list;
};

struct ComponentIteratorBase {
    ComponentIteratorBase(
        const ComponentContainer& container, u32 begin_index, u32 count)
        : ComponentIteratorBase(
              container.list.ptr_from_index(begin_index),
              count,
              container.component_type)
    {}

    ComponentIteratorBase(
        ComponentContainer::List::Ptr begin,
        u32                           count,
        const ComponentType&          type)
        : ptr(begin), count(count), type(type), current(0)
    {}
    void* next();

    ComponentContainer::List::Ptr ptr;
    ComponentType                 type;
    u32                           current;
    u32                           count;
};