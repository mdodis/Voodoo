#include "Base.h"
#include "App.h"
#include "FileSystem/Extras.h"
#include "Serialization/JSON.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/Extras.h"
#include "Reflection.h"

bool load_config(AppConfig &config) {
    CREATE_SCOPED_ARENA(&System_Allocator, init_arena, KILOBYTES(2));

    Str config_path = LIT("./app-config.json");

    FileHandle fh = open_file_read(config_path);
    if (!IS_VALID_FILE(fh)) {
        return false;
    }

    TBufferedFileTape<true> json_file(fh, 1024Ui64, init_arena);

    json_deserialize(
        &json_file, 
        init_arena, 
        descriptor_of(&config),
        (umm)&config);
    return true;
}

int main(int argc, char const *argv[]) {

    AppConfig app_config;
    load_config(app_config);

    print(LIT("Validation Layers: {}\n"), app_config.validation_layers);

    Window window;
    window.init(640, 480).unwrap();

    App app = {
        .window = window,
        .config = app_config,
    };
    DEFER(app.deinit());

    app.init_vulkan().unwrap();


    while (window.is_open) {
        window.poll();
    }
    
    print(LIT("Closing..."));
    return 0;
}
