#include "Engine.h"

#include "ECS/ECS.h"
#include "Memory/Extras.h"
#include "Renderer/Renderer.h"
#include "Window/Window.h"

void Engine::init()
{
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

    hooks.pre_init.broadcast(this);

    // Initialize window
    window->init(1600, 900).unwrap();

    // Initialize renderer
    renderer->init();

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

void Engine::deinit() { renderer->deinit(); }