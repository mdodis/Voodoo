#include "Doll.h"

#include "Arg.h"

static Doll* The_Doll = nullptr;

Doll& Doll::instance()
{
    if (The_Doll == nullptr) {
        The_Doll = new Doll();
    }

    return *The_Doll;
}

void Doll::register_action(Str name, Action::DelegateType run_delegate)
{
    actions.add(
        name,
        Action{
            .name = name,
            .run  = run_delegate,
        });
}

int Doll::run(Str action, Slice<Str> args)
{
    if (actions.contains(action)) {
        return actions[action].run.call(args);
    } else {
        print(LIT("Action {} does not exist.\n"), action);
        return actions[LIT("help")].run.call(args);
    }
}

void Doll::print_actions()
{
    print(LIT("ACTIONS\n"));
    for (auto pair : actions) {
        print(LIT("- {}\n"), pair.val.name);
    }
}

int main(int argc, char* argv[])
{
    // Parse arguments and determine what action to run
    TArray<Str> args(&System_Allocator);

    for (int i = 0; i < argc; ++i) {
        args.add(Str(argv[i]));
    }

    if (args.size <= 1) {
        print(LIT("No action specified. Exiting...\n"));
        return -1;
    }

    return The_Doll->run(args[1], slice(args, 2));
}