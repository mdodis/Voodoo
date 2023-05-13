#include "Engine.h"

#include <flecs.h>

#include "AssetSystem.h"
#include "ECS/ECS.h"
#include "Memory/Extras.h"
#include "Renderer/Renderer.h"
#include "Window/Window.h"

static Engine* The_Engine;

void Engine::init()
{
    The_Engine = this;

    // Allocate subsystems
    input                       = alloc<Input>(allocator);
    window                      = win::create_window(allocator);
    window->input               = input;
    renderer                    = alloc<Renderer>(allocator);
    renderer->window            = window;
    renderer->input             = input;
    renderer->validation_layers = true;
    renderer->allocator         = allocator;
    ecs                         = alloc<ECS>(allocator);
    asset_system                = alloc<AssetSystem>(allocator);

    hooks.pre_init.broadcast(this);

    // Initialize window
    window->init(1600, 900).unwrap();

    // Initialize renderer
    renderer->init();

    // Initialize asset manager
    asset_system->init(allocator);

    // Initialize ECS
    ecs->init(renderer);

    hooks.post_init.broadcast(this);
}

void Engine::loop()
{
    window->poll();
    while (window->is_open) {
        // Update
        input->update();
        renderer->update();
        ecs->run();

        hooks.post_update.broadcast(this);

        // Draw
        hooks.pre_draw.broadcast(this);
        renderer->draw();

        // Poll
        window->poll();
    }
}

void Engine::deinit()
{
    renderer->deinit();
    asset_system->deinit();
}

Engine* Engine::instance() { return The_Engine; }
