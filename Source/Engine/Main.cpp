#include "Main.h"

#include "Engine.h"
#include "Input.h"
#include "Window/Window.h"

static struct {
    const int default_width  = 1600;
    const int default_height = 900;

    Engine engine;
    Input  input;
} G;

int standalone_main(int argc, char* argv[])
{
    // Initialize input
    G.input = Input(System_Allocator);

    // Initialize window
    G.engine.window        = win::create_window(System_Allocator);
    G.engine.window->input = &G.input;
    G.engine.window->init(G.default_width, G.default_height).unwrap();
}
