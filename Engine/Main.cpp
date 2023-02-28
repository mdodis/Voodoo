#include "Base.h"
#include "Engine.h"
#include "FileSystem/Extras.h"
#include "FileSystem/FileSystem.h"
#include "Reflection.h"
#include "Serialization/JSON.h"

int main(int argc, char const* argv[])
{
    int width  = 640;
    int height = 480;

    Window window;
    window.init(width, height).unwrap();

    Engine eng = {
        .window            = &window,
        .validation_layers = true,
        .allocator         = System_Allocator,
    };
    eng.init();
    DEFER(eng.deinit());

    window.poll();
    while (window.is_open) {
        eng.draw();
        window.poll();
    }

    print(LIT("Closing...\n"));
    return 0;
}
