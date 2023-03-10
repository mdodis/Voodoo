#pragma once
#include "Component.h"
#include "Containers/Array.h"
#include "Traits.h"

struct ArchetypeReference {
    u32 index;

    static constexpr ArchetypeReference invalid() {
        return ArchetypeReference{NumProps<u32>::max};
    }
};

struct Archetype {
    static constexpr int Max_Components = 10;

    TInlineArray<ComponentType, Max_Components> types;

    inline bool has_component(const ComponentType& type) const
    {
        for (const auto& t : types) {
            if (type == t) {
                return true;
            }
        }
        return false;
    }
};

static _inline bool operator==(const Archetype& left, const Archetype& right) {
    for (const auto& t : left.types) {
        if (!right.has_component(t))
            return false;
    }
    return true;
}