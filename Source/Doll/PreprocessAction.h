#include "Base.h"
#include "Reflection.h"
#include "Str.h"
#include "StringFormat.h"
#include "Types.h"
#include "Variant.h"

namespace MetaTermAccess {
    enum Type : u32
    {
        Default,
        None,
        In,
        Out,
        InOut,
    };
}
typedef MetaTermAccess::Type EMetaTermAccess;

PROC_FMT_ENUM(MetaTermAccess, {
    FMT_ENUM_CASE2(MetaTermAccess, Default, "EcsInOutDefault");
    FMT_ENUM_CASE2(MetaTermAccess, None, "EcsInOutNone");
    FMT_ENUM_CASE2(MetaTermAccess, In, "EcsIn");
    FMT_ENUM_CASE2(MetaTermAccess, Out, "EcsOut");
    FMT_ENUM_CASE2(MetaTermAccess, InOut, "EcsInOut");
    FMT_ENUM_DEFAULT_CASE_UNREACHABLE();
})

PROC_PARSE_ENUM(MetaTermAccess, {
    PARSE_ENUM_CASE2(MetaTermAccess, Default, "default");
    PARSE_ENUM_CASE2(MetaTermAccess, None, "none");
    PARSE_ENUM_CASE2(MetaTermAccess, In, "in");
    PARSE_ENUM_CASE2(MetaTermAccess, Out, "out");
    PARSE_ENUM_CASE2(MetaTermAccess, InOut, "inout");
})

namespace MetaTermOperator {
    enum Type : u32
    {
        And,
        Or,
        Not,
        Optional,
        AndFrom,
        OrFrom,
        NotFrom,
    };
}
typedef MetaTermOperator::Type EMetaTermOperator;

PROC_FMT_ENUM(MetaTermOperator, {
    FMT_ENUM_CASE2(MetaTermOperator, And, "EcsAnd");
    FMT_ENUM_CASE2(MetaTermOperator, Or, "EcsOr");
    FMT_ENUM_CASE2(MetaTermOperator, Not, "EcsNot");
    FMT_ENUM_CASE2(MetaTermOperator, Optional, "EcsOptional");
    FMT_ENUM_CASE2(MetaTermOperator, AndFrom, "EcsAndFrom");
    FMT_ENUM_CASE2(MetaTermOperator, OrFrom, "EcsOrFrom");
    FMT_ENUM_CASE2(MetaTermOperator, NotFrom, "EcsNotFrom");
})

PROC_PARSE_ENUM(MetaTermOperator, {
    PARSE_ENUM_CASE2(MetaTermOperator, And, "and");
    PARSE_ENUM_CASE2(MetaTermOperator, Or, "or");
    PARSE_ENUM_CASE2(MetaTermOperator, Not, "not");
    PARSE_ENUM_CASE2(MetaTermOperator, Optional, "optional");
    PARSE_ENUM_CASE2(MetaTermOperator, AndFrom, "and_from");
    PARSE_ENUM_CASE2(MetaTermOperator, OrFrom, "or_from");
    PARSE_ENUM_CASE2(MetaTermOperator, NotFrom, "not_from");
})

struct MetaTermIDFlags {
    u32 f;

    static constexpr u32 Self       = 1u << 1;
    static constexpr u32 Up         = 1u << 2;
    static constexpr u32 Cascade    = 1u << 5;
    static constexpr u32 IsVariable = 1u << 7;
    static constexpr u32 IsEntity   = 1u << 8;
    static constexpr u32 IsName     = 1u << 9;
    static constexpr u32 Filter     = 1u << 10;

    MetaTermIDFlags(u32 f = 0) : f(f) {}

    void print_ors(WriteTape& out) const
    {
        // clang-format off
        format(&out, LIT("// ("));

        if (f & Self)     format(&out, LIT("EcsSelf,"));
        if (f & Up)       format(&out, LIT("EcsUp,"));
        if (f & Cascade)  format(&out, LIT("EcsCascade,"));
        if (f & IsEntity) format(&out, LIT("EcsIsEntity,"));
        if (f & IsName)   format(&out, LIT("EcsIsName,"));
        if (f & Filter)   format(&out, LIT("EcsFilter,"));
        format(&out, LIT(")"));
        // clang-format on
    }
};

