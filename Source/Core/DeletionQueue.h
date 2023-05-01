#pragma once
#include "Containers/Array.h"
#include "Delegates.h"

struct DeletionQueue {
    using DeletionDelegate = Delegate<void>;
    TArray<DeletionDelegate> deletors;

    DeletionQueue() {}
    DeletionQueue(Allocator& allocator) : deletors(&allocator) {}

    void add(DeletionDelegate&& delegate) { deletors.add(std::move(delegate)); }

    FORWARD_DELEGATE_LAMBDA_TEMPLATE()
    void add_lambda(FORWARD_DELEGATE_LAMBDA_SIG(DeletionDelegate))
    {
        DeletionDelegate delegate =
            FORWARD_DELEGATE_LAMBDA_CREATE(DeletionDelegate);
        add(std::move(delegate));
    }

    void flush()
    {
        for (u64 i = 0; i < deletors.size; ++i) {
            deletors[i].call_safe();
        }
        deletors.empty();
    }
};