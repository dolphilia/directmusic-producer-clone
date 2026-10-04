// Reference lifecycle probe. Does not register/unregister COM servers.
#include <windows.h>
#include <objbase.h>
#include <objidl.h>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include "compat/producer_ids.h"
#include "tempo_host_fixture.h"
#include "runtime_tempo.h"
#include "reference_timeline.h"
#include "compat/strip_manager.h"
#include "compat/timeline_services.h"
#include "reference_tempo_edit.h"
#include "reference_time_signature.h"
#include "drawing_surface.h"
#include "property_page_probe.h"
#ifdef PRODUCER_WINDOWED_PROBE
#include "hidden_ole_site.h"
#include "drag_probe.h"
#include "ole_drag_boundary.h"
#include "drop_menu_boundary.h"
#include "command_probe.h"
#include "clipboard_snapshot.h"
#endif

static HMODULE observedModule = nullptr;
static bool testSystemClipboard = false;
static HMODULE clipboardTimelineModule = nullptr;

static LONG CALLBACK observe_exception(EXCEPTION_POINTERS* exception) {
    if (exception->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) return EXCEPTION_CONTINUE_SEARCH;
    MEMORY_BASIC_INFORMATION region{};
    const auto instruction = reinterpret_cast<std::uintptr_t>(exception->ExceptionRecord->ExceptionAddress);
    VirtualQuery(exception->ExceptionRecord->ExceptionAddress, &region, sizeof(region));
    std::printf("{\"operation\":\"access_violation\",\"in_target_module\":%s,\"instruction_rva\":\"0x%lx\",\"access_kind\":%llu,\"access_address\":\"0x%llx\",\"eax\":\"0x%lx\",\"ecx\":\"0x%lx\",\"edx\":\"0x%lx\"}\n",
        observedModule && region.AllocationBase == observedModule ? "true" : "false",
        static_cast<unsigned long>(instruction - reinterpret_cast<std::uintptr_t>(region.AllocationBase)),
        static_cast<unsigned long long>(exception->ExceptionRecord->ExceptionInformation[0]),
        static_cast<unsigned long long>(exception->ExceptionRecord->ExceptionInformation[1]),
        exception->ContextRecord->Eax, exception->ContextRecord->Ecx, exception->ContextRecord->Edx);
    std::fflush(stdout);
    return EXCEPTION_CONTINUE_SEARCH; // Preserve the failing process exit status.
}

static void result(const char* operation, HRESULT hr) {
    std::printf("{\"operation\":\"%s\",\"hresult\":\"0x%08lx\"}\n", operation, static_cast<unsigned long>(hr));
    std::fflush(stdout);
}

static void guid_result(const char* operation, const GUID& g) {
    std::printf("{\"operation\":\"%s\",\"guid\":\"%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\"}\n",
        operation, g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
        g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
    std::fflush(stdout);
}

static bool probe_interface(IUnknown* object, REFIID iid, const char* name, HMODULE module) {
    IUnknown* queried = nullptr;
    HRESULT hr = object->QueryInterface(iid, reinterpret_cast<void**>(&queried));
    result(name, hr);
    if (FAILED(hr) || !queried) return false;
    std::printf("{\"operation\":\"%s_layout\",\"subobject_offset\":%lld,\"vtable_rva\":\"0x%lx\"}\n", name,
        static_cast<long long>(reinterpret_cast<std::intptr_t>(queried) - reinterpret_cast<std::intptr_t>(object)),
        static_cast<unsigned long>(*reinterpret_cast<std::uintptr_t*>(queried) - reinterpret_cast<std::uintptr_t>(module)));
    IUnknown* identity = nullptr;
    hr = queried->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&identity));
    const bool same = SUCCEEDED(hr) && identity == object;
    std::printf("{\"operation\":\"%s_identity\",\"same\":%s}\n", name, same ? "true" : "false");
    if (identity) identity->Release();
    queried->Release(); std::fflush(stdout);
    return same;
}

static bool saved_bytes(IPersistStream* persist, std::vector<unsigned char>& bytes) {
    IStream* stream = nullptr;
    HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &stream);
    if (FAILED(hr)) return false;
    hr = persist->Save(stream, FALSE);
    STATSTG stat{};
    if (SUCCEEDED(hr)) hr = stream->Stat(&stat, STATFLAG_NONAME);
    bool ok = SUCCEEDED(hr) && stat.cbSize.QuadPart <= 1024 * 1024;
    if (ok) {
        LARGE_INTEGER zero{};
        hr = stream->Seek(zero, STREAM_SEEK_SET, nullptr);
        bytes.resize(static_cast<size_t>(stat.cbSize.QuadPart));
        ULONG read = 0;
        if (SUCCEEDED(hr)) hr = stream->Read(bytes.data(), static_cast<ULONG>(bytes.size()), &read);
        ok = SUCCEEDED(hr) && read == bytes.size();
    }
    result("snapshot_saved_bytes", hr);
    stream->Release(); return ok;
}

static bool check_runtime_tempo(IUnknown* runtime, double expected, LONG at = 0) {
    runtime_tempo::TempoParam value{};
    LONG next = 0;
    const HRESULT hr = runtime_tempo::get(runtime, at, &next, &value);
    result("get_runtime_tempo", hr);
    const bool ok = SUCCEEDED(hr) && value.tempo == expected;
    std::printf("{\"operation\":\"runtime_tempo\",\"at\":%ld,\"tempo\":%.17g,\"time\":%ld,\"next\":%ld,\"expected\":%.17g,\"matches\":%s}\n",
        at, value.tempo, value.time, next, expected, ok ? "true" : "false");
    return ok;
}

static bool probe_initial_stream(IPersistStream* persist, const wchar_t* savePath, IUnknown* runtime) {
    IStream* stream = nullptr;
    HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &stream);
    result("create_memory_stream", hr);
    if (FAILED(hr)) return false;
    hr = persist->Save(stream, FALSE); result("save_initial_object", hr);
    bool ok = SUCCEEDED(hr);
    if (ok) {
        STATSTG stat{};
        hr = stream->Stat(&stat, STATFLAG_NONAME); result("saved_stream_stat", hr);
        ok = SUCCEEDED(hr) && stat.cbSize.QuadPart <= 1024 * 1024;
        if (ok) {
            LARGE_INTEGER zero{};
            hr = stream->Seek(zero, STREAM_SEEK_SET, nullptr); result("rewind_saved_stream", hr);
            ok = SUCCEEDED(hr);
            std::vector<unsigned char> bytes(static_cast<size_t>(stat.cbSize.QuadPart));
            ULONG read = 0;
            if (ok) { hr = stream->Read(bytes.data(), static_cast<ULONG>(bytes.size()), &read); result("read_saved_stream", hr); ok = SUCCEEDED(hr) && read == bytes.size(); }
            if (ok) {
                HANDLE file = CreateFileW(savePath, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (file == INVALID_HANDLE_VALUE) { result("create_output", HRESULT_FROM_WIN32(GetLastError())); ok = false; }
                else {
                    DWORD written = 0;
                    ok = WriteFile(file, bytes.data(), read, &written, nullptr) && written == read;
                    CloseHandle(file);
                    std::printf("{\"operation\":\"saved_bytes\",\"bytes\":%lu,\"written\":%s}\n", read, ok ? "true" : "false");
                }
            }
            if (ok) {
                hr = stream->Seek(zero, STREAM_SEEK_SET, nullptr); result("rewind_for_load", hr);
                if (SUCCEEDED(hr)) { hr = persist->Load(stream); result("load_saved_stream", hr); }
                ok = SUCCEEDED(hr);
                if (ok && runtime) {
                    std::vector<unsigned char> after;
                    ok = saved_bytes(persist, after) && after == bytes;
                    std::printf("{\"operation\":\"editor_roundtrip_bytes_equal\",\"equal\":%s}\n", ok ? "true" : "false");
                    const bool runtimeOk = check_runtime_tempo(runtime, 120.0);
                    ok = ok && runtimeOk;
                }
            }
        }
    }
    stream->Release(); return ok;
}

static void runtime_module(IUnknown* runtime) {
    const auto address = (*reinterpret_cast<void***>(runtime))[0];
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(address), &module)) return;
    wchar_t path[32768]{};
    const DWORD length = GetModuleFileNameW(module, path, 32768);
    if (!length || length >= 32768) return;
    const int count = WideCharToMultiByte(CP_UTF8, 0, path, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
    std::string utf8(count, '\0');
    WideCharToMultiByte(CP_UTF8, 0, path, static_cast<int>(length), utf8.data(), count, nullptr, nullptr);
    std::fputs("{\"operation\":\"runtime_module\",\"path\":\"", stdout);
    for (const unsigned char c : utf8) {
        if (c == '\\' || c == '"') std::putchar('\\');
        if (c < 32) std::printf("\\u%04x", c); else std::putchar(c);
    }
    std::puts("\"}");
}

static bool write_case_file(const std::wstring& path, const std::vector<unsigned char>& bytes) {
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    const bool ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
    CloseHandle(file); return ok;
}

static bool probe_manager_properties(producer::StripManager* manager) {
    bool ok = true;
    for (DWORD property = 0; property < 3; ++property) {
        VARIANT value{};
        const HRESULT hr = manager->GetStripMgrProperty(property, &value);
        std::printf("{\"operation\":\"get_disconnected_property\",\"property\":%lu,\"hresult\":\"0x%08lx\",\"type\":%u,\"null\":%s}\n",
            property, static_cast<unsigned long>(hr), value.vt, value.punkVal ? "false" : "true");
        ok = hr == E_FAIL && value.vt == VT_UNKNOWN && !value.punkVal && ok;
        if (value.vt == VT_UNKNOWN && value.punkVal) value.punkVal->Release();
    }
    struct Header { GUID classId; DWORD position; DWORD groups; DWORD chunk; DWORD list; } header{};
    VARIANT value{}; value.vt = VT_BYREF; value.byref = &header;
    HRESULT hr = manager->GetStripMgrProperty(3, &value); result("get_track_header", hr);
    guid_result("track_header_class_id", header.classId);
    std::printf("{\"operation\":\"track_header_fields\",\"position\":%lu,\"groups\":%lu,\"chunk\":%lu,\"list\":%lu}\n",
        header.position, header.groups, header.chunk, header.list);
    ok = hr == S_OK && header.classId == producer::CLSID_DirectMusicTempoTrack && !header.position && header.groups == 0xffffffff && header.chunk == 0x72746574 && !header.list && ok;
    header.groups = 8;
    hr = manager->SetStripMgrProperty(3, value); result("set_track_header", hr); ok = hr == S_OK && ok;
    hr = manager->GetStripMgrProperty(3, &value); result("get_track_header_after_set", hr);
    std::printf("{\"operation\":\"track_header_groups_after_set\",\"groups\":%lu}\n", header.groups);
    ok = hr == S_OK && header.groups == 0xffffffff && ok;
    for (DWORD property = 4; property <= 5; ++property) {
        DWORD flags = 0;
        value.vt = VT_BYREF; value.byref = &flags;
        hr = manager->GetStripMgrProperty(property, &value);
        std::printf("{\"operation\":\"get_initial_flags\",\"property\":%lu,\"hresult\":\"0x%08lx\",\"value\":%lu}\n",
            property, static_cast<unsigned long>(hr), flags);
        const DWORD initial = flags;
        ok = hr == S_OK && flags == (property == 4 ? 0x18u : 0u) && ok;
        flags = 0xffffffff;
        hr = manager->SetStripMgrProperty(property, value); result("set_flags", hr); ok = hr == S_OK && ok;
        flags = 0;
        hr = manager->GetStripMgrProperty(property, &value);
        std::printf("{\"operation\":\"get_changed_flags\",\"property\":%lu,\"hresult\":\"0x%08lx\",\"value\":%lu}\n",
            property, static_cast<unsigned long>(hr), flags);
        ok = hr == S_OK && flags == (property == 4 ? 0x11c58u : 0xffffffffu) && ok;
        flags = initial;
        hr = manager->SetStripMgrProperty(property, value); result("restore_flags", hr); ok = hr == S_OK && ok;
    }
    value = {};
    hr = manager->GetStripMgrProperty(6, &value); result("get_supported_flags", hr);
    std::printf("{\"operation\":\"supported_flags_value\",\"type\":%u,\"value\":%ld}\n", value.vt, value.lVal);
    ok = hr == S_OK && value.vt == VT_I4 && value.lVal == 0x11c58 && ok;
    hr = manager->GetStripMgrProperty(0, nullptr); result("get_property_null_output", hr); ok = hr == E_POINTER && ok;
    value = {};
    hr = manager->SetStripMgrProperty(1, value); result("set_property_wrong_type", hr); ok = hr == E_INVALIDARG && ok;
    hr = manager->GetStripMgrProperty(7, &value); result("get_unknown_property", hr); ok = hr == E_INVALIDARG && ok;
    hr = manager->GetParam(runtime_tempo::TempoParamGuid, 0, nullptr, nullptr);
    result("get_tempo_null_output", hr); ok = hr == E_POINTER && ok;
    return ok;
}

static bool check_editor_tempo(producer::StripManager* strip, ReferenceTimeline* timeline, LONG at, LONG eventTime, double expected) {
    runtime_tempo::TempoParam value{};
    LONG next = 0;
    HRESULT hr = strip->GetParam(runtime_tempo::TempoParamGuid, at, &next, &value);
    result("get_editor_tempo", hr);
    bool ok = hr == S_OK && value.tempo == expected && value.time == eventTime;
    std::printf("{\"operation\":\"editor_tempo\",\"at\":%ld,\"tempo\":%.17g,\"time\":%ld,\"next\":%ld,\"matches\":%s}\n",
        at, value.tempo, value.time, next, ok ? "true" : "false");
    if (timeline) {
        runtime_tempo::TempoParam viaTimeline{};
        LONG nextViaTimeline = 0;
        hr = timeline->get(runtime_tempo::TempoParamGuid, at, &nextViaTimeline, &viaTimeline);
        result("get_timeline_tempo", hr);
        const bool same = hr == S_OK && viaTimeline.time == value.time && viaTimeline.tempo == value.tempo && nextViaTimeline == next;
        std::printf("{\"operation\":\"timeline_dispatch_matches_editor\",\"same\":%s}\n", same ? "true" : "false");
        ok = ok && same;
    }
    return ok;
}

static bool observe_undo_label(producer::StripManager* manager, const char* phase) {
    // TempoStripMgr constant RVA 0x26f4, GetParam branch RVA 0x7226.
    const GUID type = {0x178633a6,0x4452,0x11d2,{0x89,0x0c,0,0xc0,0x4f,0xbf,0x8d,0x15}};
    const HRESULT supported = manager->IsParamSupported(type);
    BSTR label = nullptr; LONG next = 0x12345678;
    const HRESULT hr = manager->GetParam(type, 0, &next, &label);
    std::printf("{\"operation\":\"undo_label\",\"phase\":\"%s\",\"supported\":\"0x%08lx\",\"hresult\":\"0x%08lx\",\"next\":%ld,\"utf16_hex\":\"",
        phase, static_cast<unsigned long>(supported), static_cast<unsigned long>(hr), next);
    for (UINT i = 0; i < SysStringLen(label); ++i) std::printf("%04x", label[i]);
    std::puts("\"}");
    SysFreeString(label);
    return supported == S_OK && hr == S_OK;
}

static bool probe_tempo_cases(IPersistStream* persist, IUnknown* runtime, producer::StripManager* strip,
    ReferenceTimeline* timeline, const wchar_t* initialPath) {
    if (!observe_undo_label(strip, "initial")) return false;
    struct Event { LONG time; double tempo; };
    struct Case { const wchar_t* file; const char* name; std::vector<Event> events; };
    // These are synthetic stream edits, not claims about the editor UI or Undo.
    const Case cases[] = {
        {L"single", "single_137", {{0, 137.0}}},
        {L"fractional", "fractional_93_75", {{0, 93.75}}},
        {L"multiple", "multiple_events", {{0, 120.0}, {768, 150.0}, {1536, 90.0}}},
        {L"unsorted", "unsorted_events", {{1536, 90.0}, {0, 120.0}, {768, 150.0}}},
        {L"duplicate", "duplicate_times", {{0, 120.0}, {768, 150.0}, {768, 95.0}, {1536, 90.0}}},
        {L"replace", "replace_with_single", {{0, 100.0}}},
    };
    std::wstring parent(initialPath);
    parent.resize(parent.find_last_of(L"/\\") + 1);
    for (const auto& item : cases) {
        std::printf("{\"operation\":\"begin_stream_case\",\"case\":\"%s\"}\n", item.name);
        std::vector<unsigned char> input(12 + item.events.size() * 16, 0);
        std::memcpy(input.data(), "tetr", 4);
        const DWORD payloadSize = static_cast<DWORD>(input.size() - 8), recordSize = 16;
        std::memcpy(input.data() + 4, &payloadSize, 4);
        std::memcpy(input.data() + 8, &recordSize, 4);
        for (size_t i = 0; i < item.events.size(); ++i) {
            std::memcpy(input.data() + 12 + i * 16, &item.events[i].time, 4);
            std::memcpy(input.data() + 20 + i * 16, &item.events[i].tempo, 8);
        }
        auto sorted = item.events;
        std::stable_sort(sorted.begin(), sorted.end(), [](const Event& a, const Event& b) { return a.time < b.time; });
        auto expected = input;
        for (size_t i = 0; i < sorted.size(); ++i) {
            std::memcpy(expected.data() + 12 + i * 16, &sorted[i].time, 4);
            std::memcpy(expected.data() + 20 + i * 16, &sorted[i].tempo, 8);
        }
        bool ok = write_case_file(parent + item.file + L"-input.bin", input);
        IStream* stream = nullptr;
        HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &stream);
        if (FAILED(hr)) return false;
        ULONG written = 0;
        hr = stream->Write(input.data(), static_cast<ULONG>(input.size()), &written);
        LARGE_INTEGER zero{};
        if (SUCCEEDED(hr) && written == input.size()) hr = stream->Seek(zero, STREAM_SEEK_SET, nullptr);
        else hr = E_FAIL;
        if (SUCCEEDED(hr)) hr = persist->Load(stream);
        result("load_case_stream", hr);
        stream->Release();
        ok = ok && SUCCEEDED(hr);
        std::vector<unsigned char> output;
        if (SUCCEEDED(hr)) {
            const bool snapshotOk = saved_bytes(persist, output);
            const bool equal = snapshotOk && expected == output;
            std::printf("{\"operation\":\"case_saved_bytes_match_expected\",\"equal\":%s,\"input_was_sorted\":%s}\n",
                equal ? "true" : "false", input == expected ? "true" : "false");
            ok = write_case_file(parent + item.file + L"-output.bin", output) && equal && ok;
            for (size_t i = 0; i < sorted.size(); ++i) {
                const auto& event = sorted[i];
                if (i + 1 < sorted.size() && sorted[i + 1].time == event.time) continue;
                ok = check_runtime_tempo(runtime, event.tempo, event.time) && ok;
                ok = check_runtime_tempo(runtime, event.tempo, event.time + 1) && ok;
                ok = check_editor_tempo(strip, timeline, event.time, event.time, event.tempo) && ok;
                ok = check_editor_tempo(strip, timeline, event.time + 1, event.time, event.tempo) && ok;
            }
            // The last case must remove the earlier track's future events too.
            ok = check_runtime_tempo(runtime, sorted.back().tempo, 3072) && ok;
        }
        std::printf("{\"operation\":\"end_stream_case\",\"case\":\"%s\",\"passed\":%s}\n", item.name, ok ? "true" : "false");
        if (!ok) return false;
    }
    return true;
}

