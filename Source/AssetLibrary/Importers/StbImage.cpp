#include "DefaultImporters.h"
#define STB_IMAGE_IMPLEMENTATION
#include "Containers/Extras.h"
#include "Memory/Extras.h"
#include "stb_image.h"

static thread_local Allocator* Local_Allocator;
static Str File_Extensions[] = {".png", ".jpeg", ".bmp", ".jpg"};

PROC_IMPORTER_IMPORT(stb_image_import)
{
    Local_Allocator = &allocator;
    CREATE_SCOPED_ARENA(*Local_Allocator, temp, 1024);

    Str   path_cstr = format(temp, LIT("{}\0"), path);
    int   width, height, channels;
    void* data = stbi_load(path_cstr.data, &width, &height, &channels, 4);
    if (!data)
        return Err(
            LIT("Could not load image. Either bad path, invalid or unsupported "
                "format"));

    size_t image_size = width * height * 4;

    Slice<u8> blob = alloc_slice<u8>(allocator, image_size);

    memcpy(blob.ptr, data, image_size);

    return Ok(Asset{
        .info =
            {
                .version     = 1,
                .kind        = AssetKind::Texture,
                .compression = AssetCompression::None,
                .actual_size = image_size,
                .texture =
                    {
                        .width  = (u32)width,
                        .height = (u32)height,
                        .depth  = 1,
                        .format = TextureFormat::R8G8B8A8UInt,
                    },
            },
        .blob = blob,
    });
}

Importer Stb_Image_Importer = {
    .name = LIT("stb_image"),
    .kind = AssetKind::Texture,
    .file_extensions =
        Slice<Str>(File_Extensions, ARRAY_COUNT(File_Extensions)),
    .import = stb_image_import,
};