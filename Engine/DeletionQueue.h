#pragma once
#include "Containers/Array.h"
#include "Delegates.h"

struct DeletionQueue {
    using DeletionDelegate = Delegate<void>;
    TArray<DeletionDelegate> deletors;

    DeletionQueue() {}
    DeletionQueue(IAllocator& allocator) : deletors(&allocator) {}

    void add(DeletionDelegate delegate) { deletors.add(delegate); }

    void flush()
    {
        for (u64 i = 0; i < deletors.size; ++i) {
            deletors[i].call_safe();
        }
        deletors.empty();
    }
};