#pragma once

struct Engine {
    struct win::Window* window;
    struct Renderer*    renderer;

    void init();
};