struct MetaRelationship {
    Str name = LIT("IsA");

    bool is_default() const
    {
        return (name == LIT("IsA")) || (name == LIT("ChildOf"));
    }
};

struct MetaTermID {
    Str              entity = LIT("EcsThis");
    MetaRelationship relationship;
    MetaTermIDFlags  flags = MetaTermIDFlags::Self | MetaTermIDFlags::Up;
};

struct MetaSystemDescriptorTerm {
    Str               component_type;
    Str               component_id;
    EMetaTermAccess   access = MetaTermAccess::Default;
    EMetaTermOperator op     = MetaTermOperator::And;
    MetaTermID        source;

    bool is_pointer() const { return op == MetaTermOperator::Optional; }

    bool is_const() const
    {
        return (access == MetaTermAccess::In) ||
               (access == MetaTermAccess::None);
    }
};

struct MetaSystemDescriptor {
    Str                              name;
    Str                              phase;
    TArray<MetaSystemDescriptorTerm> terms;
};

namespace MetaComponentFlag {
    enum Type : u32
    {
        NoDefine = 1 << 0,
    };
}
typedef MetaComponentFlag::Type EMetaComponentFlag;
typedef u32                     MetaComponentFlags;

PROC_FMT_ENUM(MetaComponentFlag, {
    FMT_ENUM_CASE2(MetaComponentFlag, NoDefine, "NoDefine");
})

PROC_PARSE_ENUM(MetaComponentFlag, {
    PARSE_ENUM_CASE2(MetaComponentFlag, NoDefine, "nodefine");
})

struct MetaCompound {
    Str type;
};

struct MetaArray {
    Str type;
    Str count;
};

struct MetaPrimitive {
    Str type;
};

struct MetaType {
    TVariant<MetaPrimitive, MetaArray, MetaCompound> value;

    void format_desc_type(WriteTape& out) const
    {
        if (value.is<MetaPrimitive>()) {
            const MetaPrimitive& primitive = *value.get<MetaPrimitive>();
            format(&out, LIT("PrimitiveDescriptor<{}>"), primitive.type);
            return;
        }

        if (value.is<MetaCompound>()) {
            const MetaCompound& compound = *value.get<MetaCompound>();
            format(&out, LIT("{}_Descriptor"), compound.type);
            return;
        }

        if (value.is<MetaArray>()) {
            const MetaArray& array = *value.get<MetaArray>();
            format(
                &out,
                LIT("StaticArrayDescriptor<{}, {}>"),
                array.type,
                array.count);
            return;
        }
    }

    Str to_str() const
    {
        if (value.is<MetaPrimitive>()) {
            const MetaPrimitive& primitive = *value.get<MetaPrimitive>();
            return primitive.type;
        }

        if (value.is<MetaCompound>()) {
            const MetaCompound& compound = *value.get<MetaCompound>();
            return compound.type;
        }

        if (value.is<MetaArray>()) {
            const MetaArray& array = *value.get<MetaArray>();
            return array.type;
        }

        return Str::NullStr;
    }
};

struct MetaComponentProperty {
    Str      name;
    MetaType type;

    void format_desc_type(WriteTape& out) const { type.format_desc_type(out); }

    void format_desc_name(WriteTape& out) const
    {
        format(&out, LIT("{}_desc"), name);
    }
};

struct MetaComponentDescriptor {
    Str                           name;
    MetaComponentFlags            flags;
    TArray<MetaComponentProperty> properties;
    Str                           override_desc = Str::NullStr;

    void format_descriptor_name(WriteTape& out) const
    {
        format(&out, LIT("{}Descriptor"), name);
    }
};

static _inline bool is_meta_primitive(Str id)
{
    static Str match_table[] = {
        {LIT("bool")},
        {LIT("u8")},
        {LIT("i8")},
        {LIT("u16")},
        {LIT("i16")},
        {LIT("u32")},
        {LIT("i32")},
        {LIT("u64")},
        {LIT("i64")},
        {LIT("f32")},
        {LIT("f64")},
    };

    for (u32 i = 0; i < ARRAY_COUNT(match_table); ++i) {
        if (match_table[i] == id) return true;
    }

    return false;
}