static bool probe_property_edit(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip); result("edit_enum_strip", hr);
    if (FAILED(hr) || !strip) return false;
    for (DWORD property : {0u, 1u, 6u, 7u, 8u, 9u, 10u}) {
        using GetProperty = HRESULT (STDMETHODCALLTYPE*)(IUnknown*, DWORD, VARIANT*);
        VARIANT value{};
        hr = reinterpret_cast<GetProperty>((*reinterpret_cast<void***>(strip))[4])(strip, property, &value);
        std::printf("{\"operation\":\"strip_property\",\"property\":%lu,\"hresult\":\"0x%08lx\",\"type\":%u",
            property, static_cast<unsigned long>(hr), value.vt);
        if (value.vt == VT_BSTR) {
            std::printf(",\"utf16_hex\":\"");
            for (UINT i = 0; i < SysStringLen(value.bstrVal); ++i) std::printf("%04x", value.bstrVal[i]);
            std::printf("\"");
            SysFreeString(value.bstrVal);
        } else if (value.vt == VT_BOOL) std::printf(",\"value\":%d", value.boolVal);
        else if (value.vt == VT_INT) std::printf(",\"value\":%d", value.intVal);
        std::puts("}");
    }
    VARIANT stripManager{};
    hr = reference_edit::get_strip_manager(strip, &stripManager); result("edit_get_strip_manager", hr);
    IUnknown* identity = nullptr;
    if (SUCCEEDED(hr) && stripManager.vt == VT_UNKNOWN && stripManager.punkVal)
        hr = stripManager.punkVal->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(&identity));
    const bool sameManager = identity == static_cast<IUnknown*>(manager);
    if (identity) identity->Release();
    if (stripManager.vt == VT_UNKNOWN && stripManager.punkVal) stripManager.punkVal->Release();
    std::printf("{\"operation\":\"edit_strip_manager_identity\",\"same\":%s}\n", sameManager ? "true" : "false");
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit));
    result("query_timeline_edit", hr);
    strip->Release();
    if (FAILED(hr) || !edit) return false;
    IUnknown* properties = nullptr;
    hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject, reinterpret_cast<void**>(&properties));
    result("query_edit_properties", hr);
    if (FAILED(hr) || !properties) { edit->Release(); return false; }
    bool ok = sameManager;
    result("can_delete_before_selection", reference_edit::can_delete(edit));
    result("can_select_all_before_selection", reference_edit::can_select_all(edit));
    void* unselectedData = nullptr;
    hr = reference_edit::get_data(properties, &unselectedData); result("get_unselected_tempo_data", hr);
    std::printf("{\"operation\":\"unselected_tempo_data_null\",\"null\":%s}\n", unselectedData ? "false" : "true");
    hr = reference_edit::select_all(edit); result("select_all_tempo_events", hr);
    ok = ok && hr == S_OK;
    result("can_delete_after_selection", reference_edit::can_delete(edit));
    void* borrowedData = nullptr;
    hr = reference_edit::get_data(properties, &borrowedData); result("get_selected_tempo_data", hr);
    ok = ok && hr == S_OK && borrowedData;
    if (ok) {
        // GetData returns a borrowed property snapshot. Preserve every known
        // field through offset 0x21 and change only double tempo at offset8.
        alignas(8) unsigned char data[0x22]{};
        std::memcpy(data, borrowedData, sizeof(data));
        double before = 0; std::memcpy(&before, data + 8, sizeof(before));
        const double edited = 164.25;
        std::memcpy(data + 8, &edited, sizeof(edited));
        hr = reference_edit::set_data(properties, data); result("set_selected_tempo_data", hr);
        if (!observe_undo_label(manager, "tempo_change")) { properties->Release(); edit->Release(); return false; }
        result("is_dirty_after_property_edit", persist->IsDirty());
        ok = hr == S_OK && before == 100.0;
        ok = check_editor_tempo(manager, &timeline, 0, 0, edited) && ok;
        ok = check_runtime_tempo(runtime, edited) && ok;
        std::vector<unsigned char> output;
        ok = saved_bytes(persist, output) && ok;
        double stored = 0;
        if (output.size() == 28) std::memcpy(&stored, output.data() + 20, sizeof(stored));
        ok = output.size() == 28 && stored == edited && ok;
        std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
        ok = write_case_file(parent + L"property-edit-output.bin", output) && ok;
        std::printf("{\"operation\":\"property_edit_verified\",\"before\":%.17g,\"after\":%.17g,\"passed\":%s}\n",
            before, stored, ok ? "true" : "false");
        if (ok) {
            hr = reference_edit::delete_selected(edit); result("delete_selected_tempo_events", hr);
            if (!observe_undo_label(manager, "delete")) { properties->Release(); edit->Release(); return false; }
            result("is_dirty_after_delete", persist->IsDirty());
            result("can_delete_after_delete", reference_edit::can_delete(edit));
            result("can_select_all_after_delete", reference_edit::can_select_all(edit));
            ok = hr == S_OK;
            output.clear();
            ok = saved_bytes(persist, output) && ok;
            ok = write_case_file(parent + L"delete-output.bin", output) && ok;
            runtime_tempo::TempoParam emptyValue{};
            LONG next = -1;
            hr = manager->GetParam(runtime_tempo::TempoParamGuid, 0, &next, &emptyValue);
            result("get_editor_tempo_after_delete", hr);
            const bool empty = hr == S_FALSE && emptyValue.tempo == 120.0 && output.size() == 12;
            std::printf("{\"operation\":\"delete_observation\",\"saved_bytes\":%zu,\"fallback_tempo\":%.17g,\"empty\":%s}\n",
                output.size(), emptyValue.tempo, empty ? "true" : "false");
            ok = ok && empty;
            borrowedData = nullptr;
            hr = reference_edit::get_data(properties, &borrowedData); result("get_deleted_tempo_data", hr);
            std::printf("{\"operation\":\"deleted_tempo_data_null\",\"null\":%s}\n", borrowedData ? "false" : "true");
            hr = reference_edit::set_data(properties, data); result("set_data_without_selection", hr);
            runtime_tempo::TempoParam runtimeEmpty{};
            next = 0;
            hr = runtime_tempo::get(runtime, 0, &next, &runtimeEmpty); result("get_runtime_tempo_after_delete", hr);
            if (SUCCEEDED(hr)) std::printf("{\"operation\":\"runtime_tempo_after_delete\",\"tempo\":%.17g,\"time\":%ld,\"next\":%ld}\n",
                runtimeEmpty.tempo, runtimeEmpty.time, next);
        }
    }
    properties->Release(); edit->Release();
    return ok;
}

static void position_data(const char* phase, LONG originalTime, const unsigned char* data) {
    LONG time = 0, measure = 0, beat = 0, tick = 0;
    double tempo = 0;
    DWORD extra = 0; WORD flags = 0;
    std::memcpy(&time, data, 4); std::memcpy(&tempo, data + 8, 8);
    std::memcpy(&measure, data + 16, 4); std::memcpy(&beat, data + 20, 4);
    std::memcpy(&tick, data + 24, 4); std::memcpy(&extra, data + 28, 4); std::memcpy(&flags, data + 32, 2);
    std::printf("{\"operation\":\"position_property_data\",\"phase\":\"%s\",\"original_time\":%ld,\"time\":%ld,\"tempo\":%.17g,\"measure\":%ld,\"beat\":%ld,\"tick\":%ld,\"extra\":%lu,\"flags\":%u}\n",
        phase, originalTime, time, tempo, measure, beat, tick, extra, flags);
}

static HRESULT load_bytes(IPersistStream* persist, const std::vector<unsigned char>& bytes) {
    IStream* stream = nullptr;
    HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &stream);
    if (FAILED(hr)) return hr;
    ULONG written = 0;
    hr = stream->Write(bytes.data(), static_cast<ULONG>(bytes.size()), &written);
    LARGE_INTEGER zero{};
    if (SUCCEEDED(hr) && written != bytes.size()) hr = E_FAIL;
    if (SUCCEEDED(hr)) hr = stream->Seek(zero, STREAM_SEEK_SET, nullptr);
    if (SUCCEEDED(hr)) hr = persist->Load(stream);
    stream->Release(); return hr;
}

#ifdef PRODUCER_WINDOWED_PROBE
struct PageTrackContext {
    IPersistStream* persist;
    IUnknown* runtime;
    producer::StripManager* manager;
    ReferenceTimeline* timeline;
    const wchar_t* initialPath;
};
static bool probe_page_track_edits(HWND page, producer::PropPageManager* pages,
    producer::PropPageObject* properties, void* context) {
    const auto& host=*static_cast<PageTrackContext*>(context);
    IUnknown* strip=nullptr;IUnknown* edit=nullptr;
    HRESULT hr=host.timeline->find_manager_strip(host.manager,&strip);
    if(SUCCEEDED(hr))hr=strip->QueryInterface(reference_edit::TimelineEditIid,reinterpret_cast<void**>(&edit));
    if(strip)strip->Release();
    if(FAILED(hr)||!edit)return false;
    bool ok=true;
    const auto encode=[](LONG time,double tempo) {
        std::vector<unsigned char> bytes(28,0);std::memcpy(bytes.data(),"tetr",4);
        DWORD payload=20,record=16;std::memcpy(bytes.data()+4,&payload,4);std::memcpy(bytes.data()+8,&record,4);
        std::memcpy(bytes.data()+12,&time,4);std::memcpy(bytes.data()+20,&tempo,8);return bytes;
    };
    struct Case {const char* name;int id;const char* text;LONG time;double tempo;};
    for(const auto& test:{Case{"tempo",223,"145.50",900,145.5},Case{"clamp",223,"0",900,1},
        Case{"measure",224,"2",3972,137.25},Case{"beat",225,"3",1668,137.25},Case{"tick",233,"42",810,137.25}}) {
        std::printf("{\"operation\":\"begin_page_track_case\",\"case\":\"%s\"}\n",test.name);
        std::wstring parent(host.initialPath);parent.resize(parent.find_last_of(L"/\\")+1);
        const auto name=parent+L"page-track-"+std::wstring(test.name,test.name+std::strlen(test.name));
        const auto input=encode(900,137.25);
        hr=load_bytes(host.persist,input);result("page_track_load",hr);ok=hr==S_OK&&ok;
        ok=write_case_file(name+L"-input.bin",input)&&ok;
        hr=reference_edit::select_all(edit);result("page_track_select",hr);ok=hr==S_OK&&ok;
        hr=pages->SetObject(properties);result("page_track_set_object",hr);ok=hr==S_OK&&ok;
        hr=pages->RefreshData();result("page_track_refresh",hr);ok=hr==S_OK&&ok;
        const auto control=GetDlgItem(page,test.id);SetWindowTextA(control,test.text);
        SendMessageW(page,WM_COMMAND,MAKEWPARAM(test.id,EN_KILLFOCUS),reinterpret_cast<LPARAM>(control));
        void* borrowed=nullptr;hr=properties->GetData(&borrowed);result("page_track_get_data",hr);
        property_probe::PageData actual{};if(borrowed)std::memcpy(&actual,borrowed,0x22);
        ok=hr==S_OK&&borrowed&&actual.time==test.time&&actual.tempo==test.tempo&&ok;
        std::printf("{\"operation\":\"page_track_data\",\"time\":%ld,\"tempo\":%.17g,\"measure\":%ld,\"beat\":%ld,\"tick\":%ld,\"flags\":%u}\n",
            actual.time,actual.tempo,actual.measure,actual.beat,actual.tick,actual.flags);
        ok=check_runtime_tempo(host.runtime,test.tempo,test.time)&&ok;
        std::vector<unsigned char> output,reloaded;
        ok=saved_bytes(host.persist,output)&&write_case_file(name+L"-output.bin",output)&&output==encode(test.time,test.tempo)&&ok;
        hr=load_bytes(host.persist,output);result("page_track_reload",hr);ok=hr==S_OK&&ok;
        ok=saved_bytes(host.persist,reloaded)&&write_case_file(name+L"-reload.bin",reloaded)&&reloaded==output&&ok;
        std::printf("{\"operation\":\"end_page_track_case\",\"case\":\"%s\",\"passed\":%s}\n",test.name,ok?"true":"false");
    }
    // Do not explicitly refresh after clicking: the strip must refresh the page.
    std::puts("{\"operation\":\"begin_page_selection_probe\"}");
    std::vector<unsigned char> selectionInput(44,0);
    std::memcpy(selectionInput.data(),"tetr",4);
    const DWORD payload=36,recordSize=16; const LONG thirdTime=6144; const double bpm=112;
    std::memcpy(selectionInput.data()+4,&payload,4);std::memcpy(selectionInput.data()+8,&recordSize,4);
    std::memcpy(selectionInput.data()+20,&bpm,8);std::memcpy(selectionInput.data()+28,&thirdTime,4);std::memcpy(selectionInput.data()+36,&bpm,8);
    std::wstring selectionName(host.initialPath);selectionName.resize(selectionName.find_last_of(L"/\\")+1);selectionName+=L"page-selection";
    hr=load_bytes(host.persist,selectionInput);result("page_selection_load",hr);ok=hr==S_OK&&ok;
    ok=write_case_file(selectionName+L"-input.bin",selectionInput)&&ok;
    hr=pages->SetObject(properties);result("page_selection_set_object",hr);ok=hr==S_OK&&ok;
    hr=host.timeline->set_zoom(0.0625);result("page_selection_zoom",hr);ok=hr==S_OK&&ok;
    IUnknown* selectionStrip=nullptr;
    hr=host.timeline->find_manager_strip(host.manager,&selectionStrip);result("page_selection_strip",hr);ok=hr==S_OK&&selectionStrip&&ok;
    struct SelectionStep {const char* name;LONG time;WPARAM keys;};
    if(selectionStrip)for(const auto& step:{SelectionStep{"third",6144,0},SelectionStep{"first",0,0},
        SelectionStep{"control_add_third",6144,MK_CONTROL},SelectionStep{"collapse_first",0,0},SelectionStep{"empty_second",3072,0}}) {
        LONG position=0;hr=host.timeline->clocks_to_position(step.time,&position);ok=hr==S_OK&&ok;
        for(UINT message:{WM_LBUTTONDOWN,WM_LBUTTONUP}) {
            using Message=HRESULT (STDMETHODCALLTYPE*)(IUnknown*,UINT,WPARAM,LPARAM,LONG,LONG);
            hr=reinterpret_cast<Message>((*reinterpret_cast<void***>(selectionStrip))[6])(selectionStrip,message,step.keys,0,position+1,0);
            result(message==WM_LBUTTONDOWN?"page_selection_down":"page_selection_up",hr);ok=hr==S_OK&&ok;
            void* borrowed=nullptr;hr=properties->GetData(&borrowed);ok=hr==S_OK&&ok;
            property_probe::PageData data{};if(borrowed)std::memcpy(&data,borrowed,0x22);
            const bool multiple=borrowed&&(data.flags&2),single=borrowed&&!multiple;
            char tempoText[256]{},measureText[32]{},beatText[32]{},tickText[32]{};
            GetDlgItemTextA(page,223,tempoText,256);GetDlgItemTextA(page,224,measureText,32);
            GetDlgItemTextA(page,225,beatText,32);GetDlgItemTextA(page,233,tickText,32);
            char expectedTempo[64]{},expectedMeasure[32]{},expectedBeat[32]{},expectedTick[32]{};
            if(single){sprintf_s(expectedTempo,"%.2f",data.tempo);sprintf_s(expectedMeasure,"%ld",data.measure+1);sprintf_s(expectedBeat,"%ld",data.beat+1);sprintf_s(expectedTick,"%ld",data.tick);}
            else strcpy_s(expectedTempo,multiple?"Multiple Tempos Selected":"None");
            bool matches=std::strcmp(tempoText,expectedTempo)==0&&std::strcmp(measureText,expectedMeasure)==0&&
                std::strcmp(beatText,expectedBeat)==0&&std::strcmp(tickText,expectedTick)==0;
            for(int id:{223,224,225,233})matches=(IsWindowEnabled(GetDlgItem(page,id))!=FALSE)==single&&matches;
            std::printf("{\"operation\":\"page_selection_display\",\"case\":\"%s\",\"phase\":\"%s\",\"present\":%s,\"multiple\":%s,\"time\":%ld,\"measure\":%ld,\"tempo_text\":\"%s\",\"measure_text\":\"%s\",\"beat_text\":\"%s\",\"tick_text\":\"%s\",\"matches_selection\":%s}\n",
                step.name,message==WM_LBUTTONDOWN?"down":"up",borrowed?"true":"false",multiple?"true":"false",data.time,data.measure,tempoText,measureText,beatText,tickText,matches?"true":"false");
            ok=matches&&ok;
        }
    }
    if(selectionStrip)selectionStrip->Release();
    std::vector<unsigned char> selectionOutput;
    ok=saved_bytes(host.persist,selectionOutput)&&write_case_file(selectionName+L"-output.bin",selectionOutput)&&selectionOutput==selectionInput&&ok;
    std::printf("{\"operation\":\"end_page_selection_probe\",\"passed\":%s,\"saved_unchanged\":%s}\n",ok?"true":"false",selectionOutput==selectionInput?"true":"false");
    hr=pages->RemoveObject(properties);result("page_track_remove_object",hr);ok=hr==S_OK&&ok;
    edit->Release();return ok;
}
#endif

