#pragma once
#include "Base.h"
#include <typeinfo>

struct ComponentReference {
    u32 id;
};

namespace ComponentTypeProvider {
    enum Type : u32 {
    Native = 0,
    Plugin = 1,
    Max = 255;
    };
}
typedef ComponentTypeProvider::Type EComponentTypeProvider;

struct ComponentType {
    EComponentTypeProvider provider;
    size_t id;
};

struct ComponentMetadata {
    ComponentType type;
    size_t size;
};