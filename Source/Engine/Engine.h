#pragma once
#include <flecs.h>

#include "Base.h"
#include "Memory/Base.h"

namespace win {
    struct Window;
}

struct Engine {
    /**
     * The main allocator that's used to allocate all engine systems. Change at
     * your own risk!
     */
    IAllocator& allocator = System_Allocator;

    struct win::Window* window;
    struct Renderer*    renderer;
    struct Input*       input;
    struct ECS*         ecs;

    void init();
    void loop();
    void deinit();
};