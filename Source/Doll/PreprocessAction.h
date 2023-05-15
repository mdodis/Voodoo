#include "Base.h"
#include "Reflection.h"
#include "Str.h"
#include "StringFormat.h"
#include "Types.h"

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
