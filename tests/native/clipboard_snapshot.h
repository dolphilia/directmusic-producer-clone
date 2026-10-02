#pragma once
#include <windows.h>
#include <ole2.h>
#include <vector>
#include <cstring>

// The object returned by OleGetClipboard is a live view of the clipboard.
// Reinstalling that object and flushing it can ask the clipboard to read itself.
// Preserve supported formats as independent memory, without logging their data.
// Refuse the opt-in test before modifying a clipboard with unsupported handles.
class ClipboardSnapshot {
    struct Entry { UINT format; HGLOBAL data; };
    std::vector<Entry> entries_;
    HWND owner_ = nullptr;
    DWORD sequence_ = 0;
    static bool memory_format(UINT format) {
        return format != CF_BITMAP && format != CF_PALETTE &&
            format != CF_METAFILEPICT && format != CF_ENHMETAFILE &&
            format != CF_OWNERDISPLAY && format != CF_DSPBITMAP &&
            format != CF_DSPMETAFILEPICT && format != CF_DSPENHMETAFILE;
    }
    static HGLOBAL duplicate(HGLOBAL data) {
        const SIZE_T size = GlobalSize(data);
        if (!size) return nullptr;
        const void* source = GlobalLock(data);
        if (!source) return nullptr;
        HGLOBAL copy = GlobalAlloc(GMEM_MOVEABLE, size);
        void* destination = copy ? GlobalLock(copy) : nullptr;
        if (destination) { std::memcpy(destination, source, size); GlobalUnlock(copy); }
        else if (copy) { GlobalFree(copy); copy = nullptr; }
        GlobalUnlock(data);
        return copy;
    }
public:
    ClipboardSnapshot() = default;
    ClipboardSnapshot(const ClipboardSnapshot&) = delete;
    ClipboardSnapshot& operator=(const ClipboardSnapshot&) = delete;
    ~ClipboardSnapshot() {
        for (auto& entry : entries_) if (entry.data) GlobalFree(entry.data);
        if (owner_) DestroyWindow(owner_);
    }
    HRESULT capture() {
        owner_ = CreateWindowExW(0,L"STATIC",L"",WS_POPUP,0,0,0,0,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
        if (!owner_) return HRESULT_FROM_WIN32(GetLastError());
        if (!OpenClipboard(owner_)) return HRESULT_FROM_WIN32(GetLastError());
        HRESULT hr = S_OK;
        UINT format = 0;
        while (true) {
            SetLastError(ERROR_SUCCESS);
            format = EnumClipboardFormats(format);
            if (!format) { if (GetLastError()) hr = HRESULT_FROM_WIN32(GetLastError()); break; }
            if (!memory_format(format)) { hr = DV_E_TYMED; break; }
            HGLOBAL source = static_cast<HGLOBAL>(GetClipboardData(format));
            HGLOBAL copy = source ? duplicate(source) : nullptr;
            if (!copy) { hr = DV_E_FORMATETC; break; }
            entries_.push_back({format, copy});
        }
        sequence_ = GetClipboardSequenceNumber();
        CloseClipboard();
        return hr;
    }
    DWORD sequence() const { return sequence_; }
    HRESULT restore(DWORD expectedSequence) {
        if (!OpenClipboard(owner_)) return HRESULT_FROM_WIN32(GetLastError());
        HRESULT hr = S_OK;
        DWORD clipboardProcess = 0;
        GetWindowThreadProcessId(GetClipboardOwner(), &clipboardProcess);
        if (GetClipboardSequenceNumber() != expectedSequence || clipboardProcess != GetCurrentProcessId()) hr = S_FALSE;
        else if (!EmptyClipboard()) hr = HRESULT_FROM_WIN32(GetLastError());
        else for (auto& entry : entries_) {
            if (!SetClipboardData(entry.format, entry.data)) { hr = HRESULT_FROM_WIN32(GetLastError()); break; }
            entry.data = nullptr; // System now owns this independent handle.
        }
        CloseClipboard();
        return hr;
    }
};
