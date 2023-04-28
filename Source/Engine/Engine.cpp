#include "Engine.h"

#include "Memory/Extras.h"
#include "Renderer/Renderer.h"
#include "Window/Window.h"

void Engine::init()
{
    // Initialize input
    input = alloc<Input>(allocator);

    // Initialize window
    window        = win::create_window(allocator);
    window->input = input;
    window->init(1600, 900).unwrap();

    // Initialize renderer
    renderer                    = alloc<Renderer>(allocator);
    renderer->window            = window;
    renderer->input             = input;
    renderer->validation_layers = true;
    renderer->allocator         = allocator;
    renderer->init();
}

void Engine::loop()
{
    window->poll();
    while (window->is_open) {
        // Update input
        input->update();

        renderer->update();

        renderer->draw();
        window->poll();
    }
}

void Engine::deinit() { renderer->deinit(); }