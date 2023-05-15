#include "Doll.h"

static int doll_help(Slice<Str> args)
{
    Doll::instance().print_actions();
    return 0;
}

DOLL_DECLARE_ACTION("help", doll_help);