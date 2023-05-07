#pragma once
#include "Base.h"
#include "Containers/Map.h"
#include "Delegates.h"

template <typename T>
struct THandle {
    u32 id;

    bool valid() const { return id != 0; }
};

template <typename T>
struct THandleAttachment {
    T   data;
    u32 id        = 0;
    u32 ref_count = 0;

    void reference() { ref_count++; }

    void dereference()
    {
        if (ref_count > 0) ref_count--;
    }

    bool operator==(const THandleAttachment& other) { return other.id == id; }
};

template <typename KeyType, typename ResourceType>
struct THandleSystem {
    using ReleaseProc = Delegate<void, ResourceType&>;

    TMap<KeyType, u32>                         handles;
    TMap<u32, THandleAttachment<ResourceType>> resources;
    ReleaseProc                                on_release;

    void init(Allocator& allocator)
    {
        handles.init(allocator);
        resources.init(allocator);
    }

    void deinit()
    {
        handles.release();

        for (TMapPair<u32, THandleAttachment<ResourceType>>& pair : resources) {
            on_release.call_safe(pair.val.data);
        }

        resources.release();
    }

    u32 create_resource(const KeyType& key, ResourceType& resource)
    {
        u32 id = _get_handle_id();

        handles.add(key, id);

        THandleAttachment<ResourceType> new_attachment = {
            .data      = resource,
            .id        = id,
            .ref_count = 0,
        };

        resources.add(id, new_attachment);

        return id;
    }

    ResourceType& resolve(THandle<ResourceType> handle)
    {
        THandleAttachment<ResourceType>& attachment = resources[handle.id];

        attachment.reference();

        return attachment.data;
    }

    void release(THandle<ResourceType> handle)
    {
        THandleAttachment<ResourceType>& attachment = resources[handle.id];
        attachment.dereference();
    }

    THandle<ResourceType> get_handle(const KeyType& key)
    {
        u32 id = handles[key];
        return THandle<ResourceType>{id};
    }

    u32 _get_handle_id() { return next_handle++; }

    u32 next_handle = 1;
};