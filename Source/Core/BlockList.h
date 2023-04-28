#pragma once
#include <functional>

#include "Base.h"
#include "Containers/IntrusiveList.h"
#include "Debugging/Assertions.h"
#include "Memory/Base.h"
#include "Memory/Extras.h"

template <typename SizeType, u32 BlockSize = 64>
struct BlockList {
    static constexpr u32 ItemsPerBlock = BlockSize;

    BlockList(IAllocator& allocator, SizeType item_size)
        : allocator(allocator)
        , item_size(item_size)
        , block_total_size(BlockSize * item_size)
        , num_blocks(0)
        , num_items(0)
    {
        intrusive_list::init(&first_node);
    }

    BlockList& operator=(const BlockList& other)
    {
        first_node       = other.first_node;
        num_items        = other.num_items;
        num_blocks       = other.num_blocks;
        item_size        = other.item_size;
        block_total_size = other.block_total_size;
        allocator        = other.allocator;
        return *this;
    }

#pragma pack(push, 1)
    struct Block {
        Block() : count(0) { intrusive_list::init(&node); }
        SizeType             count;
        intrusive_list::Node node;
        u8 items[1];  // Array size depends on block_total_size
    };
#pragma pack(pop)

    struct Ptr {
        Block*   block;
        SizeType index;
        SizeType item_size;
        void*    to_data_ptr() const
        {
            return (void*)(((u8*)block->items) + (index * item_size));
        }
    };

    Ptr ptr_from_index(SizeType index) const
    {
        SizeType block_num    = (index) / BlockSize;
        SizeType block_offset = (index) % BlockSize;

        return Ptr{
            .block     = get_nth_block(block_num),
            .index     = block_offset,
            .item_size = item_size,
        };
    }

    Block* get_nth_block(SizeType n) const
    {
        const intrusive_list::Node* it = &first_node;
        while (n != 0) {
            it = it->next;
            n--;
        }

        ASSERT(it != &first_node);
        return CONTAINER_OF(it, Block, node);
    }

    Ptr allocate(SizeType count = 1)
    {
        num_items += count;
        intrusive_list::Node* last_block = &first_node;

        // If count doesn't fit in last block, then add blocks
        if ((num_items + count) > capacity()) {
            const SizeType blocks_to_add = (count / BlockSize) + 1;
            last_block                   = add_blocks(blocks_to_add);
        }
        last_block = last_block->next;

        // Check if the starting block can fit anything, otherwise start from
        // next
        Block* start = CONTAINER_OF(last_block, Block, node);
        if (start->count == BlockSize) {
            last_block = last_block->next;
            start      = CONTAINER_OF(last_block, Block, node);
        }

        Ptr result = {
            .block = CONTAINER_OF(last_block, Block, node),
            .index = start->count,
        };

        // Walk through the added blocks and change their count
        while (last_block != &first_node) {
            Block* block = CONTAINER_OF(last_block, Block, node);

            const SizeType item_capacity = BlockSize - block->count;
            const SizeType num_items_reserved =
                (count > item_capacity) ? item_capacity : count;

            block->count += num_items_reserved;
            count -= num_items_reserved;
            last_block = last_block->next;
        }

        return result;
    }

    void add(void* item) { add_range(item, 1); }

    void add_range(void* items_, SizeType count)
    {
        u8* items = (u8*)items_;

        Ptr start = allocate(count);

        intrusive_list::Node* node              = &start.block->node;
        Block*                block             = start.block;
        SizeType              write_start_index = start.index;

        SizeType items_copied = 0;
        while (items_copied != count) {
            const SizeType max_items_to_copy   = count - items_copied;
            const SizeType num_available_items = BlockSize - write_start_index;

            const SizeType items_to_copy =
                num_available_items > max_items_to_copy
                    ? max_items_to_copy
                    : num_available_items;

            memcpy(
                block->items + write_start_index,
                items + (items_copied * item_size),
                items_to_copy * item_size);
            items_copied += items_to_copy;

            node              = node->next;
            block             = CONTAINER_OF(node, Block, node);
            write_start_index = 0;
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
            last_block = &block->node;
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