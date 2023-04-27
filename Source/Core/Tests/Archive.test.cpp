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

struct Float3 {
    Float3()
    {
        values[0] = 0.0f;
        values[1] = 0.0f;
        values[2] = 0.0f;
    }

    Float3(float x, float y, float z)
    {
        values[0] = x;
        values[1] = y;
        values[2] = z;
    }

    float values[3];

    float& operator[](int index) { return values[index]; }
};

using Float3Descriptor = FixedArrayDescriptor<Float3, float, 3>;

struct ComplexObject {
    SimpleObject         another;
    TArray<SimpleObject> objects;
    Float3               f3;
};

struct ComplexObjectDescriptor : IDescriptor {
    SimpleObjectDescriptor another_desc = {
        OFFSET_OF(ComplexObject, another), LIT("another")};
    ArrayDescriptor<SimpleObject> objects_desc = {
        OFFSET_OF(ComplexObject, objects), LIT("objects")};
    Float3Descriptor f3_desc = {OFFSET_OF(ComplexObject, f3), LIT("f3")};

    IDescriptor* descs[3] = {
        &another_desc,
        &objects_desc,
        &f3_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(ComplexObject, descs)
};

DEFINE_DESCRIPTOR_OF_INL(ComplexObject)

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

TEST_CASE("Core/Archive", "archive serialize/deserialize complex")
{
    ComplexObject object = {
        .another =
            {
                .a = 10,
                .b = 20,
                .s = LIT("Hello, world!"),
            },
        .f3 = {1.0f, 2.0f, 3.0f},
    };
    object.objects.alloc = &System_Allocator;
    object.objects.add(SimpleObject{
        .a = 30,
        .b = 40,
        .s = LIT("one"),
    });
    object.objects.add(SimpleObject{
        .a = 50,
        .b = 60,
        .s = LIT("two"),
    });

    {
        auto t = open_write_tape("test.archive.complex");
        REQUIRE(
            serialize(&t, object, archive_serialize),
            "archive must be serialized");
    }

    ComplexObject deser_object;
    {
        auto t = open_read_tape("test.archive.complex");
        REQUIRE(
            deserialize(
                &t,
                System_Allocator,
                deser_object,
                archive_deserialize),
            "archive must be deserialized");
    }

    REQUIRE(deser_object.another.a == object.another.a, "a == a");
    REQUIRE(deser_object.another.b == object.another.b, "b == b");
    REQUIRE(deser_object.another.s == object.another.s, "s == s");
    REQUIRE(
        deser_object.objects.size == object.objects.size,
        "array sizes must match");

    for (u64 i = 0; i < deser_object.objects.size; ++i) {
        REQUIRE(
            deser_object.objects[i].a == object.objects[i].a,
            "[i]a == [i]a");
        REQUIRE(
            deser_object.objects[i].b == object.objects[i].b,
            "[i]b == [i]b");
        REQUIRE(
            deser_object.objects[i].s == object.objects[i].s,
            "[i]s == [i]s");
    }

    REQUIRE(deser_object.f3[0] == object.f3[0], "f3.x == f3.x");
    REQUIRE(deser_object.f3[1] == object.f3[1], "f3.y == f3.y");
    REQUIRE(deser_object.f3[2] == object.f3[2], "f3.z == f3.z");

    return MPASSED();
}