#include "Engine.h"

#include "Memory/Extras.h"
#include "Window/Window.h"

void Engine::init()
{
    // Initialize input
    input = alloc<Input>(allocator);

    // Initialize window
    window        = win::create_window(allocator);
    window->input = input;
    window->init(1600, 900).unwrap();
}

void Engine::loop()
{
    window->poll();
    while (window->is_open) {
        // Update input
        input->update();

        window->poll();
    }
}