static bool probe_position_boundaries(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    struct Event { LONG time; double tempo; };
    struct Case {
        const char* name;
        std::vector<Event> events;
        LONG measure, beat, tick, length;
        double tempo;
    };
    const Case cases[] = {
        {"negative_tick", {{900,118}}, 0,0,-100,30720,222},
        {"beyond_end", {{900,118}}, 20,0,0,30720,222},
        {"partial_measure_end", {{900,118}}, 0,2,0,1000,222},
        {"multiple_tempo", {{0,118},{768,150},{1536,90}}, 0,0,0,30720,133.5},
        {"multiple_collision", {{0,118},{768,150},{1536,90}}, 0,1,0,30720,222},
        {"multiple_reorder", {{0,118},{768,150},{1536,90}}, 1,0,0,30720,222},
    };
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip); result("boundary_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit));
    strip->Release();
    if (FAILED(hr) || !edit) return false;
    IUnknown* properties = nullptr;
    hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject, reinterpret_cast<void**>(&properties));
    if (FAILED(hr) || !properties) { edit->Release(); return false; }
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    bool allOk = true;
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_boundary_case\",\"case\":\"%s\",\"measure\":%ld,\"beat\":%ld,\"tick\":%ld,\"length\":%ld,\"tempo\":%.17g}\n",
            test.name, test.measure, test.beat, test.tick, test.length, test.tempo);
        std::wstring name = parent + L"boundary-";
        for (const char* letter = test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        std::vector<unsigned char> input(12 + test.events.size() * 16, 0);
        std::memcpy(input.data(), "tetr", 4);
        const DWORD payload = static_cast<DWORD>(input.size() - 8);
        std::memcpy(input.data() + 4, &payload, 4); input[8] = 16;
        for (size_t i = 0; i < test.events.size(); ++i) {
            std::memcpy(input.data() + 12 + i * 16, &test.events[i].time, 4);
            std::memcpy(input.data() + 20 + i * 16, &test.events[i].tempo, 8);
        }
        bool ok = write_case_file(name + L"-input.bin", input);
        hr = timeline.set_length(test.length); result("boundary_set_length", hr); ok = hr == S_OK && ok;
        if (ok) { hr = load_bytes(persist, input); result("boundary_load_input", hr); ok = hr == S_OK; }
        if (ok) { hr = reference_edit::select_all(edit); result("boundary_select_all", hr); ok = hr == S_OK; }
        void* borrowed = nullptr;
        if (ok) { hr = reference_edit::get_data(properties, &borrowed); result("boundary_get_before", hr); ok = hr == S_OK && borrowed; }
        alignas(8) unsigned char data[0x22]{};
        if (ok) {
            std::memcpy(data, borrowed, sizeof(data)); position_data("boundary_before", test.events[0].time, data);
            std::memcpy(data + 8, &test.tempo, 8);
            std::memcpy(data + 16, &test.measure, 4);
            std::memcpy(data + 20, &test.beat, 4);
            std::memcpy(data + 24, &test.tick, 4);
            hr = reference_edit::set_data(properties, data); result("boundary_set_data", hr); ok = hr == S_OK;
        }
        std::vector<unsigned char> output;
        if (ok) ok = saved_bytes(persist, output) && write_case_file(name + L"-output.bin", output);
        if (ok) {
            hr = reference_edit::get_data(properties, &borrowed); result("boundary_get_after", hr);
            if (hr == S_OK && borrowed) { std::memcpy(data, borrowed, sizeof(data)); position_data("boundary_after", test.events[0].time, data); }
            else ok = false;
        }
        if (ok && (output.size() < 12 || (output.size() - 12) % 16)) ok = false;
        // Record the original's result first. No inferred boundary value is
        // used as an expected result; the comparator checks both observations.
        if (ok) for (size_t offset = 12; offset < output.size(); offset += 16) {
            Event value{};
            std::memcpy(&value.time, output.data() + offset, 4);
            std::memcpy(&value.tempo, output.data() + offset + 8, 8);
            std::printf("{\"operation\":\"boundary_saved_event\",\"index\":%zu,\"time\":%ld,\"tempo\":%.17g}\n",
                (offset - 12) / 16, value.time, value.tempo);
            LONG nextTime = value.time + 1;
            if (offset + 16 < output.size()) std::memcpy(&nextTime, output.data() + offset + 16, 4);
            if (nextTime != value.time) {
                ok = check_editor_tempo(manager, &timeline, value.time, value.time, value.tempo) && ok;
                ok = check_runtime_tempo(runtime, value.tempo, value.time) && ok;
            }
        }
        if (ok) {
            hr = load_bytes(persist, output); result("boundary_reload_output", hr); ok = hr == S_OK;
            std::vector<unsigned char> after;
            ok = saved_bytes(persist, after) && ok;
            const bool equal = output == after;
            std::printf("{\"operation\":\"boundary_roundtrip_equal\",\"equal\":%s}\n", equal ? "true" : "false");
            ok = write_case_file(name + L"-reload-output.bin", after) && equal && ok;
        }
        std::printf("{\"operation\":\"end_boundary_case\",\"case\":\"%s\",\"passed\":%s}\n", test.name, ok ? "true" : "false");
        allOk = allOk && ok;
    }
    hr = timeline.set_length(30720); result("boundary_restore_length", hr);
    properties->Release(); edit->Release(); return allOk && hr == S_OK;
}

static bool probe_notification_registration(ReferenceTimeline& timeline, IUnknown* manager, bool connected) {
    // Independent constants from original TempoStripMgr registration calls.
    const GUID types[] = {
        {0x7cfd1ee0,0xcbef,0x11d2,{0x85,0x45,0,0x10,0x5a,0x27,0x96,0xde}},
        {0x96a0a26c,0xf4e7,0x11d1,{0x88,0xcb,0,0xc0,0x4f,0xbf,0x8d,0x15}},
        {0xd2ac28a4,0xb39b,0x11d1,{0x87,0x04,0,0x60,0x08,0x93,0xb1,0xbd}},
        {0x1528eab8,0xc518,0x11d2,{0xb0,0xe7,0,0x10,0x5a,0x26,0x62,0x0b}},
        {0x71754743,0xa98d,0x11d2,{0xb0,0xd3,0,0x10,0x5a,0x26,0x62,0x0b}},
        {0x71754744,0xa98d,0x11d2,{0xb0,0xd3,0,0x10,0x5a,0x26,0x62,0x0b}},
    };
    bool ok = true;
    for (const auto& type : types) {
        guid_result("notification_type", type);
        HRESULT hr = timeline.remove_notification(manager, type, 0xffffffff);
        result(connected ? "remove_registered_notification" : "notification_absent_after_disconnect", hr);
        ok = hr == (connected ? S_OK : E_INVALIDARG) && ok;
        if (connected) {
            hr = timeline.remove_notification(manager, type, 0xffffffff);
            result("notification_no_duplicate", hr); ok = hr == E_INVALIDARG && ok;
            hr = timeline.add_notification(manager, type, 0xffffffff);
            result("restore_registered_notification", hr); ok = hr == S_OK && ok;
        }
    }
    std::printf("{\"operation\":\"notification_registration_verified\",\"connected\":%s,\"passed\":%s}\n",
        connected ? "true" : "false", ok ? "true" : "false");
    return ok;
}

static bool probe_meter_changes(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, ReferenceTimeSignature& meter, const wchar_t* initialPath) {
    constexpr GUID meterChanged = {0xd2ac28a4,0xb39b,0x11d1,{0x87,0x04,0x00,0x60,0x08,0x93,0xb1,0xbd}};
    constexpr GUID refreshPositions = {0x96a0a26c,0xf4e7,0x11d1,{0x88,0xcb,0x00,0xc0,0x4f,0xbf,0x8d,0x15}};
    struct Meter { BYTE beats, denominator; const char* name; };
    const Meter cases[] = {{3,4,"three_four"}, {3,8,"three_eight"}, {5,4,"five_four"}};
    struct Event { LONG time; double tempo; };
    const Event events[] = {{900,118}, {2304,137}, {2404,142}, {4708,150}, {7528,90}};
    std::vector<unsigned char> input(12 + sizeof(events) / sizeof(events[0]) * 16, 0);
    std::memcpy(input.data(), "tetr", 4);
    const DWORD payload = static_cast<DWORD>(input.size() - 8);
    std::memcpy(input.data() + 4, &payload, 4); input[8] = 16;
    for (size_t i = 0; i < sizeof(events) / sizeof(events[0]); ++i) {
        std::memcpy(input.data() + 12 + i * 16, &events[i].time, 4);
        std::memcpy(input.data() + 20 + i * 16, &events[i].tempo, 8);
    }
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip); result("meter_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit)); strip->Release();
    if (FAILED(hr) || !edit) return false;
    IUnknown* properties = nullptr;
    hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject, reinterpret_cast<void**>(&properties));
    if (FAILED(hr) || !properties) { edit->Release(); return false; }
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    bool allOk = true;
    hr = manager->OnUpdate(GUID_NULL, 1, nullptr); result("update_unknown_notification", hr);
    allOk = hr == E_FAIL;
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_meter_case\",\"case\":\"%s\",\"beats\":%u,\"denominator\":%u}\n", test.name, test.beats, test.denominator);
        std::wstring name = parent + L"meter-";
        for (const char* letter = test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        bool ok = write_case_file(name + L"-input.bin", input);
        hr = meter.seed_default_meter(); result("meter_restore_default", hr); ok = hr == S_OK && ok;
        if (ok) { hr = load_bytes(persist, input); result("meter_load_tempos", hr); ok = hr == S_OK; }
        if (ok) { hr = reference_edit::select_all(edit); result("meter_select_all", hr); ok = hr == S_OK; }
        void* borrowed = nullptr;
        alignas(8) unsigned char data[0x22]{};
        if (ok) {
            hr = reference_edit::get_data(properties, &borrowed); result("meter_get_before", hr);
            ok = hr == S_OK && borrowed;
            if (ok) { std::memcpy(data, borrowed, sizeof(data)); position_data("meter_before", events[0].time, data); }
        }
        if (ok) {
            ok = write_case_file(name + L"-time-signature-input.bin", ReferenceTimeSignature::meter_stream(test.beats, test.denominator));
            hr = meter.seed_meter(test.beats, test.denominator); result("meter_load_changed_signature", hr); ok = hr == S_OK && ok;
        }
        if (ok) {
            hr = reference_edit::get_data(properties, &borrowed); result("meter_get_before_notification", hr);
            ok = hr == S_OK && borrowed;
            if (ok) { std::memcpy(data, borrowed, sizeof(data)); position_data("meter_before_notification", events[0].time, data); }
            hr = timeline.notify(meterChanged, 0xffffffff, nullptr); result("meter_notify_timeline", hr); ok = hr == S_OK && ok;
        }
        std::vector<unsigned char> output;
        if (ok) ok = saved_bytes(persist, output) && write_case_file(name + L"-output.bin", output);
        if (ok) {
            hr = reference_edit::get_data(properties, &borrowed); result("meter_get_after_notification", hr);
            ok = hr == S_OK && borrowed;
            if (ok) { std::memcpy(data, borrowed, sizeof(data)); position_data("meter_after_notification", events[0].time, data); }
        }
        if (ok && (output.size() < 12 || (output.size() - 12) % 16)) ok = false;
        if (ok) for (size_t offset = 12; offset < output.size(); offset += 16) {
            Event value{};
            std::memcpy(&value.time, output.data() + offset, 4); std::memcpy(&value.tempo, output.data() + offset + 8, 8);
            std::printf("{\"operation\":\"meter_saved_event\",\"index\":%zu,\"time\":%ld,\"tempo\":%.17g}\n", (offset - 12) / 16, value.time, value.tempo);
            LONG next = value.time + 1;
            if (offset + 16 < output.size()) std::memcpy(&next, output.data() + offset + 16, 4);
            if (next != value.time) {
                ok = check_editor_tempo(manager, &timeline, value.time, value.time, value.tempo) && ok;
                ok = check_runtime_tempo(runtime, value.tempo, value.time) && ok;
            }
        }
        if (ok) {
            // This notification recomputes coordinates while keeping absolute
            // clocks. It is distinct from moving events with meter changes.
            hr = meter.seed_default_meter(); result("meter_restore_for_refresh", hr); ok = hr == S_OK;
            hr = timeline.notify(refreshPositions, 0xffffffff, nullptr); result("meter_refresh_positions", hr); ok = hr == S_OK && ok;
            hr = reference_edit::get_data(properties, &borrowed); result("meter_get_refreshed", hr);
            if (hr == S_OK && borrowed) { std::memcpy(data, borrowed, sizeof(data)); position_data("meter_refreshed", events[0].time, data); }
            else ok = false;
            std::vector<unsigned char> after;
            ok = saved_bytes(persist, after) && ok;
            const bool equal = output == after;
            std::printf("{\"operation\":\"meter_refresh_preserved_clocks\",\"equal\":%s}\n", equal ? "true" : "false");
            ok = write_case_file(name + L"-refresh-output.bin", after) && equal && ok;
        }
        std::printf("{\"operation\":\"end_meter_case\",\"case\":\"%s\",\"passed\":%s}\n", test.name, ok ? "true" : "false");
        allOk = allOk && ok;
    }
    properties->Release(); edit->Release(); return allOk;
}

static bool read_copy_data(IUnknown* data, UINT format, std::vector<unsigned char>& bytes) {
    IStream* stream = nullptr;
    const HRESULT hr = ReferenceTimeline::data_stream(data, format, &stream);
    result("copy_get_stream", hr);
    if (FAILED(hr) || !stream) return false;
    STATSTG stat{};
    bool ok = SUCCEEDED(stream->Stat(&stat, STATFLAG_NONAME)) && !stat.cbSize.HighPart && stat.cbSize.LowPart <= 1024 * 1024;
    if (ok) {
        bytes.resize(stat.cbSize.LowPart);
        LARGE_INTEGER zero{}; ULONG count = 0;
        ok = SUCCEEDED(stream->Seek(zero, STREAM_SEEK_SET, nullptr)) &&
            SUCCEEDED(stream->Read(bytes.data(), static_cast<ULONG>(bytes.size()), &count)) && count == bytes.size();
    }
    stream->Release();
    if (!ok || bytes.size() < 12 || std::memcmp(bytes.data(), "tetr", 4) || (bytes.size() - 12) % 24) return false;
    DWORD recordSize = 0, payload = 0;
    std::memcpy(&recordSize, bytes.data() + 8, 4); std::memcpy(&payload, bytes.data() + 4, 4);
    if (recordSize != 24 || payload != bytes.size() - 8) return false;
    for (size_t offset = 12; offset < bytes.size(); offset += 24) {
        LONG time = 0, tick = 0; double tempo = 0;
        std::memcpy(&time, bytes.data() + offset, 4);
        std::memcpy(&tempo, bytes.data() + offset + 8, 8);
        std::memcpy(&tick, bytes.data() + offset + 16, 4);
        std::printf("{\"operation\":\"copy_record\",\"index\":%zu,\"relative_time\":%ld,\"tempo\":%.17g,\"tick\":%ld}\n",
            (offset - 12) / 24, time, tempo, tick);
    }
    return true;
}

static bool probe_copy_events(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    struct Event { LONG time; double tempo; };
    struct Case { const char* name; bool select; LONG start; std::vector<Event> events; };
    const Case cases[] = {
        {"unselected", false, 900, {{900,137},{2304,150}}},
        {"single", true, 900, {{900,137}}},
        {"multiple", true, 900, {{900,137},{2304,150},{4708,93.75}}},
        {"early_boundary", true, 0, {{900,137},{2304,150},{4708,93.75}}},
    };
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip); result("copy_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit)); strip->Release();
    if (FAILED(hr) || !edit) return false;
    const UINT format = RegisterClipboardFormatA("Jazz v.1 Tempolist");
    if (!format) { edit->Release(); return false; }
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    bool allOk = true;
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_copy_case\",\"case\":\"%s\",\"start\":%ld}\n", test.name, test.start);
        std::wstring name = parent + L"copy-";
        for (const char* letter = test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        std::vector<unsigned char> input(12 + test.events.size() * 16, 0);
        std::memcpy(input.data(), "tetr", 4);
        const DWORD payload = static_cast<DWORD>(input.size() - 8);
        std::memcpy(input.data() + 4, &payload, 4); input[8] = 16;
        for (size_t i = 0; i < test.events.size(); ++i) {
            std::memcpy(input.data() + 12 + i * 16, &test.events[i].time, 4);
            std::memcpy(input.data() + 20 + i * 16, &test.events[i].tempo, 8);
        }
        bool ok = write_case_file(name + L"-input.bin", input);
        hr = load_bytes(persist, input); result("copy_load_input", hr); ok = hr == S_OK && ok;
        if (test.select) { hr = reference_edit::select_all(edit); result("copy_select_all", hr); ok = hr == S_OK && ok; }
        result("can_copy_before", reference_edit::can_copy(edit));
        result("can_cut_before", reference_edit::can_cut(edit));
        for (bool cut : {false, true}) {
            IUnknown* data = nullptr;
            hr = timeline.create_data_object(&data); result("copy_create_data_object", hr);
            if (FAILED(hr) || !data) { ok = false; break; }
            hr = ReferenceTimeline::set_data_boundaries(data, test.start, 6000); result("copy_set_boundaries", hr); ok = hr == S_OK && ok;
            result("can_paste_empty_data", reference_edit::can_paste(edit, data));
            hr = cut ? reference_edit::cut(edit, data) : reference_edit::copy(edit, data);
            result(cut ? "cut_to_data_object" : "copy_to_data_object", hr);
            ok = observe_undo_label(manager, cut ? "cut" : "copy") && ok;
            ok = hr == (test.select ? S_OK : E_UNEXPECTED) && ok;
            const HRESULT available = ReferenceTimeline::data_format_available(data, format);
            result("copy_format_available", available);
            ok = available == (test.select ? S_OK : S_FALSE) && ok;
            result("can_paste_copied_data", reference_edit::can_paste(edit, data));
            std::vector<unsigned char> copied;
            if (test.select && available == S_OK) ok = read_copy_data(data, format, copied) && ok;
            ok = write_case_file(name + (cut ? L"-cut-clipboard.bin" : L"-clipboard.bin"), copied) && ok;
            data->Release();
            std::vector<unsigned char> state;
            ok = saved_bytes(persist, state) && write_case_file(name + (cut ? L"-cut-output.bin" : L"-output.bin"), state) && ok;
            const bool expected = cut && test.select ? state.size() == 12 : state == input;
            std::printf("{\"operation\":\"copy_source_state\",\"cut\":%s,\"expected\":%s}\n", cut ? "true" : "false", expected ? "true" : "false");
            ok = expected && ok;
            result("can_copy_after", reference_edit::can_copy(edit));
            result("can_cut_after", reference_edit::can_cut(edit));
            if (cut && test.select) {
                LONG next = -1; runtime_tempo::TempoParam empty{};
                hr = runtime_tempo::get(runtime, 0, &next, &empty); result("copy_cut_runtime_empty", hr);
                ok = FAILED(hr) && ok;
            } else {
                const auto& first = test.events.front();
                ok = check_editor_tempo(manager, &timeline, first.time, first.time, first.tempo) && ok;
                ok = check_runtime_tempo(runtime, first.tempo, first.time) && ok;
            }
        }
        std::printf("{\"operation\":\"end_copy_case\",\"case\":\"%s\",\"passed\":%s}\n", test.name, ok ? "true" : "false");
        allOk = ok && allOk;
    }
    edit->Release(); return allOk;
}

