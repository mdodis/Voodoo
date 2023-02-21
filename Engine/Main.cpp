#include "Base.h"
#include "App.h"
#include "FileSystem/Extras.h"

int main(int argc, char const *argv[]) {

    Window window;
    window.init(640, 480).unwrap();

    App app = {
        .window = window,
    };

    app.init_vulkan().unwrap();


    while (window.is_open) {
        window.poll();
    }
    
    return 0;
}
