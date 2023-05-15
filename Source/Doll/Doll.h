#pragma once

#include "Containers/Map.h"
#include "Containers/Slice.h"
#include "Delegates.h"

struct Action {
    using DelegateType = Delegate<int, Slice<Str>>;

    Str          name;
    DelegateType run;
};

struct Doll {
    Doll() { actions.init(system_allocator); }
    static Doll& instance();

    void register_action(Str name, Action::DelegateType run_delegate);
    int  run(Str action, Slice<Str> args);
    void print_actions();

private:
    TMap<Str, Action> actions;
    SystemAllocator   system_allocator;
};

#define PROC_DOLL_ACTION_RUN(name) int name(Slice<Str> args)
typedef PROC_DOLL_ACTION_RUN(ProcDollActionRun);

struct StaticAction {
    StaticAction(Str name, ProcDollActionRun* proc)
    {
        Doll::instance()
            .register_action(name, Action::DelegateType::create_static(proc));
    }
};

#define DOLL_DECLARE_ACTION_3(x, y) x##y
#define DOLL_DECLARE_ACTION_2(x, y) DOLL_DECLARE_ACTION_3(x, y)
#define DOLL_DECLARE_ACTION_1(prefix) DOLL_DECLARE_ACTION_2(prefix, __COUNTER__)
#define DOLL_DECLARE_ACTION(namestr, fn) \
    static StaticAction DOLL_DECLARE_ACTION_1(Doll_Action_)(LIT(namestr), fn)