static bool probe_paste_events(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    const HRESULT connected = timeline.connect_time_strip(); result("paste_connect_time_strip", connected);
    if (connected != S_OK) return false;
    struct Event { LONG time; double tempo; };
    struct Case { const char* name; LONG at; DWORD mode; LONG copyStart; LONG length; std::vector<Event> target; };
    const Case cases[] = {
        {"merge_empty",900,0,900,30720,{}},
        {"snap_existing",900,0,900,30720,{{0,80},{900,90},{1000,100}}},
        {"merge_duplicates",0,0,900,30720,{{0,80},{132,90},{132,95},{1000,100},{1536,101},{3940,105}}},
        {"overwrite",0,1,900,30720,{{0,80},{132,90},{1000,100},{5100,125},{5101,126},{6000,120}}},
        {"near_end",30000,0,900,30720,{{0,80}}},
        {"before_start",0,0,3072,30720,{{0,80}}},
        {"short_target",900,0,900,1000,{{0,80}}},
    };
    const auto encode = [](const std::vector<Event>& events) {
        std::vector<unsigned char> bytes(12 + events.size() * 16, 0);
        std::memcpy(bytes.data(), "tetr", 4);
        const DWORD size = static_cast<DWORD>(bytes.size() - 8);
        std::memcpy(bytes.data() + 4, &size, 4); bytes[8] = 16;
        for (size_t i = 0; i < events.size(); ++i) {
            std::memcpy(bytes.data() + 12 + i * 16, &events[i].time, 4);
            std::memcpy(bytes.data() + 20 + i * 16, &events[i].tempo, 8);
        }
        return bytes;
    };
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip); result("paste_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit)); strip->Release();
    if (FAILED(hr) || !edit) return false;
    IUnknown* properties = nullptr;
    hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject, reinterpret_cast<void**>(&properties));
    if (FAILED(hr) || !properties) { edit->Release(); return false; }
    const UINT format = RegisterClipboardFormatA("Jazz v.1 Tempolist");
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    bool allOk = true;
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_paste_case\",\"case\":\"%s\",\"at\":%ld,\"mode\":%lu,\"copy_start\":%ld,\"length\":%ld}\n",
            test.name, test.at, test.mode, test.copyStart, test.length);
        std::wstring name = parent + L"paste-";
        for (const char* letter = test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        const auto source = encode({{900,137},{2304,150},{4708,93.75}});
        const auto target = encode(test.target);
        bool ok = write_case_file(name + L"-source.bin", source) && write_case_file(name + L"-target.bin", target);
        hr = timeline.set_length(30720); result("paste_source_length", hr); ok = hr == S_OK && ok;
        hr = load_bytes(persist, source); result("paste_load_source", hr); ok = hr == S_OK && ok;
        hr = reference_edit::select_all(edit); result("paste_select_source", hr); ok = hr == S_OK && ok;
        IUnknown* data = nullptr;
        hr = timeline.create_data_object(&data); result("paste_create_data", hr);
        if (FAILED(hr) || !data) { allOk = false; break; }
        hr = ReferenceTimeline::set_data_boundaries(data, test.copyStart, 6000); result("paste_set_source_boundaries", hr); ok = hr == S_OK && ok;
        hr = reference_edit::copy(edit, data); result("paste_copy_source", hr); ok = hr == S_OK && ok;
        // GetInternalClipFormat moves the format into a consumed list. Observe
        // a separate copy so that the object passed to Paste is still available.
        IUnknown* inspection = nullptr;
        hr = timeline.create_data_object(&inspection); result("paste_create_inspection_data", hr);
        if (FAILED(hr) || !inspection) { data->Release(); allOk = false; break; }
        hr = ReferenceTimeline::set_data_boundaries(inspection, test.copyStart, 6000); result("paste_inspection_boundaries", hr); ok = hr == S_OK && ok;
        hr = reference_edit::copy(edit, inspection); result("paste_inspection_copy", hr); ok = hr == S_OK && ok;
        std::vector<unsigned char> copied;
        ok = read_copy_data(inspection, format, copied) && write_case_file(name + L"-clipboard.bin", copied) && ok;
        result("paste_format_after_inspection", ReferenceTimeline::data_format_available(inspection, format));
        inspection->Release();
        result("paste_format_before_execution", ReferenceTimeline::data_format_available(data, format));
        hr = timeline.set_length(test.length); result("paste_target_length", hr); ok = hr == S_OK && ok;
        hr = load_bytes(persist, target); result("paste_load_target", hr); ok = hr == S_OK && ok;
        hr = reference_edit::select_all(edit); result("paste_select_old_events", hr); ok = hr == S_OK && ok;
        hr = timeline.set_paste_mode(test.mode); result("paste_set_mode", hr); ok = hr == S_OK && ok;
        hr = timeline.set_marker(0, test.at); result("paste_set_cursor", hr); ok = hr == S_OK && ok;
        LONG actualCursor = -1;
        hr = timeline.get_marker(0, &actualCursor); result("paste_get_cursor", hr);
        std::printf("{\"operation\":\"paste_cursor\",\"requested\":%ld,\"actual\":%ld}\n", test.at, actualCursor);
        ok = hr == S_OK && actualCursor == test.at && ok;
        hr = reference_edit::paste(edit, data); result("paste_events", hr); ok = hr == S_OK && ok;
        data->Release();
        ok = observe_undo_label(manager, "paste") && ok;
        void* borrowed = nullptr;
        hr = reference_edit::get_data(properties, &borrowed); result("paste_get_selected", hr); ok = hr == S_OK && borrowed && ok;
        if (borrowed) { alignas(8) unsigned char value[0x22]{}; std::memcpy(value, borrowed, sizeof(value)); position_data("pasted", test.at, value); }
        std::vector<unsigned char> output;
        ok = saved_bytes(persist, output) && write_case_file(name + L"-output.bin", output) && ok;
        if (output.size() < 12 || (output.size() - 12) % 16) ok = false;
        if (ok) for (size_t offset = 12; offset < output.size(); offset += 16) {
            Event event{}; std::memcpy(&event.time, output.data() + offset, 4); std::memcpy(&event.tempo, output.data() + offset + 8, 8);
            std::printf("{\"operation\":\"paste_saved_event\",\"index\":%zu,\"time\":%ld,\"tempo\":%.17g}\n", (offset-12)/16, event.time, event.tempo);
            LONG next = event.time + 1;
            if (offset + 16 < output.size()) std::memcpy(&next, output.data() + offset + 16, 4);
            if (next != event.time) {
                ok = check_editor_tempo(manager, &timeline, event.time, event.time, event.tempo) && ok;
                ok = check_runtime_tempo(runtime, event.tempo, event.time) && ok;
            }
        }
        hr = load_bytes(persist, output); result("paste_reload_output", hr); ok = hr == S_OK && ok;
        std::vector<unsigned char> after;
        ok = saved_bytes(persist, after) && write_case_file(name + L"-reload-output.bin", after) && after == output && ok;
        std::printf("{\"operation\":\"end_paste_case\",\"case\":\"%s\",\"passed\":%s}\n", test.name, ok ? "true" : "false");
        allOk = ok && allOk;
    }
    hr = timeline.set_length(30720); result("paste_restore_length", hr); allOk = hr == S_OK && allOk;
    hr = timeline.disconnect_time_strip(); result("paste_disconnect_time_strip", hr); allOk = hr == S_OK && allOk;
    properties->Release(); edit->Release(); return allOk;
}

static bool probe_tempo_notification(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    const GUID notification = {0x1528eab8,0xc518,0x11d2,{0xb0,0xe7,0,0x10,0x5a,0x26,0x62,0x0b}};
    HRESULT hr = timeline.connect_time_strip(); result("tempo_notification_connect_time_strip", hr);
    if (FAILED(hr)) return false;
    hr = manager->OnUpdate(notification, 0xffffffff, nullptr); result("tempo_notification_null", hr);
    bool allOk = hr == E_POINTER;
    struct Event { LONG time; double tempo; };
    struct Case {
        const char* name; LONG cursor; double tempo; bool selected; bool property12; bool throughTimeline;
        std::vector<Event> events;
    };
    const std::vector<Case> cases{
        {"empty", 0, 164.25, false, false, false, {}},
        {"before_first", 0, 164.25, false, false, false, {{100,137}}},
        {"first", 0, 164.25, false, false, false, {{0,120},{900,137},{2304,150}}},
        {"between", 1000, 164.25, false, false, false, {{0,120},{900,137},{2304,150}}},
        {"duplicates", 900, 164.25, false, false, false, {{0,120},{900,137},{900,142},{2304,150}}},
        {"after_last", 30000, 164.25, true, true, false, {{0,120},{900,137},{2304,150}}},
        {"unchanged", 900, 137, true, true, false, {{0,120},{900,137},{2304,150}}},
        {"timeline_delivery", 900, 171.5, true, true, true, {{0,120},{900,137},{2304,150}}},
    };
    const auto encode = [](const std::vector<Event>& events) {
        std::vector<unsigned char> bytes(12 + events.size() * 16, 0);
        std::memcpy(bytes.data(), "tetr", 4);
        const DWORD size = static_cast<DWORD>(bytes.size() - 8);
        std::memcpy(bytes.data() + 4, &size, 4); bytes[8] = 16;
        for (size_t i = 0; i < events.size(); ++i) {
            std::memcpy(bytes.data() + 12 + i * 16, &events[i].time, 4);
            std::memcpy(bytes.data() + 20 + i * 16, &events[i].tempo, 8);
        }
        return bytes;
    };
    IUnknown* strip = nullptr;
    hr = timeline.find_manager_strip(manager, &strip); result("tempo_notification_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit)); strip->Release();
    if (FAILED(hr) || !edit) return false;
    IUnknown* properties = nullptr;
    hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject, reinterpret_cast<void**>(&properties));
    if (FAILED(hr) || !properties) { edit->Release(); return false; }
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_tempo_notification_case\",\"case\":\"%s\",\"cursor\":%ld,\"tempo\":%.17g,\"selected\":%s,\"property12\":%s,\"through_timeline\":%s}\n",
            test.name, test.cursor, test.tempo, test.selected ? "true" : "false", test.property12 ? "true" : "false", test.throughTimeline ? "true" : "false");
        std::wstring name = parent + L"tempo-notification-";
        for (const char* letter = test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        const auto input = encode(test.events);
        bool ok = write_case_file(name + L"-input.bin", input);
        hr = load_bytes(persist, input); result("tempo_notification_load", hr); ok = hr == S_OK && ok;
        if (test.selected) { hr = reference_edit::select_all(edit); result("tempo_notification_select", hr); ok = hr == S_OK && ok; }
        hr = timeline.set_marker(0, test.cursor); result("tempo_notification_set_cursor", hr); ok = hr == S_OK && ok;
        LONG cursor = -1;
        hr = timeline.get_marker(0, &cursor); result("tempo_notification_get_cursor", hr);
        std::printf("{\"operation\":\"tempo_notification_cursor\",\"requested\":%ld,\"actual\":%ld}\n", test.cursor, cursor);
        ok = hr == S_OK && cursor == test.cursor && ok;
        hr = timeline.set_boolean_property(12, test.property12); result("tempo_notification_set_property12", hr); ok = hr == S_OK && ok;
        ok = observe_undo_label(manager, "before_tempo_notification") && ok;
        runtime_tempo::TempoParam value{}; value.time = -1234567; value.tempo = test.tempo;
        hr = test.throughTimeline ? timeline.notify(notification, 0xffffffff, &value) : manager->OnUpdate(notification, 0xffffffff, &value);
        result("tempo_notification_update", hr); ok = hr == S_OK && ok;
        ok = observe_undo_label(manager, "after_tempo_notification") && ok;
        VARIANT flag{};
        hr = timeline.get_property(12, &flag); result("tempo_notification_get_property12", hr);
        std::printf("{\"operation\":\"tempo_notification_property12\",\"type\":%u,\"value\":%d}\n", flag.vt, flag.boolVal);
        ok = hr == S_OK && flag.vt == VT_BOOL && flag.boolVal == (test.property12 ? 1 : 0) && ok;
        void* borrowed = nullptr;
        hr = reference_edit::get_data(properties, &borrowed); result("tempo_notification_get_selected", hr);
        std::printf("{\"operation\":\"tempo_notification_selection\",\"present\":%s}\n", borrowed ? "true" : "false");
        ok = hr == S_OK && (borrowed != nullptr) == test.selected && ok;
        if (borrowed) { alignas(8) unsigned char selected[0x22]{}; std::memcpy(selected, borrowed, sizeof(selected)); position_data("tempo_notification", cursor, selected); }
        std::vector<unsigned char> output;
        ok = saved_bytes(persist, output) && write_case_file(name + L"-output.bin", output) && ok;
        if (output.size() < 12 || (output.size() - 12) % 16) ok = false;
        if (ok) for (size_t offset = 12; offset < output.size(); offset += 16) {
            Event event{}; std::memcpy(&event.time, output.data() + offset, 4); std::memcpy(&event.tempo, output.data() + offset + 8, 8);
            std::printf("{\"operation\":\"tempo_notification_saved_event\",\"index\":%zu,\"time\":%ld,\"tempo\":%.17g}\n", (offset-12)/16, event.time, event.tempo);
            LONG next = event.time + 1;
            if (offset + 16 < output.size()) std::memcpy(&next, output.data() + offset + 16, 4);
            if (next != event.time) {
                ok = check_editor_tempo(manager, &timeline, event.time, event.time, event.tempo) && ok;
                ok = check_runtime_tempo(runtime, event.tempo, event.time) && ok;
            }
        }
        hr = load_bytes(persist, output); result("tempo_notification_reload", hr); ok = hr == S_OK && ok;
        std::vector<unsigned char> after;
        ok = saved_bytes(persist, after) && write_case_file(name + L"-reload-output.bin", after) && after == output && ok;
        std::printf("{\"operation\":\"end_tempo_notification_case\",\"case\":\"%s\",\"passed\":%s}\n", test.name, ok ? "true" : "false");
        allOk = ok && allOk;
    }
    hr = timeline.set_boolean_property(12, false); result("tempo_notification_restore_property12", hr); allOk = hr == S_OK && allOk;
    hr = timeline.disconnect_time_strip(); result("tempo_notification_disconnect_time_strip", hr); allOk = hr == S_OK && allOk;
    properties->Release(); edit->Release(); return allOk;
}

static bool probe_range_selection(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    struct Case { const char* name; LONG start; LONG end; bool gutter; bool clear; };
    const Case cases[] = {
        {"same_beat",900,1000,true,false}, {"end_beat",900,1536,true,false},
        {"across_measures",3000,5100,true,false}, {"equal",900,900,true,false},
        {"disabled",0,3072,false,false}, {"reversed",4708,900,true,false},
        {"start_minus_one",-1,768,true,false}, {"cleared",0,3072,true,true},
    };
    const LONG times[] = {0,768,900,1000,1535,1536,2304,3072,4708,5000,6144};
    std::vector<unsigned char> input(12 + std::size(times) * 16, 0);
    std::memcpy(input.data(), "tetr", 4);
    const DWORD size = static_cast<DWORD>(input.size() - 8);
    std::memcpy(input.data() + 4, &size, 4); input[8] = 16;
    for (size_t i = 0; i < std::size(times); ++i) {
        const double tempo = 100.0 + i;
        std::memcpy(input.data() + 12 + i * 16, &times[i], 4);
        std::memcpy(input.data() + 20 + i * 16, &tempo, 8);
    }
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip); result("range_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit));
    if (FAILED(hr) || !edit) { strip->Release(); return false; }
    IUnknown* properties = nullptr;
    hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject, reinterpret_cast<void**>(&properties));
    if (FAILED(hr) || !properties) { edit->Release(); strip->Release(); return false; }
    bool allOk = true;
    VARIANT bad{};
    hr = reference_edit::set_strip_property(strip, 3, bad); result("range_wrong_type", hr); allOk = hr == E_FAIL && allOk;
    hr = reference_edit::set_strip_property(strip, 99, bad); result("range_unknown_property", hr); allOk = hr == E_FAIL && allOk;
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    const UINT format = RegisterClipboardFormatA("Jazz v.1 Tempolist");
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_range_case\",\"case\":\"%s\",\"start\":%ld,\"end\":%ld,\"gutter\":%s,\"clear\":%s}\n",
            test.name, test.start, test.end, test.gutter ? "true" : "false", test.clear ? "true" : "false");
        std::wstring name = parent + L"range-";
        for (const char* letter = test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        bool ok = write_case_file(name + L"-input.bin", input);
        hr = load_bytes(persist, input); result("range_load", hr); ok = hr == S_OK && ok;
        hr = reference_edit::select_all(edit); result("range_select_old_events", hr); ok = hr == S_OK && ok;
        VARIANT value{}; value.vt = VT_I4; value.lVal = test.start;
        hr = reference_edit::set_strip_property(strip, 3, value); result("range_set_start", hr); ok = hr == S_OK && ok;
        value.lVal = test.end;
        hr = reference_edit::set_strip_property(strip, 4, value); result("range_set_end", hr); ok = hr == S_OK && ok;
        value.vt = VT_BOOL; value.boolVal = test.gutter ? 1 : 0;
        hr = reference_edit::set_strip_property(strip, 2, value); result("range_set_gutter", hr); ok = hr == S_OK && ok;
        if (test.clear) {
            value.boolVal = 0;
            hr = reference_edit::set_strip_property(strip, 2, value); result("range_clear_gutter", hr); ok = hr == S_OK && ok;
        }
        const HRESULT copyable = reference_edit::can_copy(edit); result("range_can_copy", copyable);
        const HRESULT deletable = reference_edit::can_delete(edit); result("range_can_delete", deletable);
        ok = (copyable == S_OK || copyable == S_FALSE) && deletable == copyable && ok;
        void* borrowed = nullptr;
        hr = reference_edit::get_data(properties, &borrowed); result("range_get_selected", hr);
        std::printf("{\"operation\":\"range_selection\",\"present\":%s}\n", borrowed ? "true" : "false");
        ok = hr == S_OK && (borrowed != nullptr) == (copyable == S_OK) && ok;
        if (borrowed) { alignas(8) unsigned char selected[0x22]{}; std::memcpy(selected, borrowed, sizeof(selected)); position_data("range", test.start, selected); }
        std::vector<unsigned char> before;
        ok = saved_bytes(persist, before) && write_case_file(name + L"-selected-output.bin", before) && before == input && ok;
        IUnknown* data = nullptr;
        hr = timeline.create_data_object(&data); result("range_create_data", hr);
        if (FAILED(hr) || !data) { allOk = false; break; }
        hr = ReferenceTimeline::set_data_boundaries(data, 0, 7000); result("range_set_copy_boundaries", hr); ok = hr == S_OK && ok;
        hr = reference_edit::copy(edit, data); result("range_copy", hr);
        ok = hr == (copyable == S_OK ? S_OK : E_UNEXPECTED) && ok;
        std::vector<unsigned char> copied;
        if (copyable == S_OK) ok = read_copy_data(data, format, copied) && ok;
        ok = write_case_file(name + L"-clipboard.bin", copied) && ok;
        data->Release();
        hr = reference_edit::delete_selected(edit); result("range_delete", hr); ok = hr == S_OK && ok;
        std::vector<unsigned char> output;
        ok = saved_bytes(persist, output) && write_case_file(name + L"-output.bin", output) && ok;
        if (output.size() < 12 || (output.size() - 12) % 16) ok = false;
        if (ok) for (size_t offset = 12; offset < output.size(); offset += 16) {
            LONG time = 0; double tempo = 0;
            std::memcpy(&time, output.data() + offset, 4); std::memcpy(&tempo, output.data() + offset + 8, 8);
            std::printf("{\"operation\":\"range_saved_event\",\"index\":%zu,\"time\":%ld,\"tempo\":%.17g}\n", (offset-12)/16, time, tempo);
            ok = check_editor_tempo(manager, &timeline, time, time, tempo) && check_runtime_tempo(runtime, tempo, time) && ok;
        }
        hr = load_bytes(persist, output); result("range_reload", hr); ok = hr == S_OK && ok;
        std::vector<unsigned char> after;
        ok = saved_bytes(persist, after) && write_case_file(name + L"-reload-output.bin", after) && after == output && ok;
        std::printf("{\"operation\":\"end_range_case\",\"case\":\"%s\",\"passed\":%s}\n", test.name, ok ? "true" : "false");
        allOk = ok && allOk;
    }
    VARIANT reset{}; reset.vt = VT_BOOL;
    hr = reference_edit::set_strip_property(strip, 2, reset); result("range_reset_gutter", hr); allOk = hr == S_OK && allOk;
    properties->Release(); edit->Release(); strip->Release(); return allOk;
}

