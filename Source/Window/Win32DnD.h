#pragma once
#include "Compat/Win32Base.h"
#include "Str.h"

namespace Win32 {

    namespace DataObjectType {
        enum Type : int
        {
            Unsupported = 0,
            File
        };
    }
    typedef DataObjectType::Type EDataObjectType;

    struct DataObject {
        EDataObjectType type;

        static constexpr DataObject invalid()
        {
            return DataObject{
                .type = DataObjectType::Unsupported,
            };
        }

        inline bool valid() { return type != DataObjectType::Unsupported; }
    };

    struct IDnD {
        virtual Win32::HRESULT drag_enter(
            DataObject& data_object, Win32::DWORD key_state) = 0;
        virtual void           drag_leave()                  = 0;
        virtual Win32::HRESULT drag_over(
            Win32::DWORD key_state, Win32::POINT point) = 0;
        virtual void           drag_drop_begin()        = 0;
        virtual Win32::HRESULT drag_drop(Str path)      = 0;
        virtual void           drag_drop_finish()       = 0;
    };

    typedef void* DnDHandle;

    DnDHandle register_dnd(Win32::HWND hwnd, IDnD* dnd);
    void      revoke_dnd(DnDHandle handle);

}  // namespace Win32