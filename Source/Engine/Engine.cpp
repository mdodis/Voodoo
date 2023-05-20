#include "Engine.h"

#include <flecs.h>

#include "AssetSystem.h"
#include "ECS/ECS.h"
#include "Memory/Extras.h"
#include "ModuleSystem.h"
#include "Renderer/Renderer.h"
#include "SubsystemManager.h"
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
    subsystems                  = alloc<SubsystemManager>(allocator);

    asset_system  = alloc<AssetSystem>(allocator);
    module_system = alloc<ModuleSystem>(allocator);

    hooks.pre_init.broadcast(this);

    // Initialize window
    window->init(1600, 900).unwrap();

    // Initialize renderer
    renderer->init();

    subsystems->init();

    // Initialize asset manager
    asset_system->init(allocator);

    // Initialize ECS
    ecs->init(renderer);

    module_system->init();

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
    subsystems->deinit();
}

Engine* Engine::instance() { return The_Engine; }
void Engine::set_instance(Engine* new_instance) { The_Engine = new_instance; }
