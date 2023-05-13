#pragma once

#include "Base.h"
#include "Delegates.h"
#include "Memory/Base.h"
#include "MulticastDelegate.h"

namespace win {
    struct Window;
}

struct Engine {
    /**
     * The main allocator that's used to allocate all engine systems. Change at
     * your own risk!
     */
    Allocator& allocator = System_Allocator;

    struct win::Window* window;
    struct Renderer*    renderer;
    struct Input*       input;
    struct ECS*         ecs;
    struct AssetSystem* asset_system;

    using Hook = MulticastDelegate<Engine*>;

    struct {
        Hook pre_init;
        Hook post_init;
        Hook post_update;
        Hook pre_draw;
    } hooks;

    void init();
    void loop();
    void deinit();

    static Engine* instance();
};