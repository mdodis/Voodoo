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

    return MPASSED();
}