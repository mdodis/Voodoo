#pragma once

struct KVarBase {
    virtual Str name() = 0;
};

template <typename T>
struct KVar : KVarBase {
    KVar(Str name_str, T default_value)
        : data(default_value), name_str(name_str)
    {}

    T           data;
    Str         name_str;
    virtual Str name() override { return name_str; }
};

struct KVarSystem {
    TArray<KVarBase*> variables;
};