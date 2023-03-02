#include "Base.h"
#include "Engine.h"
#include "FileSystem/Extras.h"
#include "FileSystem/FileSystem.h"
#include "Reflection.h"
#include "Serialization/JSON.h"

struct {
    Input  input{System_Allocator};
    Window window = {
        .input = &input,
    };
} G;

int main(int argc, char const* argv[])
{
    int width  = 640;
    int height = 480;

    G.window.init(width, height).unwrap();

    Engine eng = {
        .window            = &G.window,
        .input             = &G.input,
        .validation_layers = true,
        .allocator         = System_Allocator,
    };
    eng.init();
    DEFER(eng.deinit());

    RenderObject monke = {
        .mesh      = eng.get_mesh(LIT("monke")),
        .material  = eng.get_material(LIT("default.mesh")),
        .transform = Mat4::identity(),
    };

    eng.render_objects.add(monke);
    for (int x = -20; x <= 20; ++x) {
        for (int y = -20; y <= 20; ++y) {
            Mat4 transform =
                scale(Vec3(0.2f)) * translation(Vec3{(float)x, -2.f, (float)y});

            RenderObject triangle = {
                .mesh      = eng.get_mesh(LIT("triangle")),
                .material  = eng.get_material(LIT("default.mesh")),
                .transform = transform,
            };

            eng.render_objects.add(triangle);
        }
    }

    G.window.poll();
    while (G.window.is_open) {
        G.input.update();

        eng.draw();
        G.window.poll();
    }

    print(LIT("Closing...\n"));
    return 0;
}
