#pragma once
#include <functional>
#include "Base.h"
#include "Debugging/Assertions.h"
#include "IntrusiveList.h"
#include "Memory/Base.h"
#include "Memory/Extras.h"

template <typename SizeType, u32 BlockSize = 64>
struct BlockList {
    BlockList(IAllocator& allocator, SizeType item_size)
        : allocator(allocator)
        , item_size(item_size)
        , block_total_size(BlockSize * item_size)
        , num_blocks(0)
        , num_items(0)
    {
        intrusive_list::init(&first_node);
    }

#pragma pack(push, 1)
    struct Block {
        Block() : count(0) { intrusive_list::init(&node); }
        SizeType             count;
        intrusive_list::Node node;
        u8 items[1];  // Array size depends on block_total_size
    };
#pragma pack(pop)

    void add(void* item) { add_range(item, 1); }

    void add_range(void* items_, SizeType count)
    {
        u8* items = (u8*)items_;

        intrusive_list::Node* last_block = first_node.prev;

        if (capacity() < count) {
            bool items_fit_in_last_block = false;

            // Do the items fit in the last block?
            if (!intrusive_list::is_empty(&first_node)) {
                Block* last_block = CONTAINER_OF(first_node.prev, Block, node);
                if (last_block->count + count <= BlockSize) {
                    items_fit_in_last_block = true;
                }
            }

            if (!items_fit_in_last_block) {
                const SizeType blocks_to_add = (count / BlockSize) + 1;
                last_block                   = add_blocks(blocks_to_add);
            }
        }

        SizeType items_copied = 0;
        while (items_copied != count) {
            intrusive_list::Node* list_node = last_block->next;

            const SizeType max_items_to_copy = count - items_copied;

            Block* block = CONTAINER_OF(list_node, Block, node);

            SizeType items_to_copy = max_items_to_copy;
            if (max_items_to_copy + block->count > BlockSize) {
                items_to_copy = BlockSize - block->count;
            }

            if (items_to_copy != 0) {
                memcpy(
                    block->items + (block->count * item_size),
                    items + (items_copied * item_size),
                    items_to_copy * item_size);
                items_copied += items_to_copy;
                block->count += items_to_copy;
            }

            last_block = last_block->next;
        }
    }

    intrusive_list::Node* add_blocks(SizeType count)
    {
        intrusive_list::Node* last_block       = first_node.prev;
        intrusive_list::Node* first_last_block = last_block;

        while (count != 0) {
            count--;

            Block* block = allocate_block();
            ASSERT(block);

            intrusive_list::append(&block->node, last_block);
            num_blocks++;
        }

        return first_last_block;
    }

    Block* allocate_block()
    {
        void* new_block_ptr =
            allocator.reserve(sizeof(Block) + (block_total_size - 1));
        ASSERT(new_block_ptr);

        Block* new_block = new (new_block_ptr) Block();
        return new_block;
    }

    template <typename StorageType>
    void for_each(std::function<void(SizeType, StorageType*)> fn)
    {
        intrusive_list::Node* it    = first_node.next;
        SizeType              index = 0;

        while (it != &first_node) {
            Block* block = CONTAINER_OF(it, Block, node);

            for (SizeType i = 0; i < block->count; ++i) {
                fn(index, (StorageType*)(block->items + i * item_size));

                index++;
            }

            it = it->next;
        }
    }

    SizeType capacity() const { return num_blocks * BlockSize; }

    intrusive_list::Node first_node;
    SizeType             num_items;
    SizeType             num_blocks;
    SizeType             item_size;
    SizeType             block_total_size;
    IAllocator&          allocator;
};

template <typename StorageType, typename SizeType, u32 BlockSize = 64>
struct TBlockList : public BlockList<SizeType, BlockSize> {
    using Base = BlockList<SizeType, BlockSize>;

    TBlockList() = delete;
    TBlockList(IAllocator& allocator)
        : Base(allocator, (SizeType)sizeof(StorageType))
    {}
};

template <typename StorageType, u32 BlockSize = 64>
using BlockListU32 = TBlockList<StorageType, u32, BlockSize>;