#ifdef PRODUCER_WINDOWED_PROBE
static bool probe_mouse_selection(IPersistStream* persist, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    struct Event { LONG time; double tempo; };
    struct Step { LONG x; WPARAM modifiers; };
    struct Case { const char* name; bool selectAll; std::vector<Event> events; std::vector<Step> steps; bool selectAfter = false; };
    const std::vector<Event> normal{{0,120},{900,137},{2304,150},{4708,93.75}};
    const Case cases[] = {
        {"first",false,normal,{{10,0}}},
        {"second",false,normal,{{100,0}}},
        {"beat_end",false,normal,{{191,0}}},
        {"replace",false,normal,{{10,0},{300,0}}},
        {"control_add",false,normal,{{10,0},{100,MK_CONTROL}}},
        {"shift_forward",false,normal,{{10,0},{300,MK_SHIFT}}},
        {"shift_reverse",false,normal,{{300,0},{10,MK_SHIFT}}},
        {"same_beat",false,{{0,120},{100,130},{300,140},{900,137}},{{10,0}}},
        {"empty_beat",false,normal,{{200,0}}},
        {"collapse",true,normal,{{100,0}}},
        {"empty_track",false,{},{{10,0}}},
        {"shift_from_empty",false,normal,{{200,0},{300,MK_SHIFT}}},
        {"empty_then_select_all",false,normal,{{200,0}},true},
    };
    IUnknown* strip = nullptr; IUnknown* edit = nullptr; IUnknown* properties = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager,&strip); result("mouse_find_strip",hr);
    if (SUCCEEDED(hr)) hr = strip->QueryInterface(reference_edit::TimelineEditIid,reinterpret_cast<void**>(&edit));
    if (SUCCEEDED(hr)) hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject,reinterpret_cast<void**>(&properties));
    if (FAILED(hr)) { if (edit) edit->Release(); if (strip) strip->Release(); return false; }
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\")+1);
    bool allOk = true;
    timeline.set_zoom(.125); timeline.set_horizontal_scroll(0);
    using Message = HRESULT (STDMETHODCALLTYPE*)(IUnknown*,UINT,WPARAM,LPARAM,LONG,LONG);
    const auto message = reinterpret_cast<Message>((*reinterpret_cast<void***>(strip))[6]);
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_mouse_case\",\"case\":\"%s\"}\n",test.name);
        std::wstring name = parent+L"mouse-";
        for (const char* letter=test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        VARIANT gutter{}; gutter.vt = VT_BOOL;
        hr = reference_edit::set_strip_property(strip,2,gutter); bool ok = hr == S_OK;
        std::vector<unsigned char> input(12+test.events.size()*16,0);
        std::memcpy(input.data(),"tetr",4);
        const DWORD payload=static_cast<DWORD>(input.size()-8), recordSize=16;
        std::memcpy(input.data()+4,&payload,4); std::memcpy(input.data()+8,&recordSize,4);
        for (size_t i=0;i<test.events.size();++i) {
            std::memcpy(input.data()+12+i*16,&test.events[i].time,4);
            std::memcpy(input.data()+20+i*16,&test.events[i].tempo,8);
        }
        ok = write_case_file(name+L"-input.bin",input) && ok;
        hr = load_bytes(persist,input); result("mouse_load",hr); ok = hr == S_OK && ok;
        if (test.selectAll) { hr=reference_edit::select_all(edit); result("mouse_select_all",hr); ok=hr==S_OK&&ok; }
        for (const auto& step : test.steps) {
            for (const UINT kind : {WM_LBUTTONDOWN,WM_LBUTTONUP}) {
                const WPARAM keys=step.modifiers | (kind==WM_LBUTTONDOWN ? MK_LBUTTON : 0);
                std::printf("{\"operation\":\"mouse_message\",\"message\":%u,\"keys\":%zu,\"x\":%ld}\n",kind,static_cast<size_t>(keys),step.x);
                hr=message(strip,kind,keys,0,step.x,0); result("mouse_dispatch",hr); ok=hr==S_OK&&ok;
                void* borrowed=nullptr;
                hr=reference_edit::get_data(properties,&borrowed); result("mouse_get_data",hr); ok=hr==S_OK&&ok;
                std::printf("{\"operation\":\"mouse_selection_present\",\"present\":%s}\n",borrowed?"true":"false");
                if(borrowed) { alignas(8) unsigned char data[0x22]{}; std::memcpy(data,borrowed,sizeof(data)); position_data(kind==WM_LBUTTONDOWN?"mouse_down":"mouse_up",step.x,data); }
            }
        }
        if(test.selectAfter) { hr=reference_edit::select_all(edit);result("mouse_select_all_after_empty",hr);ok=hr==S_OK&&ok; }
        const HRESULT canCopy=reference_edit::can_copy(edit); result("mouse_can_copy",canCopy);
        result("mouse_can_delete",reference_edit::can_delete(edit));
        result("mouse_can_select_all",reference_edit::can_select_all(edit));
        std::vector<unsigned char> copied;
        if(canCopy==S_OK) {
            IUnknown* data=nullptr;
            hr=timeline.create_data_object(&data); result("mouse_create_copy",hr); ok=hr==S_OK&&ok;
            if(data) {
                hr=ReferenceTimeline::set_data_boundaries(data,0,30720); result("mouse_copy_boundaries",hr); ok=hr==S_OK&&ok;
                hr=reference_edit::copy(edit,data); result("mouse_copy",hr); ok=hr==S_OK&&ok;
                ok=read_copy_data(data,RegisterClipboardFormatA("Jazz v.1 Tempolist"),copied)&&ok;
                data->Release();
            }
        } else ok=canCopy==S_FALSE&&ok;
        ok=write_case_file(name+L"-copy.bin",copied)&&ok;
        std::vector<unsigned char> first;
        for(unsigned repeat=0;repeat<2;++repeat) {
            DrawingSurface surface(0,640);
            if(!surface.valid()){ok=false;break;}
            using Draw=HRESULT (STDMETHODCALLTYPE*)(IUnknown*,HDC,DWORD,LONG);
            hr=reinterpret_cast<Draw>((*reinterpret_cast<void***>(strip))[3])(strip,surface.dc(),0,0);
            result("mouse_draw",hr);ok=hr==S_OK&&ok;
            const auto bitmap=surface.bitmap_bytes();
            ok=write_case_file(name+(repeat?L"-repeat.bmp":L"-output.bmp"),bitmap)&&ok;
            if(repeat)ok=bitmap==first&&ok;else first=bitmap;
        }
        std::vector<unsigned char> output;
        ok=saved_bytes(persist,output)&&write_case_file(name+L"-after.bin",output)&&ok;
        std::printf("{\"operation\":\"mouse_saved_unchanged\",\"same\":%s}\n",output==input?"true":"false");
        ok=output==input&&ok;
        if(std::string(test.name)=="empty_beat" || std::string(test.name)=="shift_from_empty") {
            const bool insertion=std::string(test.name)=="empty_beat";
            if(insertion) { hr=reference_edit::insert(edit);result("mouse_insert_after_empty_selection",hr);ok=hr==S_OK&&ok; }
            else {
                void* borrowed=nullptr;
                hr=reference_edit::get_data(properties,&borrowed);ok=hr==S_OK&&borrowed&&ok;
                if(borrowed) {
                    alignas(8) unsigned char changed[0x22]{};std::memcpy(changed,borrowed,sizeof(changed));
                    const double tempo=171.5;std::memcpy(changed+8,&tempo,8);
                    hr=reference_edit::set_data(properties,changed);result("mouse_edit_after_empty_selection",hr);ok=hr==S_OK&&ok;
                }
            }
            std::vector<unsigned char> edited,reloaded;
            ok=saved_bytes(persist,edited)&&write_case_file(name+L"-edited.bin",edited)&&ok;
            hr=load_bytes(persist,edited);result("mouse_reload_edited",hr);ok=hr==S_OK&&ok;
            ok=saved_bytes(persist,reloaded)&&write_case_file(name+L"-reload.bin",reloaded)&&edited==reloaded&&ok;
        }
        std::printf("{\"operation\":\"end_mouse_case\",\"case\":\"%s\",\"passed\":%s}\n",test.name,ok?"true":"false");
        allOk=ok&&allOk;
    }
    VARIANT gutter{};gutter.vt=VT_BOOL;
    hr=reference_edit::set_strip_property(strip,2,gutter); result("mouse_reset_selection",hr);allOk=hr==S_OK&&allOk;
    properties->Release();edit->Release();strip->Release();return allOk;
}
static bool probe_commands(IPersistStream* persist,IUnknown* runtime,producer::StripManager* manager,
    ReferenceTimeline& timeline,IUnknown* strip,IUnknown* edit,const wchar_t* initialPath){
    using Message=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,UINT,WPARAM,LPARAM,LONG,LONG);
    const auto message=reinterpret_cast<Message>((*reinterpret_cast<void***>(strip))[6]);
    IUnknown* properties=nullptr;
    auto hr=manager->QueryInterface(producer::IID_IDMUSProdPropPageObject,reinterpret_cast<void**>(&properties));
    if(FAILED(hr))return false;
    std::wstring parent(initialPath);parent.resize(parent.find_last_of(L"/\\")+1);bool allOk=true;
    for(const auto& test:command_probe::cases){
        std::printf("{\"operation\":\"begin_command_case\",\"case\":\"%s\",\"command\":%lu,\"position\":%ld,\"all\":%s}\n",test.name,static_cast<DWORD>(test.command),test.position,test.all?"true":"false");
        std::wstring name=parent+L"command-";for(const char* c=test.name;*c;++c)name+=static_cast<wchar_t>(*c);
        bool ok=timeline.set_zoom(.125)==S_OK&&timeline.set_horizontal_scroll(0)==S_OK;
        std::vector<unsigned char> input(76,0);std::memcpy(input.data(),"tetr",4);const DWORD payload=68,size=16;
        std::memcpy(input.data()+4,&payload,4);std::memcpy(input.data()+8,&size,4);
        const LONG times[]={0,900,2304,4708};const double tempos[]={120,137,150,93.75};
        for(size_t i=0;i<4;++i){std::memcpy(input.data()+12+i*16,&times[i],4);std::memcpy(input.data()+20+i*16,&tempos[i],8);}
        ok=write_case_file(name+L"-input.bin",input)&&load_bytes(persist,input)==S_OK&&ok;
        hr=message(strip,WM_LBUTTONDOWN,MK_LBUTTON,0,test.position,0);result("command_mouse_down",hr);ok=hr==S_OK&&ok;
        hr=message(strip,WM_LBUTTONUP,0,0,test.position,0);result("command_mouse_up",hr);ok=hr==S_OK&&ok;
        if(test.all){hr=reference_edit::select_all(edit);result("command_prepare_all",hr);ok=hr==S_OK&&ok;}
        // Deliberately unrelated coordinates: WM_COMMAND must use the retained
        // insertion position, and decode the command from LOWORD(wParam).
        hr=message(strip,WM_COMMAND,test.command,0,999,999);result("command_execute",hr);
        ok=observe_undo_label(manager,"command")&&ok;
        void* data=nullptr;hr=reference_edit::get_data(properties,&data);result("command_get_selected",hr);ok=hr==S_OK&&ok;
        std::printf("{\"operation\":\"command_selection_present\",\"present\":%s}\n",data?"true":"false");
        if(data){alignas(8) unsigned char value[0x22]{};std::memcpy(value,data,sizeof(value));position_data("command",test.position,value);}
        const auto canCopy=reference_edit::can_copy(edit);result("command_can_copy",canCopy);
        std::vector<unsigned char> selected;
        if(canCopy==S_OK){IUnknown* copied=nullptr;hr=timeline.create_data_object(&copied);ok=hr==S_OK&&ok;
            if(copied){ok=ReferenceTimeline::set_data_boundaries(copied,0,30720)==S_OK&&ok;
                hr=reference_edit::copy(edit,copied);result("command_copy_selected",hr);ok=hr==S_OK&&ok;
                ok=read_copy_data(copied,RegisterClipboardFormatA("Jazz v.1 Tempolist"),selected)&&ok;copied->Release();}}
        else ok=canCopy==S_FALSE&&ok;
        ok=write_case_file(name+L"-selected-clipboard.bin",selected)&&ok;
        std::vector<unsigned char> output,reloaded;ok=saved_bytes(persist,output)&&write_case_file(name+L"-output.bin",output)&&ok;
        if(output.size()==12){LONG next=0;runtime_tempo::TempoParam value{};hr=runtime_tempo::get(runtime,0,&next,&value);result("command_runtime_empty",hr);ok=FAILED(hr)&&ok;}
        else for(size_t offset=12;offset+16<=output.size();offset+=16){LONG time=0;double tempo=0;
            std::memcpy(&time,output.data()+offset,4);std::memcpy(&tempo,output.data()+offset+8,8);
            ok=check_editor_tempo(manager,&timeline,time,time,tempo)&&check_runtime_tempo(runtime,tempo,time)&&ok;}
        hr=load_bytes(persist,output);result("command_reload",hr);ok=hr==S_OK&&ok;
        ok=saved_bytes(persist,reloaded)&&write_case_file(name+L"-reload.bin",reloaded)&&reloaded==output&&ok;
        std::printf("{\"operation\":\"end_command_case\",\"case\":\"%s\",\"passed\":%s}\n",test.name,ok?"true":"false");allOk=ok&&allOk;
    }
    properties->Release();return allOk;
}
static bool probe_system_clipboard(IPersistStream* persist, producer::StripManager* manager,
    ReferenceTimeline& timeline, IUnknown* strip, IUnknown* edit, const wchar_t* initialPath) {
    ClipboardSnapshot previous;
    const HRESULT previousResult = previous.capture();
    result("system_clipboard_snapshot", previousResult);
    if (FAILED(previousResult)) return false;
    if (GetClipboardSequenceNumber() != previous.sequence()) return false;
    DWORD ownedSequence = 0;
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    struct Case { const char* name; bool all, cut; DWORD mode; };
    const Case cases[] = {{"single",false,false,0},{"multiple",true,false,0},
        {"cut_multiple",true,true,0},{"overwrite_multiple",true,false,1}};
    bool allOk = true;
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_system_clipboard_case\",\"case\":\"%s\"}\n",test.name);
        std::wstring name=parent+L"system-clipboard-";for(const char* c=test.name;*c;++c)name+=static_cast<wchar_t>(*c);
        std::vector<unsigned char> input(76,0);std::memcpy(input.data(),"tetr",4);
        const DWORD payload=68,record=16;std::memcpy(input.data()+4,&payload,4);std::memcpy(input.data()+8,&record,4);
        const LONG times[]={0,900,2304,4708};const double tempos[]={120,137,150,93.75};
        for(size_t i=0;i<4;++i){std::memcpy(input.data()+12+i*16,&times[i],4);std::memcpy(input.data()+20+i*16,&tempos[i],8);}
        bool ok=write_case_file(name+L"-input.bin",input)&&load_bytes(persist,input)==S_OK;
        using Message=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,UINT,WPARAM,LPARAM,LONG,LONG);
        const auto message=reinterpret_cast<Message>((*reinterpret_cast<void***>(strip))[6]);
        auto hr=message(strip,WM_LBUTTONDOWN,MK_LBUTTON,0,100,0);result("system_clipboard_select_down",hr);ok=hr==S_OK&&ok;
        hr=message(strip,WM_LBUTTONUP,0,0,100,0);result("system_clipboard_select_up",hr);ok=hr==S_OK&&ok;
        if(test.all){hr=reference_edit::select_all(edit);result("system_clipboard_select_all",hr);ok=hr==S_OK&&ok;}
        const auto counterBefore=timeline.module_counter();
        hr=test.cut?reference_edit::cut(edit,nullptr):reference_edit::copy(edit,nullptr);
        result("system_clipboard_export",hr);ok=hr==S_OK&&ok;
        const auto counterAfter=timeline.module_counter();
        std::printf("{\"operation\":\"system_clipboard_module_counter\",\"before\":%ld,\"after\":%ld,\"delta\":%ld}\n",counterBefore,counterAfter,counterAfter-counterBefore);
        if(hr==S_OK)ownedSequence=GetClipboardSequenceNumber();
        IDataObject* exported=nullptr;hr=OleGetClipboard(&exported);result("system_clipboard_get",hr);ok=hr==S_OK&&exported&&ok;
        IUnknown* imported=nullptr;hr=timeline.create_data_object(&imported);ok=hr==S_OK&&imported&&ok;
        if(exported&&imported){
            hr=producer::timeline_data::import_data(imported,exported);result("system_clipboard_import",hr);ok=hr==S_OK&&ok;
            LONG start=-1,end=-1;hr=producer::timeline_data::get_boundaries(imported,&start,&end);
            result("system_clipboard_boundaries",hr);ok=hr==S_OK&&ok;
            std::printf("{\"operation\":\"system_clipboard_range\",\"start\":%ld,\"end\":%ld}\n",start,end);
            std::vector<unsigned char> bytes;ok=read_copy_data(imported,RegisterClipboardFormatA("Jazz v.1 Tempolist"),bytes)&&
                write_case_file(name+L"-clipboard.bin",bytes)&&ok;
        }
        if(imported)imported->Release();if(exported)exported->Release();
        hr=reference_edit::can_paste(edit,nullptr);result("system_clipboard_can_paste",hr);ok=hr==S_OK&&ok;
        std::vector<unsigned char> afterCopy;ok=saved_bytes(persist,afterCopy)&&write_case_file(name+L"-after-copy.bin",afterCopy)&&ok;
        if(!test.cut)ok=afterCopy==input&&ok;
        hr=timeline.set_paste_mode(test.mode);ok=hr==S_OK&&ok;
        hr=timeline.set_marker(0,6144);ok=hr==S_OK&&ok;
        hr=reference_edit::paste(edit,nullptr);result("system_clipboard_paste",hr);ok=hr==S_OK&&ok;
        ok=observe_undo_label(manager,"system_clipboard")&&ok;
        std::vector<unsigned char> output,reloaded;ok=saved_bytes(persist,output)&&write_case_file(name+L"-output.bin",output)&&ok;
        hr=load_bytes(persist,output);result("system_clipboard_reload",hr);ok=hr==S_OK&&ok;
        ok=saved_bytes(persist,reloaded)&&write_case_file(name+L"-reload.bin",reloaded)&&output==reloaded&&ok;
        std::printf("{\"operation\":\"end_system_clipboard_case\",\"case\":\"%s\",\"passed\":%s}\n",test.name,ok?"true":"false");allOk=ok&&allOk;
    }
    timeline.set_paste_mode(0);
    if(ownedSequence&&GetClipboardSequenceNumber()==ownedSequence){
        const auto flushed=OleFlushClipboard();result("system_clipboard_flush",flushed);allOk=flushed==S_OK&&allOk;
        const auto hr=previous.restore(GetClipboardSequenceNumber());result("system_clipboard_restore",hr);allOk=hr==S_OK&&allOk;
    }
    else {result("system_clipboard_restore",S_FALSE);allOk=false;}
    return allOk;
}
static bool probe_drag_start(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, IUnknown* strip, IUnknown* edit, const wchar_t* initialPath) {
    struct Case { const char* name; bool control,all; DWORD effect; HRESULT returned; bool self=false; LONG dropX=384; bool real=false; };
    const Case cases[]={
        {"control_single_cancel",true,false,0,DRAGDROP_S_CANCEL},
        {"control_all_cancel",true,true,0,DRAGDROP_S_CANCEL},
        {"move_single_cancel",false,false,0,DRAGDROP_S_CANCEL},
        {"move_all_cancel",false,true,0,DRAGDROP_S_CANCEL},
        {"control_all_copy",true,true,1,DRAGDROP_S_DROP},
        {"move_all_copy",false,true,1,DRAGDROP_S_DROP},
        {"control_single_move",true,false,2,DRAGDROP_S_DROP},
        {"move_all_move",false,true,2,DRAGDROP_S_DROP},
        {"control_all_failure",true,true,0,E_FAIL},
        {"self_single_move",false,false,2,DRAGDROP_S_DROP,true},
        {"self_all_copy",true,true,1,DRAGDROP_S_DROP,true},
        {"self_same_position",false,true,2,DRAGDROP_S_DROP,true,192},
        {"real_control_all_cancel",true,true,0,DRAGDROP_S_CANCEL,false,384,true},
        {"real_move_all_cancel",false,true,0,DRAGDROP_S_CANCEL,false,384,true},
    };
    ole_boundary::Hook hook(observedModule);
    std::printf("{\"operation\":\"drag_start_hook\",\"installed\":%s}\n",hook.installed()?"true":"false");
    if(!hook.installed())return false;
    IUnknown* properties=nullptr;auto hr=manager->QueryInterface(producer::IID_IDMUSProdPropPageObject,reinterpret_cast<void**>(&properties));
    if(FAILED(hr))return false;
    using Message=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,UINT,WPARAM,LPARAM,LONG,LONG);
    const auto message=reinterpret_cast<Message>((*reinterpret_cast<void***>(strip))[6]);
    std::wstring parent(initialPath);parent.resize(parent.find_last_of(L"/\\")+1);bool allOk=true;
    for(const auto& test:cases){
        std::printf("{\"operation\":\"begin_drag_start_case\",\"case\":\"%s\",\"control\":%s,\"all\":%s,\"effect\":%lu,\"returned\":\"0x%08lx\"}\n",test.name,test.control?"true":"false",test.all?"true":"false",test.effect,test.returned);
        std::wstring name=parent+L"drag-start-";for(const char* c=test.name;*c;++c)name+=static_cast<wchar_t>(*c);
        hr=timeline.set_zoom(.125);result("drag_start_set_zoom",hr);bool setup=hr==S_OK;
        hr=timeline.set_horizontal_scroll(0);result("drag_start_set_scroll",hr);setup=hr==S_OK&&setup;
        std::vector<unsigned char> bytes(76,0);std::memcpy(bytes.data(),"tetr",4);const DWORD payload=68,record=16;
        std::memcpy(bytes.data()+4,&payload,4);std::memcpy(bytes.data()+8,&record,4);
        const LONG times[]={0,900,2304,4708};const double tempos[]={120,137,150,93.75};
        for(size_t i=0;i<4;++i){std::memcpy(bytes.data()+12+i*16,&times[i],4);std::memcpy(bytes.data()+20+i*16,&tempos[i],8);}
        bool ok=write_case_file(name+L"-input.bin",bytes)&&setup;hr=load_bytes(persist,bytes);ok=hr==S_OK&&ok;
        if(test.all){hr=reference_edit::select_all(edit);ok=hr==S_OK&&ok;}
        else {hr=message(strip,WM_LBUTTONDOWN,MK_LBUTTON,0,100,0);ok=hr==S_OK&&ok;hr=message(strip,WM_LBUTTONUP,0,0,100,0);ok=hr==S_OK&&ok;}
        ole_boundary::Context state;state.strip=strip;state.returnedEffect=test.effect;state.returnedResult=test.returned;
        state.dropToSelf=test.self;state.dropX=test.dropX;state.runtime=runtime;
        state.realLoop=test.real;
        ole_boundary::active=&state;
        hr=message(strip,WM_LBUTTONDOWN,MK_LBUTTON|(test.control?MK_CONTROL:0),0,100,0);result("drag_start_down",hr);
        if(!test.control){hr=message(strip,WM_MOUSEMOVE,MK_LBUTTON,0,192,0);result("drag_start_move",hr);}
        hr=message(strip,WM_MOUSEMOVE,MK_LBUTTON,0,192,0);result("drag_start_second_move",hr);ok=hr==S_OK&&ok;
        ole_boundary::active=nullptr;
        ok=state.calls==1&&state.passed&&ok;
        ok=write_case_file(name+L"-clipboard.bin",state.bytes)&&ok;
        if(state.retained){std::vector<unsigned char> retainedBytes;const bool live=ole_boundary::read_data(state.retained,retainedBytes);
            const bool same=live&&retainedBytes==state.bytes;
            const auto released=state.retained->Release();state.retained=nullptr;
            std::printf("{\"operation\":\"drag_start_retained_data\",\"same\":%s,\"released\":%s}\n",same?"true":"false",released==0?"true":"false");ok=same&&released==0&&ok;}
        for(unsigned phase=0;phase<2;++phase){
            if(phase){hr=message(strip,WM_LBUTTONUP,0,0,100,0);result("drag_start_up",hr);ok=hr==S_OK&&ok;}
            void* data=nullptr;hr=reference_edit::get_data(properties,&data);result("drag_start_get_selected",hr);ok=hr==S_OK&&ok;
            std::printf("{\"operation\":\"drag_start_selection_present\",\"phase\":%u,\"present\":%s}\n",phase,data?"true":"false");
            if(data){alignas(8) unsigned char value[0x22]{};std::memcpy(value,data,sizeof(value));position_data(phase?"drag_start_up":"drag_start_return",100,value);}
            std::vector<unsigned char> output;ok=saved_bytes(persist,output)&&write_case_file(name+(phase?L"-up.bin":L"-returned.bin"),output)&&ok;
            if(!phase){
                const HRESULT canCopy=reference_edit::can_copy(edit);result("drag_start_can_copy",canCopy);
                std::vector<unsigned char> selected;IUnknown* copied=nullptr;
                if(canCopy==S_OK){hr=timeline.create_data_object(&copied);ok=hr==S_OK&&ok;
                    if(copied){hr=ReferenceTimeline::set_data_boundaries(copied,0,30720);ok=hr==S_OK&&ok;
                        hr=reference_edit::copy(edit,copied);ok=hr==S_OK&&ok;
                        ok=read_copy_data(copied,RegisterClipboardFormatA("Jazz v.1 Tempolist"),selected)&&ok;copied->Release();}}
                else ok=canCopy==S_FALSE&&ok;
                ok=write_case_file(name+L"-selected-clipboard.bin",selected)&&ok;
                if(output.size()==12){LONG next=0;runtime_tempo::TempoParam value{};hr=runtime_tempo::get(runtime,0,&next,&value);result("drag_start_runtime_empty",hr);ok=FAILED(hr)&&ok;}
                else for(size_t offset=12;offset+16<=output.size();offset+=16){LONG time=0;double tempo=0;
                    std::memcpy(&time,output.data()+offset,4);std::memcpy(&tempo,output.data()+offset+8,8);
                    ok=check_editor_tempo(manager,&timeline,time,time,tempo)&&check_runtime_tempo(runtime,tempo,time)&&ok;}
            }
            if(phase){hr=load_bytes(persist,output);ok=hr==S_OK&&ok;std::vector<unsigned char> reloaded;
                ok=saved_bytes(persist,reloaded)&&write_case_file(name+L"-reload.bin",reloaded)&&reloaded==output&&ok;}
        }
        ok=observe_undo_label(manager,"drag_start")&&ok;
        std::printf("{\"operation\":\"end_drag_start_case\",\"case\":\"%s\",\"calls\":%u,\"passed\":%s}\n",test.name,state.calls,ok?"true":"false");allOk=ok&&allOk;
    }
    properties->Release();const bool restored=hook.restore();std::printf("{\"operation\":\"drag_start_hook_restored\",\"restored\":%s}\n",restored?"true":"false");return restored&&allOk;
}
static bool probe_drop_events(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, IUnknown* strip, IUnknown* edit, const wchar_t* initialPath) {
    struct Event { LONG time; double tempo; };
    struct Case { const char* name; LONG x; DWORD keys, allowed; bool valid; LONG length; bool selected=true; };
    const Case cases[] = {
        {"copy",192,MK_LBUTTON,1,true,30720},
        {"move",192,MK_LBUTTON,3,true,30720},
        {"control",192,MK_LBUTTON|MK_CONTROL,3,true,30720},
        {"control_without_allowed_copy",192,MK_LBUTTON|MK_CONTROL,0,true,30720},
        {"snap",200,MK_LBUTTON,1,true,30720},
        {"origin",0,MK_LBUTTON,1,true,30720},
        {"near_end",3800,MK_LBUTTON,1,true,30720},
        {"short_target",192,MK_LBUTTON,1,true,1000},
        {"negative",-1,MK_LBUTTON,1,true,30720},
        {"unsupported",192,MK_LBUTTON,1,false,30720},
        {"no_effect",192,MK_LBUTTON,0,true,30720},
        {"release_after_enter",192,MK_LBUTTON,1,true,30720},
        {"unselected",192,MK_LBUTTON,1,true,30720,false},
        {"single_selected",192,MK_LBUTTON,1,true,30720,false},
        {"release_negative_over",192,MK_LBUTTON,1,true,30720},
        {"unsupported_medium",192,MK_LBUTTON,1,true,30720},
        {"query_sfalse",192,MK_LBUTTON,1,true,30720},
        {"right_dismiss_none",192,MK_RBUTTON,0,true,30720},
        {"right_dismiss_copy",192,MK_RBUTTON,1,true,30720},
        {"right_dismiss_move",192,MK_RBUTTON,2,true,30720},
        {"right_dismiss_both",192,MK_RBUTTON,3,true,30720},
        {"right_dismiss_link",192,MK_RBUTTON,4,true,30720},
        {"right_dismiss_unsupported",192,MK_RBUTTON,3,false,30720},
        {"right_dismiss_negative",-1,MK_RBUTTON,3,true,30720},
    };
    ole_boundary::Hook menuHook(observedModule,"user32.dll","TrackPopupMenu",reinterpret_cast<ULONG_PTR>(drop_menu_boundary::dismiss));
    std::printf("{\"operation\":\"drop_menu_hook\",\"installed\":%s}\n",menuHook.installed()?"true":"false");
    if(!menuHook.installed())return false;
    const auto encode = [](const std::vector<Event>& events) {
        std::vector<unsigned char> bytes(12+events.size()*16,0);std::memcpy(bytes.data(),"tetr",4);
        const DWORD size=static_cast<DWORD>(bytes.size()-8),record=16;
        std::memcpy(bytes.data()+4,&size,4);std::memcpy(bytes.data()+8,&record,4);
        for(size_t i=0;i<events.size();++i){std::memcpy(bytes.data()+12+i*16,&events[i].time,4);std::memcpy(bytes.data()+20+i*16,&events[i].tempo,8);}
        return bytes;
    };
    IDropTarget* target=nullptr;IUnknown* properties=nullptr;
    HRESULT hr=strip->QueryInterface(IID_IDropTarget,reinterpret_cast<void**>(&target));
    if(SUCCEEDED(hr))hr=manager->QueryInterface(producer::IID_IDMUSProdPropPageObject,reinterpret_cast<void**>(&properties));
    if(FAILED(hr)){if(target)target->Release();return false;}
    std::wstring parent(initialPath);parent.resize(parent.find_last_of(L"/\\")+1);
    bool allOk=true;
    for(const auto& test:cases) {
        std::printf("{\"operation\":\"begin_drop_case\",\"case\":\"%s\",\"x\":%ld,\"keys\":%lu,\"allowed\":%lu,\"valid\":%s,\"length\":%ld}\n",test.name,test.x,test.keys,test.allowed,test.valid?"true":"false",test.length);
        std::wstring name=parent+L"drop-";for(const char* letter=test.name;*letter;++letter)name+=static_cast<wchar_t>(*letter);
        const auto source=encode({{900,137},{2304,150},{4708,93.75}}),initial=encode({{0,80},{1668,95},{5500,101}});
        bool ok=write_case_file(name+L"-source.bin",source)&&write_case_file(name+L"-target.bin",initial);
        hr=timeline.set_length(30720);ok=hr==S_OK&&ok;
        hr=load_bytes(persist,source);result("drop_load_source",hr);ok=hr==S_OK&&ok;
        hr=reference_edit::select_all(edit);result("drop_select_source",hr);ok=hr==S_OK&&ok;
        IUnknown* copied=nullptr;hr=timeline.create_data_object(&copied);ok=hr==S_OK&&ok;
        std::vector<unsigned char> bytes;
        if(copied){hr=ReferenceTimeline::set_data_boundaries(copied,768,6000);ok=hr==S_OK&&ok;
            hr=reference_edit::copy(edit,copied);result("drop_copy_source",hr);ok=hr==S_OK&&ok;
            ok=read_copy_data(copied,RegisterClipboardFormatA("Jazz v.1 Tempolist"),bytes)&&ok;copied->Release();}
        ok=write_case_file(name+L"-clipboard.bin",bytes)&&ok;
        hr=timeline.set_length(test.length);result("drop_set_length",hr);ok=hr==S_OK&&ok;
        hr=load_bytes(persist,initial);result("drop_load_target",hr);ok=hr==S_OK&&ok;
        if(test.selected){hr=reference_edit::select_all(edit);ok=hr==S_OK&&ok;}
        if(std::string(test.name)=="single_selected"){
            using Message=HRESULT(STDMETHODCALLTYPE*)(IUnknown*,UINT,WPARAM,LPARAM,LONG,LONG);
            const auto message=reinterpret_cast<Message>((*reinterpret_cast<void***>(strip))[6]);
            hr=message(strip,WM_LBUTTONDOWN,MK_LBUTTON,0,10,0);ok=hr==S_OK&&ok;
            hr=message(strip,WM_LBUTTONUP,0,0,10,0);ok=hr==S_OK&&ok;
        }
        // Deliberately leave cursor/mode unrelated to the drop location.
        hr=timeline.set_marker(0,100);ok=hr==S_OK&&ok;
        hr=timeline.set_paste_mode(1);ok=hr==S_OK&&ok;
        drag_probe::Data data(bytes,test.valid);DWORD effect=test.allowed;
        if(std::string(test.name)=="unsupported_medium")data.medium=TYMED_HGLOBAL;
        if(std::string(test.name)=="query_sfalse")data.queryResult=S_FALSE;
        hr=target->DragEnter(&data,test.keys,{test.x,0},&effect);
        std::printf("{\"operation\":\"drop_enter\",\"hresult\":\"0x%08lx\",\"effect\":%lu,\"refs\":%lu}\n",hr,effect,data.refs);ok=hr==S_OK&&data.refs==2&&ok;
        const bool release=std::string(test.name)=="release_after_enter"||std::string(test.name)=="release_negative_over";
        const DWORD dropKeys=release?0:test.keys;
        const LONG overX=std::string(test.name)=="release_negative_over"?-1:test.x;
        DWORD over=test.allowed;hr=target->DragOver(dropKeys,{overX,0},&over);result("drop_over",hr);ok=hr==S_OK&&ok;
        const auto menuCalls=drop_menu_boundary::calls;
        effect=test.allowed;hr=target->Drop(&data,dropKeys,{test.x,0},&effect);
        std::printf("{\"operation\":\"drop_execute\",\"hresult\":\"0x%08lx\",\"effect\":%lu,\"refs\":%lu}\n",hr,effect,data.refs);ok=hr==S_OK&&data.refs==1&&ok;
        if(test.keys&MK_RBUTTON){const bool seen=drop_menu_boundary::calls==menuCalls+1;
            std::printf("{\"operation\":\"drop_menu_dismissed\",\"observed\":%s,\"effect\":%lu}\n",seen?"true":"false",effect);
            ok=seen&&drop_menu_boundary::passed&&effect==0&&ok;}
        ok=observe_undo_label(manager,"drop")&&ok;
        LONG cursor=-1;timeline.get_marker(0,&cursor);DWORD mode=0;timeline.get_paste_mode(&mode);
        std::printf("{\"operation\":\"drop_cursor_mode\",\"cursor\":%ld,\"mode\":%lu}\n",cursor,mode);ok=cursor==100&&mode==1&&ok;
        void* borrowed=nullptr;hr=reference_edit::get_data(properties,&borrowed);result("drop_get_selected",hr);ok=hr==S_OK&&ok;
        std::printf("{\"operation\":\"drop_selection_present\",\"present\":%s}\n",borrowed?"true":"false");
        if(borrowed){alignas(8) unsigned char value[0x22]{};std::memcpy(value,borrowed,sizeof(value));position_data("drop",test.x,value);}
        IUnknown* selectedData=nullptr;hr=timeline.create_data_object(&selectedData);ok=hr==S_OK&&ok;
        std::vector<unsigned char> selectedBytes;
        if(selectedData){hr=ReferenceTimeline::set_data_boundaries(selectedData,0,30720);ok=hr==S_OK&&ok;
            hr=reference_edit::copy(edit,selectedData);result("drop_copy_selection",hr);ok=hr==S_OK&&ok;
            ok=read_copy_data(selectedData,RegisterClipboardFormatA("Jazz v.1 Tempolist"),selectedBytes)&&ok;selectedData->Release();}
        ok=write_case_file(name+L"-selected-clipboard.bin",selectedBytes)&&ok;
        std::vector<unsigned char> output,after;
        ok=saved_bytes(persist,output)&&write_case_file(name+L"-output.bin",output)&&ok;
        if(output.size()<12||(output.size()-12)%16)ok=false;
        if(ok)for(size_t offset=12;offset<output.size();offset+=16){
            Event event{};std::memcpy(&event.time,output.data()+offset,4);std::memcpy(&event.tempo,output.data()+offset+8,8);
            std::printf("{\"operation\":\"drop_saved_event\",\"index\":%zu,\"time\":%ld,\"tempo\":%.17g}\n",(offset-12)/16,event.time,event.tempo);
            ok=check_editor_tempo(manager,&timeline,event.time,event.time,event.tempo)&&ok;
            ok=check_runtime_tempo(runtime,event.tempo,event.time)&&ok;
        }
        hr=load_bytes(persist,output);result("drop_reload",hr);ok=hr==S_OK&&ok;
        ok=saved_bytes(persist,after)&&write_case_file(name+L"-reload.bin",after)&&after==output&&ok;
        std::printf("{\"operation\":\"end_drop_case\",\"case\":\"%s\",\"passed\":%s}\n",test.name,ok?"true":"false");allOk=ok&&allOk;
    }
    timeline.set_length(30720);timeline.set_paste_mode(0);properties->Release();target->Release();
    const bool restored=menuHook.restore();std::printf("{\"operation\":\"drop_menu_hook_restored\",\"restored\":%s}\n",restored?"true":"false");return restored&&allOk;
}
#endif

