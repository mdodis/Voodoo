/**
 * Runs pre-process step for modules
 *
 * A folder named "Intermediate" will be created in the source code directory of
 * the module. As an example:
 *
 * - Root
 *     - Modules
 *         - MyModule
 *             - MyModule.h
 *             - MySystem.h
 *             - MySystem.cpp
 *             - ...
 *             - Intermediate <- This is the generated folder
 *                 - MySystem.generated.h
 *                 - GeneratedFiles.txt <- The files that were generated
 *                 - Module.generated.cpp
 *                 - Module.generated.h
 *
 */
#include "PreprocessAction.h"

#include "Arg.h"
#include "Doll.h"
#include "FileSystem/Extras.h"

struct PreprocessContext {
    Arena<ArenaMode::Dynamic>    arena;
    TArray<MetaSystemDescriptor> systems;
    TArray<Str>                  folder_context;
    TArray<Str>                  processed_files;
    TArray<Str>                  module_includes;

    void init()
    {
        arena = Arena<ArenaMode::Dynamic>(System_Allocator, MEGABYTES(1));
        arena.init();
        systems         = TArray<MetaSystemDescriptor>(&System_Allocator);
        folder_context  = TArray<Str>(&System_Allocator);
        processed_files = TArray<Str>(&System_Allocator);
        module_includes = TArray<Str>(&System_Allocator);
    }
};

static PreprocessContext G;

/** Push folder onto the stack */
static void push_folder_context(Str folder)
{
    G.folder_context.add(folder.clone(System_Allocator));
}

/** Pop folder from the stack */
static void pop_folder_context()
{
    ASSERT(G.folder_context.size > 0);

    Str* last = G.folder_context.last();
    System_Allocator.release((umm)last->data);
    G.folder_context.pop();
}

/** Concatenate all the folders in the stack into a single path */
static Str get_folder_context(Allocator& allocator)
{
    if (G.folder_context.size == 0) return Str::NullStr;

    Str current = G.folder_context[0].clone(allocator);
    for (u64 i = 1; i < G.folder_context.size; ++i) {
        current = format(allocator, LIT("{}/{}"), current, G.folder_context[i]);
        print(LIT("{} = {}\n"), i, G.folder_context[i]);
    }

    return current;
}

/**
 * Add the include.
 * This will generate a #include directive in the Module.cpp files
 */
static void add_module_include(Str filename)
{
    SAVE_ARENA(G.arena);
    Str folder = get_folder_context(G.arena);
    Str include;

    if (folder == Str::NullStr) {
        G.module_includes.add(filename.clone(System_Allocator));
    } else {
        Str include = format(G.arena, LIT("{}/{}"), folder, filename);
        G.module_includes.add(include.clone(System_Allocator));
    }
}

static int doll_preprocess(Slice<Str> args)
{
    G.init();
    ArgCollection arg_collection(G.arena);

    arg_collection.register_arg<Str>(
        LIT("path"),
        Str::NullStr,
        LIT("[REQUIRED] The path to the source code of the module that will be "
            "processed"));
    arg_collection.register_arg<Str>(
        LIT("name"),
        Str::NullStr,
        LIT("[OPTIONAL] The name of the module"));

    if (!arg_collection.parse_args(args)) {
        print(LIT("Invalid argument format. Printing summary...\n"));
        arg_collection.summary();
        return -1;
    }

    Str path_to_parse = *arg_collection.get_arg<Str>(LIT("path"));
    if (path_to_parse == Str::NullStr) {
        print(LIT("Argument '-path' must be specified.\n"));
        arg_collection.summary();
    }

    path_to_parse = to_absolute_path(path_to_parse, G.arena);
    print(LIT("Processing directory: {}\n"), path_to_parse);

    return 0;
}

DOLL_DECLARE_ACTION("preprocess", doll_preprocess);