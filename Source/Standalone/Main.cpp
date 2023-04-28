#pragma once
#include "Engine/Engine.h"

static struct {
    Engine engine;
} G;

int main(int argc, char* argv[])
{
    G.engine.init();
    G.engine.loop();
    G.engine.deinit();
    return 0;
}