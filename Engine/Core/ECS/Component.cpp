#include "Component.h"

void* ComponentIteratorBase::next()
{
    if (current < count) {
        void* data_ptr = ptr.to_data_ptr();

        if (ptr.index < ComponentContainer::List::ItemsPerBlock) {
            ptr.index++;
        } else {
            ptr.block = CONTAINER_OF(
                ptr.block->node.next,
                ComponentContainer::List::Block,
                node);
            ptr.index = 0;
        }
        current++;

        return data_ptr;
    } else {
        return 0;
    }
}