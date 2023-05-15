#include "Doll.h"

static int doll_preprocess(Slice<Str> args) { return 0; }

DOLL_DECLARE_ACTION("preprocess", doll_preprocess);