static bool probe_drawing(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
#ifndef PRODUCER_WINDOWED_PROBE
    (void)runtime;
#endif
    struct Event { LONG time; double tempo; };
    struct Case {
        const char* name; double zoom; LONG offset; LONG clipLeft; LONG clipRight;
        DWORD view; bool selected; bool range; std::vector<Event> events;
        LONG scroll = 0;
    };
    const std::vector<Event> multiple{{0,120},{900,137},{2304,150},{4708,93.75}};
    std::vector<Case> cases = {
        {"empty",.125,0,0,640,0,false,false,{}},
        {"single",.125,0,0,640,0,false,false,{{0,120}}},
        {"fractional",.125,0,0,640,0,false,false,{{0,93.75}}},
        {"multiple",.125,0,0,640,0,false,false,multiple},
        {"same_beat",.125,0,0,640,0,false,false,{{0,120},{100,130},{300,140},{900,137}}},
        {"selected",.125,0,0,640,0,true,false,multiple},
        {"selected_same_beat",.125,0,0,640,0,true,false,{{0,120},{100,130},{300,140},{900,137}}},
        {"range",.125,0,0,640,0,false,true,multiple},
        {"offset",.125,192,0,640,0,false,false,multiple},
        {"clipped",.125,0,100,500,0,false,false,multiple},
        {"tight",.01,0,0,640,0,false,false,multiple},
        {"alternate_view",.125,0,0,640,1,false,false,multiple},
    };
#ifdef PRODUCER_WINDOWED_PROBE
    HiddenOleHost host;
    IOleObject* object = nullptr;
    HRESULT activated = timeline.query(IID_IOleObject, reinterpret_cast<void**>(&object));
    if (SUCCEEDED(activated)) { activated = host.activate(object); object->Release(); }
    result("draw_activate_window", activated);
    if (FAILED(activated)) return false;
    cases.push_back({"scroll_beat",.125,192,0,640,0,false,false,multiple,192});
    cases.push_back({"scroll_middle",.125,200,0,640,0,false,false,multiple,200});
    cases.push_back({"scroll_same_beat",.125,192,0,640,0,false,false,{{0,120},{800,130},{900,140},{3000,150}},192});
    cases.push_back({"scroll_selected",.125,192,0,640,0,true,false,multiple,192});
    cases.push_back({"ghost_single",.125,192,0,640,0,false,false,{{0,120},{4708,137}},192});
    cases.push_back({"ghost_italic",.125,192,0,640,0,false,false,{{0,120},{100,130},{300,140},{4708,137}},192});
    cases.push_back({"ghost_middle",.125,200,0,640,0,false,false,{{0,120},{4708,137}},200});
    cases.push_back({"ghost_selected",.125,192,0,640,0,true,false,{{0,120},{4708,137}},192});
#endif
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip); result("draw_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit));
    if (FAILED(hr) || !edit) { strip->Release(); return false; }
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    bool allOk = true;
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_draw_case\",\"case\":\"%s\",\"zoom\":%.17g,\"offset\":%ld,\"clip_left\":%ld,\"clip_right\":%ld,\"view\":%lu,\"selected\":%s,\"range\":%s}\n",
            test.name,test.zoom,test.offset,test.clipLeft,test.clipRight,test.view,test.selected?"true":"false",test.range?"true":"false");
        std::wstring name = parent + L"draw-";
        for (const char* letter = test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        std::vector<unsigned char> input(12 + test.events.size() * 16, 0);
        std::memcpy(input.data(), "tetr", 4);
        const DWORD size = static_cast<DWORD>(input.size() - 8);
        std::memcpy(input.data() + 4, &size, 4); input[8] = 16;
        for (size_t i = 0; i < test.events.size(); ++i) {
            std::memcpy(input.data() + 12 + i * 16, &test.events[i].time, 4);
            std::memcpy(input.data() + 20 + i * 16, &test.events[i].tempo, 8);
        }
        bool ok = write_case_file(name + L"-input.bin", input);
        VARIANT flag{}; flag.vt = VT_BOOL;
        hr = reference_edit::set_strip_property(strip, 2, flag); result("draw_clear_gutter", hr); ok = hr == S_OK && ok;
        hr = load_bytes(persist, input); result("draw_load", hr); ok = hr == S_OK && ok;
        hr = timeline.set_zoom(test.zoom); result("draw_set_zoom", hr); ok = hr == S_OK && ok;
#ifdef PRODUCER_WINDOWED_PROBE
        hr = timeline.set_horizontal_scroll(test.scroll); result("draw_set_scroll", hr); ok = hr == S_OK && ok;
        VARIANT scroll{};
        hr = timeline.get_property(9, &scroll); result("draw_get_scroll", hr);
        const bool exact = hr == S_OK && scroll.vt == VT_I4 && scroll.lVal == test.scroll;
        std::printf("{\"operation\":\"draw_scroll_readback\",\"requested\":%ld,\"actual\":%ld,\"exact\":%s}\n",test.scroll,scroll.lVal,exact?"true":"false");
        ok = exact && ok; VariantClear(&scroll);
#endif
        if (test.selected) { hr = reference_edit::select_all(edit); result("draw_select_all", hr); ok = hr == S_OK && ok; }
        if (test.range) {
            VARIANT position{}; position.vt = VT_I4; position.lVal = 900;
            hr = reference_edit::set_strip_property(strip, 3, position); result("draw_range_start", hr); ok = hr == S_OK && ok;
            position.lVal = 2304;
            hr = reference_edit::set_strip_property(strip, 4, position); result("draw_range_end", hr); ok = hr == S_OK && ok;
            flag.boolVal = 1;
            hr = reference_edit::set_strip_property(strip, 2, flag); result("draw_range_gutter", hr); ok = hr == S_OK && ok;
        }
        LONG visibleStart = -1;
        hr = timeline.get_marker(3, &visibleStart); result("draw_visible_marker", hr);
        std::printf("{\"operation\":\"draw_visible_start\",\"time\":%ld}\n",visibleStart); ok = hr == S_OK && ok;
        std::vector<unsigned char> first;
        for (unsigned repeat = 0; repeat < 2; ++repeat) {
            DrawingSurface surface(test.clipLeft,test.clipRight);
            if (!surface.valid()) { ok = false; break; }
            if (test.name == std::string("empty") && repeat == 0) {
                const auto font = surface.font_data(); ok = !font.empty() && write_case_file(parent + L"draw-font.bin",font) && ok;
                char face[128]{}; GetTextFaceA(surface.dc(),static_cast<int>(std::size(face)),face);
                std::printf("{\"operation\":\"draw_font\",\"face\":\"%s\",\"bytes\":%zu,\"height\":-12,\"quality\":3}\n",face,font.size());
            }
            using Draw = HRESULT (STDMETHODCALLTYPE*)(IUnknown*,HDC,DWORD,LONG);
            hr = reinterpret_cast<Draw>((*reinterpret_cast<void***>(strip))[3])(strip,surface.dc(),test.view,test.offset);
            result("draw_strip",hr); ok = hr == S_OK && ok;
            std::printf("{\"operation\":\"draw_dc_state\",\"repeat\":%u,\"font_preserved\":%s,\"text_color\":%lu,\"background_color\":%lu,\"background_mode\":%d}\n",
                repeat,surface.font_preserved()?"true":"false",GetTextColor(surface.dc()),GetBkColor(surface.dc()),GetBkMode(surface.dc()));
            const auto bitmap = surface.bitmap_bytes();
            ok = write_case_file(name + (repeat ? L"-repeat.bmp" : L"-output.bmp"),bitmap) && ok;
            if (repeat) ok = bitmap == first && ok; else first = bitmap;
        }
        std::vector<unsigned char> saved;
        ok = saved_bytes(persist,saved) && write_case_file(name + L"-after.bin",saved) && saved == input && ok;
        std::printf("{\"operation\":\"end_draw_case\",\"case\":\"%s\",\"passed\":%s}\n",test.name,ok?"true":"false");
        allOk = ok && allOk;
    }
    VARIANT flag{}; flag.vt = VT_BOOL;
    hr = reference_edit::set_strip_property(strip,2,flag); result("draw_restore_gutter",hr); allOk = hr == S_OK && allOk;
    hr = timeline.set_zoom(.125); result("draw_restore_zoom",hr); allOk = hr == S_OK && allOk;
