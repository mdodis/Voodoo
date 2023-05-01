#define MOK_WIN32_NO_FUNCTIONS

#include "Win32DnD.h"

#include <oleidl.h>

#include "Compat/Win32Internal.inc"
#include "Debugging/Assertions.h"
#include "Memory/Arena.h"

static bool         OLE_Initialized = false;
static _inline void check_initialize_ole()
{
    if (!OLE_Initialized) {
        ASSERT(OleInitialize(0) == S_OK);
    }
}

struct Win32DropTargetProxy : IDropTarget {
    Win32DropTargetProxy(Win32::IDnD* iface, Win32::HWND hwnd)
        : iface(iface), hwnd(hwnd)
    {}

    void init() { RegisterDragDrop((HWND)hwnd, this); }

    void deinit() { RevokeDragDrop((HWND)hwnd); }

    // Inherited via IDropTarget
    virtual HRESULT __stdcall QueryInterface(
        REFIID riid, void** ppvObject) override
    {
        if (riid == IID_IDropTarget) {
            *ppvObject = this;
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    virtual ULONG __stdcall AddRef(void) override { return 1; }
    virtual ULONG __stdcall Release(void) override { return 0; }

    virtual HRESULT __stdcall DragEnter(
        IDataObject* pDataObj,
        DWORD        grfKeyState,
        POINTL       pt,
        DWORD*       pdwEffect) override
    {
        Win32::DataObject data_object = make_data_object(pDataObj);

        if (!data_object.valid()) {
            return E_NOTIMPL;
        }

        *pdwEffect &= DROPEFFECT_COPY;
        return iface->drag_enter(data_object, grfKeyState);
    }

    virtual HRESULT __stdcall DragOver(
        DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
    {
        *pdwEffect &= DROPEFFECT_COPY;
        return iface->drag_over(grfKeyState, Win32::POINT{pt.x, pt.y});
    }

    virtual HRESULT __stdcall DragLeave(void) override
    {
        iface->drag_leave();
        return S_OK;
    }

    virtual HRESULT __stdcall Drop(
        IDataObject* pDataObj,
        DWORD        grfKeyState,
        POINTL       pt,
        DWORD*       pdwEffect) override
    {
        FORMATETC fmc = {
            .cfFormat = CF_HDROP,
            .ptd      = NULL,
            .dwAspect = DVASPECT_CONTENT,
            .lindex   = -1,
            .tymed    = TYMED_HGLOBAL,
        };
        STGMEDIUM stgm;

        if (SUCCEEDED(pDataObj->GetData(&fmc, &stgm))) {
            iface->drag_drop_begin();
            CREATE_SCOPED_ARENA(System_Allocator, temp, KILOBYTES(2));
            HDROP hdrop = (HDROP)stgm.hGlobal;

            UINT file_count = DragQueryFileW(hdrop, 0xFFFFFFFF, NULL, 0);
            for (UINT i = 0; i < file_count; ++i) {
                SAVE_ARENA(temp);

                UINT   size = DragQueryFileW(hdrop, i, NULL, 0) + 1;
                LPWSTR wstr =
                    (LPWSTR)System_Allocator.reserve(size * sizeof(wchar_t));

                UINT copied = DragQueryFileW(hdrop, i, wstr, size);
                ASSERT(copied <= size);

                Str path = wstr_to_multibyte(wstr, copied, temp);
                iface->drag_drop(path);
            }

            ReleaseStgMedium(&stgm);
            iface->drag_drop_finish();
        }

        return S_OK;
    }

    Win32::DataObject make_data_object(IDataObject* pDataObj)
    {
        FORMATETC fmc = {
            .cfFormat = CF_HDROP,
            .ptd      = NULL,
            .dwAspect = DVASPECT_CONTENT,
            .lindex   = -1,
            .tymed    = TYMED_HGLOBAL,
        };

        if (!SUCCEEDED(pDataObj->QueryGetData(&fmc))) {
            // If query data failed just return invalid object
            return Win32::DataObject::invalid();
        }

        return Win32::DataObject{
            .type = Win32::DataObjectType::File,
        };
    }

    Win32::IDnD* iface;
    Win32::HWND  hwnd;
};

namespace Win32 {
    DnDHandle register_dnd(HWND hwnd, IDnD* dnd)
    {
        ::check_initialize_ole();
        Win32DropTargetProxy* proxy = new Win32DropTargetProxy(dnd, hwnd);
        proxy->init();

        return (DnDHandle)proxy;
    }

    void revoke_dnd(DnDHandle handle)
    {
        Win32DropTargetProxy* proxy = (Win32DropTargetProxy*)handle;
        proxy->deinit();

        delete proxy;
    }
}  // namespace Win32
