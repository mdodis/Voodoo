#include "Archive.h"

#include "Test/Test.h"

struct SimpleObject {
    i32 a = 0;
    i32 b = 1;
    Str s = LIT("Hello");
};

struct SimpleObjectDescriptor : IDescriptor {
    PrimitiveDescriptor<i32> a_desc = {OFFSET_OF(SimpleObject, a), LIT("a")};
    PrimitiveDescriptor<i32> b_desc = {OFFSET_OF(SimpleObject, b), LIT("b")};
    StrDescriptor            s_desc = {OFFSET_OF(SimpleObject, s), LIT("s")};

    IDescriptor* descs[3] = {
        &a_desc,
        &b_desc,
        &s_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(SimpleObject, descs)
};

DEFINE_DESCRIPTOR_OF_INL(SimpleObject)

TEST_CASE("Core/Archive", "archive_serialize simple")
{
    SimpleObject object;

    auto t = open_write_tape("test.archive");
    REQUIRE(serialize(&t, object, archive_serialize), "");

    return MPASSED();
}

TEST_CASE("Core/Archive", "archive serialize/deserialize simple")
{
    SimpleObject object = {
        .a = 10,
        .b = 20,
        .s = LIT("Hello, world!"),
    };

    {
        auto t = open_write_tape("test.archive");
        REQUIRE(
            serialize(&t, object, archive_serialize),
            "archive must be serialized");
    }

    SimpleObject deser_object;
    {
        auto t = open_read_tape("test.archive");
        REQUIRE(
            deserialize(
                &t,
                System_Allocator,
                deser_object,
                archive_deserialize),
            "archive must be deserialized");
    }

    REQUIRE(object.a == deser_object.a, "a == a");
    REQUIRE(object.b == deser_object.b, "b == b");
    REQUIRE(object.s == deser_object.s, "s == s");

    return MPASSED();
}