#ifdef PRODUCER_WINDOWED_PROBE
    hr = timeline.set_horizontal_scroll(0); result("draw_restore_scroll",hr); allOk = hr == S_OK && allOk;
    allOk = probe_mouse_selection(persist,manager,timeline,initialPath) && allOk;
    allOk = drag_probe::callbacks(strip) && allOk;
    allOk = probe_drop_events(persist,runtime,manager,timeline,strip,edit,initialPath) && allOk;
    allOk = probe_drag_start(persist,runtime,manager,timeline,strip,edit,initialPath) && allOk;
    allOk = probe_commands(persist,runtime,manager,timeline,strip,edit,initialPath) && allOk;
    if(testSystemClipboard)allOk=probe_system_clipboard(persist,manager,timeline,strip,edit,initialPath)&&allOk;
    hr = host.close(); result("draw_close_window",hr); allOk = hr == S_OK && allOk;
#endif
    edit->Release(); strip->Release(); return allOk;
}

static bool probe_insert_events(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    const HRESULT zoomHr = timeline.set_zoom(0.125);
    result("insert_set_zoom", zoomHr);
    if (zoomHr != S_OK) return false;
    struct Event { LONG time; double tempo; };
    struct Case { const char* name; LONG at; std::vector<Event> events; };
    const Case cases[] = {
        {"empty", 900, {}},
        {"between", 900, {{0,137},{1536,150}}},
        {"occupied", 900, {{0,137},{768,150}}},
        {"occupied_tick", 900, {{0,137},{900,150}}},
        {"later_measure", 4708, {{0,137}}},
        {"negative", -1, {{0,137}}},
        {"beyond_length", 40000, {{0,137}}},
    };
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip); result("insert_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit));
    if (FAILED(hr) || !edit) { strip->Release(); return false; }
    IUnknown* properties = nullptr;
    hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject, reinterpret_cast<void**>(&properties));
    if (FAILED(hr) || !properties) { strip->Release(); edit->Release(); return false; }
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    bool allOk = true;
    result("can_insert_before_mouse_position", reference_edit::can_insert(edit));
    result("insert_before_mouse_position", reference_edit::insert(edit));
    for (const auto& test : cases) {
        std::printf("{\"operation\":\"begin_insert_case\",\"case\":\"%s\",\"at\":%ld}\n", test.name, test.at);
        std::wstring name = parent + L"insert-";
        for (const char* letter = test.name; *letter; ++letter) name += static_cast<wchar_t>(*letter);
        std::vector<unsigned char> input(12 + test.events.size() * 16, 0);
        std::memcpy(input.data(), "tetr", 4);
        const DWORD payload = static_cast<DWORD>(input.size() - 8);
        std::memcpy(input.data() + 4, &payload, 4); input[8] = 16;
        for (size_t i = 0; i < test.events.size(); ++i) {
            std::memcpy(input.data() + 12 + i * 16, &test.events[i].time, 4);
            std::memcpy(input.data() + 20 + i * 16, &test.events[i].tempo, 8);
        }
        bool ok = write_case_file(name + L"-input.bin", input);
        hr = load_bytes(persist, input); result("insert_load_input", hr); ok = hr == S_OK && ok;
        LONG x = -1;
        if (test.at >= 0) { hr = timeline.clocks_to_position(test.at, &x); result("insert_clocks_to_position", hr); ok = hr == S_OK && ok; }
        std::printf("{\"operation\":\"insert_mouse_position\",\"at\":%ld,\"x\":%ld}\n", test.at, x);
        if (ok) { hr = reference_edit::mouse_release(strip, x); result("insert_mouse_release", hr); ok = hr == S_OK; }
        if (ok) {
            result("can_insert_at_position", reference_edit::can_insert(edit));
            hr = reference_edit::insert(edit); result("insert_event", hr);
            ok = observe_undo_label(manager, "insert") && ok;
            ok = hr == (test.at < 0 ? E_FAIL : S_OK) && ok;
            result("can_insert_after_insert", reference_edit::can_insert(edit));
        }
        std::vector<unsigned char> output;
        if (ok) ok = saved_bytes(persist, output) && write_case_file(name + L"-output.bin", output);
        if (ok) {
            void* borrowed = nullptr;
            hr = reference_edit::get_data(properties, &borrowed); result("insert_get_selected", hr); ok = hr == S_OK;
            std::printf("{\"operation\":\"insert_has_selection\",\"selected\":%s}\n", borrowed ? "true" : "false");
            if (borrowed) { alignas(8) unsigned char data[0x22]{}; std::memcpy(data, borrowed, sizeof(data)); position_data("inserted", test.at, data); }
            for (DWORD marker : {1u,2u}) {
                LONG time = -1;
                hr = timeline.get_marker(marker, &time);
                std::printf("{\"operation\":\"insert_marker\",\"marker\":%lu,\"time\":%ld,\"hresult\":\"0x%08lx\"}\n", marker, time, static_cast<unsigned long>(hr));
                ok = hr == S_OK && ok;
            }
        }
        if (ok && (output.size() < 12 || (output.size() - 12) % 16)) ok = false;
        if (ok) for (size_t offset = 12; offset < output.size(); offset += 16) {
            Event value{};
            std::memcpy(&value.time, output.data() + offset, 4); std::memcpy(&value.tempo, output.data() + offset + 8, 8);
            std::printf("{\"operation\":\"insert_saved_event\",\"index\":%zu,\"time\":%ld,\"tempo\":%.17g}\n", (offset - 12) / 16, value.time, value.tempo);
            LONG next = value.time + 1;
            if (offset + 16 < output.size()) std::memcpy(&next, output.data() + offset + 16, 4);
            if (next != value.time) {
                ok = check_editor_tempo(manager, &timeline, value.time, value.time, value.tempo) && ok;
                ok = check_runtime_tempo(runtime, value.tempo, value.time) && ok;
            }
        }
        if (ok) {
            hr = load_bytes(persist, output); result("insert_reload_output", hr); ok = hr == S_OK;
            std::vector<unsigned char> after;
            ok = saved_bytes(persist, after) && ok;
            const bool equal = output == after;
            std::printf("{\"operation\":\"insert_roundtrip_equal\",\"equal\":%s}\n", equal ? "true" : "false");
            ok = write_case_file(name + L"-reload-output.bin", after) && equal && ok;
        }
        std::printf("{\"operation\":\"end_insert_case\",\"case\":\"%s\",\"passed\":%s}\n", test.name, ok ? "true" : "false");
        allOk = allOk && ok;
    }
    properties->Release(); edit->Release(); strip->Release(); return allOk;
}

