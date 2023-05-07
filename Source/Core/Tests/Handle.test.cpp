#include "Handle.h"

#include "Containers/Array.h"
#include "FileSystem/Extras.h"
#include "Str.h"
#include "Test/Test.h"

struct SimpleResource {
    int data;
};

TEST_CASE("Core/Handle", "Handle Example")
{
    THandleSystem<Str, SimpleResource> handle_system;
    handle_system.init(System_Allocator);

    bool freed_100 = false, freed_200 = false;
    bool valid_100 = true, valid_200 = true;

    handle_system.on_release.bind_lambda([&](SimpleResource& resource) {
        if (resource.data == 100) {
            valid_100 = !freed_100;
            freed_100 = true;
        } else if (resource.data == 200) {
            valid_200 = !freed_200;
            freed_200 = true;
        }
    });

    {
        SimpleResource resource = {
            .data = 100,
        };
        handle_system.create_resource(LIT("100"), resource);
    }

    {
        SimpleResource resource = {
            .data = 200,
        };
        handle_system.create_resource(LIT("200"), resource);
    }

    {
        THandle<SimpleResource> handle = handle_system.get_handle(LIT("100"));

        SimpleResource& resource = handle_system.resolve(handle);

        REQUIRE(resource.data == 100, "");
    }

    {
        THandle<SimpleResource> handle = handle_system.get_handle(LIT("200"));

        SimpleResource& resource = handle_system.resolve(handle);

        REQUIRE(resource.data == 200, "");
    }

    handle_system.deinit();

    REQUIRE(freed_100 && freed_200, "");
    REQUIRE(valid_100 && valid_200, "");

    return MPASSED();
}