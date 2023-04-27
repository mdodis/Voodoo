#include "BlockList.h"

#include "Containers/Array.h"
#include "FileSystem/Extras.h"
#include "Test/Test.h"

struct Item {
    u32 member;
};

TEST_CASE("Engine/BlockList", "{4 add_range() calls work}")
{
    BlockListU32<Item, 2> list(System_Allocator);

    auto items = arr<Item>(Item{0}, Item{1}, Item{2});
    list.add_range((void*)items.elements, items.count());
    auto items2 = arr<Item>(Item{3}, Item{4}, Item{5});
    list.add_range((void*)items2.elements, items2.count());

    bool valid = true;
    list.for_each<Item>([&valid](u32 index, Item* item) {
        if (index != item->member) {
            valid = false;
        }
    });

    REQUIRE(valid, "");
    return MPASSED();
}