static bool probe_position_edit(IPersistStream* persist, IUnknown* runtime, producer::StripManager* manager,
    ReferenceTimeline& timeline, const wchar_t* initialPath) {
    IUnknown* strip = nullptr;
    HRESULT hr = timeline.find_manager_strip(manager, &strip);
    result("position_find_strip", hr);
    if (FAILED(hr) || !strip) return false;
    IUnknown* edit = nullptr;
    hr = strip->QueryInterface(reference_edit::TimelineEditIid, reinterpret_cast<void**>(&edit)); strip->Release();
    if (FAILED(hr) || !edit) return false;
    IUnknown* properties = nullptr;
    hr = manager->QueryInterface(producer::IID_IDMUSProdPropPageObject, reinterpret_cast<void**>(&properties));
    if (FAILED(hr) || !properties) { edit->Release(); return false; }
    std::wstring parent(initialPath); parent.resize(parent.find_last_of(L"/\\") + 1);
    bool allOk = true;
    for (LONG time : {768L, 900L, 4708L}) {
        std::printf("{\"operation\":\"begin_position_case\",\"time\":%ld}\n", time);
        const auto name = parent + L"position-" + std::to_wstring(time);
        std::vector<unsigned char> input(28, 0);
        std::memcpy(input.data(), "tetr", 4); input[4] = 20; input[8] = 16;
        const double initialTempo = 118;
        std::memcpy(input.data() + 12, &time, 4); std::memcpy(input.data() + 20, &initialTempo, 8);
        bool ok = write_case_file(name + L"-input.bin", input);
        IStream* stream = nullptr;
        hr = CreateStreamOnHGlobal(nullptr, TRUE, &stream);
        if (SUCCEEDED(hr)) {
            ULONG written = 0;
            hr = stream->Write(input.data(), static_cast<ULONG>(input.size()), &written);
            LARGE_INTEGER zero{};
            if (SUCCEEDED(hr) && written != input.size()) hr = E_FAIL;
            if (SUCCEEDED(hr)) hr = stream->Seek(zero, STREAM_SEEK_SET, nullptr);
            if (SUCCEEDED(hr)) hr = persist->Load(stream);
            stream->Release();
        }
        result("load_position_stream", hr); ok = SUCCEEDED(hr) && ok;
        if (ok) { hr = reference_edit::select_all(edit); result("select_position_event", hr); ok = hr == S_OK; }
        void* borrowed = nullptr;
        if (ok) { hr = reference_edit::get_data(properties, &borrowed); result("get_position_data", hr); ok = hr == S_OK && borrowed; }
        alignas(8) unsigned char data[0x22]{};
        if (ok) {
            std::memcpy(data, borrowed, sizeof(data)); position_data("initial", time, data);
            const double edited = 133.5;
            std::memcpy(data + 8, &edited, 8);
            hr = reference_edit::set_data(properties, data); result("set_nonzero_tempo", hr); ok = hr == S_OK;
            ok = check_editor_tempo(manager, &timeline, time, time, edited) && ok;
            ok = check_runtime_tempo(runtime, edited, time) && ok;
            std::vector<unsigned char> output;
            ok = saved_bytes(persist, output) && ok;
            ok = write_case_file(name + L"-tempo-output.bin", output) && ok;
            LONG measure = 0; std::memcpy(&measure, data + 16, 4); ++measure;
            std::memcpy(data + 16, &measure, 4);
            hr = reference_edit::set_data(properties, data); result("move_position_one_measure", hr); ok = hr == S_OK && ok;
            ok = observe_undo_label(manager, "move") && ok;
            output.clear(); ok = saved_bytes(persist, output) && ok;
            ok = write_case_file(name + L"-move-output.bin", output) && ok;
            LONG movedTime = -1;
            if (output.size() == 28) std::memcpy(&movedTime, output.data() + 12, 4);
            const bool moved = movedTime == time + 3072;
            std::printf("{\"operation\":\"position_move_observation\",\"from\":%ld,\"to\":%ld,\"one_measure\":%s}\n", time, movedTime, moved ? "true" : "false");
            ok = moved && ok;
            borrowed = nullptr;
            hr = reference_edit::get_data(properties, &borrowed); result("get_moved_position_data", hr);
            if (hr == S_OK && borrowed) { std::memcpy(data, borrowed, sizeof(data)); position_data("moved", time, data); }
            else ok = false;
            ok = check_editor_tempo(manager, &timeline, movedTime, movedTime, edited) && ok;
            ok = check_runtime_tempo(runtime, edited, movedTime) && ok;
        }
        std::printf("{\"operation\":\"end_position_case\",\"time\":%ld,\"passed\":%s}\n", time, ok ? "true" : "false");
        allOk = allOk && ok;
        if (!ok) break;
    }
    properties->Release(); edit->Release(); return allOk;
}

int wmain(int argc, wchar_t** argv) {
    if(argc>1 && wcscmp(argv[argc-1],L"--clipboard")==0){testSystemClipboard=true;--argc;}
#ifdef PRODUCER_WINDOWED_PROBE
    const BOOL enabled = SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
    DWORD flags = 0; BOOL permanent = FALSE;
    const BOOL queried = GetProcessDEPPolicy(GetCurrentProcess(), &flags, &permanent);
    const bool valid = enabled && queried && flags == PROCESS_DEP_ENABLE && permanent;
    std::printf("{\"operation\":\"windowed_probe_dep\",\"enabled\":%s,\"flags\":%lu,\"permanent\":%s}\n",valid?"true":"false",flags,permanent?"true":"false");
    if (!valid) return 1;
#endif
    if (argc < 3 || argc > 8 || (argc >= 5 && wcscmp(argv[4], L"--connected") != 0)
        || (argc >= 6 && wcscmp(argv[5], L"--timeline") != 0)) {
        std::fputs("Usage: com_probe <absolute DLL path> <CLSID> [new stream output file [--connected [--timeline [absolute Timeline DLL [TimeSigStripMgr DLL]]]]]\n", stderr);
        return 2;
    }
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    AddVectoredExceptionHandler(1, observe_exception);
    CLSID clsid{};
    HRESULT hr = CLSIDFromString(argv[2], &clsid);
    if (FAILED(hr)) { result("parse_clsid", hr); return 2; }
    if (argc >= 5 && clsid != producer::CLSID_TempoMgr) return 2;
    hr = argc >= 6 ? OleInitialize(nullptr) : CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    result("initialize_sta", hr);
    if (FAILED(hr)) return 1;
    HMODULE module = LoadLibraryExW(argv[1], nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!module) {
        result("load_library", HRESULT_FROM_WIN32(GetLastError()));
        if (argc >= 6) OleUninitialize(); else CoUninitialize();
        return 1;
    }
    result("load_library", S_OK);
    observedModule = module;
    using GetClassObject = HRESULT (STDAPICALLTYPE*)(REFCLSID, REFIID, LPVOID*);
    using CanUnload = HRESULT (STDAPICALLTYPE*)();
    auto getClass = reinterpret_cast<GetClassObject>(GetProcAddress(module, "DllGetClassObject"));
    auto canUnload = reinterpret_cast<CanUnload>(GetProcAddress(module, "DllCanUnloadNow"));
    int failure = 0;
    if (!getClass) { result("get_export", HRESULT_FROM_WIN32(GetLastError())); failure = 1; }
    else {
        IClassFactory* factory = nullptr;
        hr = getClass(clsid, IID_IClassFactory, reinterpret_cast<void**>(&factory));
        result("class_factory", hr);
        if (SUCCEEDED(hr) && factory) {
            IUnknown* object = nullptr;
            hr = factory->CreateInstance(nullptr, IID_IUnknown, reinterpret_cast<void**>(&object));
            result("create_instance", hr);
            if (SUCCEEDED(hr) && object) {
                std::printf("{\"operation\":\"object_vtable\",\"module_rva\":\"0x%lx\"}\n",
                    static_cast<unsigned long>(*reinterpret_cast<std::uintptr_t*>(object) - reinterpret_cast<std::uintptr_t>(module)));
                if (IsEqualGUID(clsid, producer::CLSID_TempoMgr)) {
                    if (!probe_interface(object, producer::IID_IDMUSProdStripMgr, "query_strip_mgr", module)) failure = 1;
                    if (!probe_interface(object, producer::IID_IDMUSProdPropPageObject, "query_prop_page_object", module)) failure = 1;
                }
                IUnknown* runtime = nullptr;
                fixture::Framework* framework = nullptr;
                producer::StripManager* strip = nullptr;
                ReferenceTimeline timeline;
                ReferenceTimeSignature timeSignature;
                bool timeSignatureInserted = false;
                bool timelineInserted = false;
                bool connected = argc < 5;
                if (argc >= 5 && IsEqualGUID(clsid, producer::CLSID_TempoMgr)) {
                    hr = object->QueryInterface(producer::IID_IDMUSProdStripMgr, reinterpret_cast<void**>(&strip));
                    result("query_strip_for_connection", hr);
                    if (SUCCEEDED(hr)) {
                        if (!probe_manager_properties(strip)) failure = 1;
                        HRESULT supports = strip->IsParamSupported(runtime_tempo::TempoParamGuid);
                        result("editor_supports_tempo", supports);
                        if (supports != S_OK) failure = 1;
                        supports = strip->IsParamSupported(GUID_NULL);
                        result("editor_unsupported_param", supports);
                        if (supports != S_FALSE) failure = 1;
                        const HRESULT setResult = strip->SetParam(runtime_tempo::TempoParamGuid, 0, nullptr);
                        result("editor_set_param_unimplemented", setResult);
                        if (setResult != E_NOTIMPL) failure = 1;
                    }
                    framework = new fixture::Framework;
                    if (SUCCEEDED(hr)) hr = fixture::set_property(strip, 2, framework);
                    result("set_framework", hr);
                    if (SUCCEEDED(hr)) hr = CoCreateInstance(producer::CLSID_DirectMusicTempoTrack, nullptr,
                        CLSCTX_INPROC_SERVER, runtime_tempo::TrackIid, reinterpret_cast<void**>(&runtime));
                    result("create_runtime_tempo_track", hr);
                    if (SUCCEEDED(hr)) {
                        runtime_module(runtime);
                        hr = fixture::set_property(strip, 1, runtime);
                        result("set_runtime_track", hr);
                        connected = SUCCEEDED(hr);
                        if (connected) {
                            // A different sentinel distinguishes successful Load synchronization
                            // from merely observing the runtime's own default tempo.
                            runtime_tempo::TempoParam sentinel{0, 90.0};
                            hr = runtime_tempo::set(runtime, 0, &sentinel);
                            result("seed_runtime_tempo", hr);
                            connected = SUCCEEDED(hr) && check_runtime_tempo(runtime, 90.0);
                        }
                    }
                    if (!connected) failure = 1;
                    if (connected && argc >= 6) {
                        hr = timeline.load(argv[1], argc >= 7 ? argv[6] : nullptr); result("load_original_timeline", hr);
                        if (SUCCEEDED(hr) && argc == 8) {
                            hr = timeSignature.load(argv[7]); result("load_original_time_signature", hr);
                            if (SUCCEEDED(hr)) {
                                std::wstring parent(argv[3]); parent.resize(parent.find_last_of(L"/\\") + 1);
                                if (!write_case_file(parent + L"timesig-input.bin", ReferenceTimeSignature::default_meter_stream())) hr = E_FAIL;
                                if (SUCCEEDED(hr)) hr = timeSignature.seed_default_meter();
                                result("load_default_time_signature", hr);
                            }
                            if (SUCCEEDED(hr)) {
                                hr = timeline.insert(timeSignature.manager()); result("timeline_insert_time_signature", hr);
                                timeSignatureInserted = SUCCEEDED(hr);
                            }
                            if (SUCCEEDED(hr)) {
                                hr = timeline.set_length(30720); result("set_timeline_length", hr);
                                LONG measure = -1, beat = -1;
                                if (SUCCEEDED(hr)) hr = timeline.clocks_to_measure_beat(4708, &measure, &beat);
                                result("timeline_convert_position", hr);
                                std::printf("{\"operation\":\"timeline_position\",\"time\":4708,\"measure\":%ld,\"beat\":%ld}\n", measure, beat);
                            }
                        }
                        if (SUCCEEDED(hr)) {
                            hr = timeline.insert(strip); result("timeline_insert_strip_manager", hr);
                            timelineInserted = SUCCEEDED(hr);
                        }
                        connected = timelineInserted;
                        if (!connected) failure = 1;
                        if (timelineInserted) {
                            if (!probe_notification_registration(timeline, strip, true)) failure = 1;
                            VARIANT timelineProperty{};
                            hr = strip->GetStripMgrProperty(0, &timelineProperty); result("get_connected_timeline_property", hr);
                            IUnknown* canonicalTimeline = nullptr;
                            if (SUCCEEDED(hr) && timelineProperty.vt == VT_UNKNOWN && timelineProperty.punkVal)
                                hr = timelineProperty.punkVal->QueryInterface(producer::IID_IDMUSProdTimeline, reinterpret_cast<void**>(&canonicalTimeline));
                            const bool sameInterface = SUCCEEDED(hr) && canonicalTimeline && canonicalTimeline == timelineProperty.punkVal;
                            std::printf("{\"operation\":\"timeline_property_is_timeline_interface\",\"same\":%s}\n", sameInterface ? "true" : "false");
                            if (canonicalTimeline) canonicalTimeline->Release();
                            if (timelineProperty.vt == VT_UNKNOWN && timelineProperty.punkVal) timelineProperty.punkVal->Release();
                            if (!sameInterface) failure = 1;
                            IUnknown* firstStrip = nullptr;
                            hr = timeline.enum_strip(0, &firstStrip); result("timeline_enum_first_strip", hr);
                            if (FAILED(hr) || !firstStrip) failure = 1;
                            if (firstStrip) firstStrip->Release();
                        }
                    }
                }
                IPersistStream* persist = nullptr;
                hr = object->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&persist));
                result("query_persist_stream", hr);
                if (SUCCEEDED(hr) && persist) {
                    CLSID actual{};
                    hr = persist->GetClassID(&actual); result("get_class_id", hr);
                    if (SUCCEEDED(hr)) guid_result("persist_class_id", actual);
                    result("is_dirty_initial", persist->IsDirty());
                    ULARGE_INTEGER size{};
                    hr = persist->GetSizeMax(&size); result("get_size_max", hr);
                    if (SUCCEEDED(hr)) std::printf("{\"operation\":\"size_max\",\"bytes\":%llu}\n", size.QuadPart);
                    if (argc >= 4 && connected) {
                        const bool initialOk = probe_initial_stream(persist, argv[3], runtime);
                        if (!initialOk) failure = 1;
                        if (initialOk && runtime) {
                            const bool casesOk = probe_tempo_cases(persist, runtime, strip,
                                timelineInserted ? &timeline : nullptr, argv[3]);
                            if (!casesOk) failure = 1;
                            if (casesOk && timelineInserted && !probe_property_edit(persist, runtime, strip, timeline, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted && !probe_position_edit(persist, runtime, strip, timeline, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted && !probe_position_boundaries(persist, runtime, strip, timeline, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted && !probe_meter_changes(persist, runtime, strip, timeline, timeSignature, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted && !probe_insert_events(persist, runtime, strip, timeline, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted && !probe_copy_events(persist, runtime, strip, timeline, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted && !probe_paste_events(persist, runtime, strip, timeline, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted && !probe_tempo_notification(persist, runtime, strip, timeline, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted && !probe_range_selection(persist, runtime, strip, timeline, argv[3])) failure = 1;
                            if (casesOk && timeSignatureInserted) {
#ifdef PRODUCER_WINDOWED_PROBE
                                PageTrackContext pageHost{persist,runtime,strip,&timeline,argv[3]};
                                if(!property_probe::run(strip,timeline,probe_page_track_edits,&pageHost))failure=1;
#else
                                if(!property_probe::run(strip,timeline))failure=1;
#endif
                            }
                            if (casesOk && timeSignatureInserted && !probe_drawing(persist, runtime, strip, timeline, argv[3])) failure = 1;
                        }
                    }
                    persist->Release();
                } else failure = 1;
                if (timelineInserted) {
                    hr = timeline.remove(strip); result("timeline_remove_strip_manager", hr);
                    if (FAILED(hr)) failure = 1;
                    if (SUCCEEDED(hr) && !probe_notification_registration(timeline, strip, false)) failure = 1;
                }
                if (timeSignatureInserted) {
                    hr = timeline.remove(timeSignature.manager()); result("timeline_remove_time_signature", hr);
                    if (FAILED(hr)) failure = 1;
                }
                if (timeSignature.manager()) {
                    hr = timeSignature.close(); result("time_signature_can_unload_after_release", hr);
                    if (hr != S_OK) failure = 1;
                }
                if (timeline.loaded()) {
                    hr = timeline.close(); result("timeline_can_unload_after_release", hr);
                    // Clipboard export retains a Timeline-created data object
                    // until the Tempo manager dies. Verify final unload below.
                    if (hr != S_OK && !(testSystemClipboard && hr == S_FALSE)) failure = 1;
                }
                if (strip) {
                    result("disconnect_runtime", fixture::set_property(strip, 1, nullptr));
                    result("disconnect_framework", fixture::set_property(strip, 2, nullptr));
                    strip->Release();
                }
                if (runtime) runtime->Release();
                if (framework) {
                    const ULONG remaining = framework->Release();
                    std::printf("{\"operation\":\"release_fixture\",\"remaining\":%lu}\n", remaining);
                    if (remaining) failure = 1;
                }
                const ULONG remaining = object->Release();
                std::printf("{\"operation\":\"release_object\",\"remaining\":%lu}\n", remaining);
                if (remaining) failure = 1;
                if(testSystemClipboard){
                    MSG pending{};
                    while(PeekMessageW(&pending,nullptr,0,0,PM_REMOVE)){TranslateMessage(&pending);DispatchMessageW(&pending);}
                    const auto finalTimeline=timeline.close();result("timeline_can_unload_after_clipboard_owner_release",finalTimeline);
                    std::printf("{\"operation\":\"timeline_module_counter_after_owner_release\",\"value\":%ld}\n",timeline.module_counter());
                    if(finalTimeline==S_FALSE)clipboardTimelineModule=GetModuleHandleW(argv[6]);
                    else if(finalTimeline!=S_OK)failure=1;
                }
                std::fflush(stdout);
            } else failure = 1;
            factory->Release();
        } else failure = 1;
    }
    if (canUnload) {
        hr = canUnload(); result("can_unload_after_release", hr);
        if (hr == S_OK) FreeLibrary(module);
        // If S_FALSE, leave the module mapped until process exit.
    }
    if (argc >= 6) OleUninitialize(); else CoUninitialize();
    if(testSystemClipboard && clipboardTimelineModule){
        using CanUnload=HRESULT(STDMETHODCALLTYPE*)();
        const auto unload=reinterpret_cast<CanUnload>(GetProcAddress(clipboardTimelineModule,"DllCanUnloadNow"));
        const auto finalUnload=unload?unload():E_NOINTERFACE;result("timeline_can_unload_after_ole_shutdown",finalUnload);
        if(finalUnload==S_OK)FreeLibrary(clipboardTimelineModule);else failure=1;
    }
    return failure;
}
