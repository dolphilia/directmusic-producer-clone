#include <windows.h>
#include <ole2.h>
#include <ocidl.h>
#include <cstdio>
#include "reference_timeline.h"
#include "hidden_ole_site.h"

struct ActivationDiagnostic {
    HANDLE done = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    HANDLE main = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, GetCurrentThreadId());
    HMODULE timeline = GetModuleHandleW(L"Timeline.dll");
    HANDLE worker = nullptr;
    ActivationDiagnostic() {
        if (done && main) worker = CreateThread(nullptr, 0, inspect, this, 0, nullptr);
    }
    ~ActivationDiagnostic() {
        if (done) SetEvent(done);
        if (worker) { WaitForSingleObject(worker, INFINITE); CloseHandle(worker); }
        if (main) CloseHandle(main);
        if (done) CloseHandle(done);
    }
    static DWORD WINAPI inspect(void* data) {
        auto& state = *static_cast<ActivationDiagnostic*>(data);
        if (WaitForSingleObject(state.done, 3000) != WAIT_TIMEOUT) return 0;
        CONTEXT context{}; context.ContextFlags = CONTEXT_CONTROL;
        DWORD stack[2048]{}; SIZE_T bytes = 0;
        if (SuspendThread(state.main) == DWORD(-1)) return 0;
        const BOOL captured = GetThreadContext(state.main, &context);
        if (captured) ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(context.Esp), stack, sizeof(stack), &bytes);
        ResumeThread(state.main);
        const auto base = reinterpret_cast<DWORD>(state.timeline);
        std::printf("{\"operation\":\"activation_timeout_sample\",\"captured\":%s,\"instruction\":%lu,\"timeline_base\":%lu,\"stack_timeline_rvas\":[",
            captured ? "true" : "false", context.Eip, base);
        bool first = true;
        for (SIZE_T index = 0; index < bytes / sizeof(DWORD); ++index) {
            if (stack[index] >= base + 0x1000 && stack[index] < base + 0x1c000) {
                std::printf("%s%lu", first ? "" : ",", stack[index] - base); first = false;
            }
        }
        std::puts("]}"); std::fflush(stdout);
        return 0;
    }
};

static LONG CALLBACK log_exception(EXCEPTION_POINTERS* pointers) {
    const auto* record = pointers->ExceptionRecord;
    std::printf("{\"operation\":\"native_exception\",\"code\":%lu,\"address\":%lu,\"parameter_count\":%lu,\"parameter0\":%lu,\"parameter1\":%lu}\n",
        record->ExceptionCode, reinterpret_cast<DWORD>(record->ExceptionAddress), record->NumberParameters,
        record->NumberParameters ? record->ExceptionInformation[0] : 0,
        record->NumberParameters > 1 ? record->ExceptionInformation[1] : 0);
    std::fflush(stdout);
    return EXCEPTION_CONTINUE_SEARCH;
}

static bool report(const char* operation, HRESULT hr) {
    std::printf("{\"operation\":\"%s\",\"hresult\":\"0x%08lX\"}\n", operation, static_cast<unsigned long>(hr));
    std::fflush(stdout); return SUCCEEDED(hr);
}

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) return 2;
    // Use the documented ATL compatibility mode while keeping DEP enabled.
    // Never call SetProcessDEPPolicy(0), change machine policy, or patch the DLL.
    const BOOL enabled = SetProcessDEPPolicy(PROCESS_DEP_ENABLE);
    const DWORD policyError = enabled ? ERROR_SUCCESS : GetLastError();
    DWORD flags = 0; BOOL permanent = FALSE;
    const BOOL queried = GetProcessDEPPolicy(GetCurrentProcess(), &flags, &permanent);
    std::printf("{\"operation\":\"dep_policy\",\"set_succeeded\":%s,\"error\":%lu,\"query_succeeded\":%s,\"flags\":%lu,\"permanent\":%s}\n",
        enabled ? "true" : "false", policyError, queried ? "true" : "false", flags, permanent ? "true" : "false");
    if (!enabled || !queried || flags != PROCESS_DEP_ENABLE || !permanent) return 1;
    const auto exceptionHandler = AddVectoredExceptionHandler(1, log_exception);
    if (!report("ole_initialize", OleInitialize(nullptr))) return 1;
    bool ok = true;
    {
        ReferenceTimeline timeline;
        auto* site = new HiddenOleSite;
        IOleObject* object = nullptr;
        IOleInPlaceObject* inPlace = nullptr;
        ok = site->window() && report("load_timeline", timeline.load(argv[1], argv[1]));
        if (ok) ok = report("query_ole_object", timeline.query(IID_IOleObject, reinterpret_cast<void**>(&object)));
        if (ok) ok = report("set_client_site", object->SetClientSite(site));
        if (ok) {
            IPersistStreamInit* persist = nullptr;
            ok = report("query_persist_init", object->QueryInterface(IID_IPersistStreamInit, reinterpret_cast<void**>(&persist)));
            if (persist) { ok = report("init_new", persist->InitNew()) && ok; persist->Release(); }
        }
        if (ok) {
            ok = report("set_length_before_activation", timeline.set_length(30720)) && ok;
            ok = report("set_zoom_before_activation", timeline.set_zoom(.125)) && ok;
            RECT rect = site->rectangle();
            ActivationDiagnostic diagnostic;
            ok = report("in_place_activate", object->DoVerb(OLEIVERB_INPLACEACTIVATE, nullptr, site, 0, site->window(), &rect));
        }
        if (ok) ok = report("query_in_place", object->QueryInterface(IID_IOleInPlaceObject, reinterpret_cast<void**>(&inPlace)));
        if (ok) {
            HWND child = nullptr;
            ok = report("get_control_window", inPlace->GetWindow(&child));
            const bool valid = child && IsWindow(child) && IsChild(site->window(), child);
            std::printf("{\"operation\":\"control_window\",\"valid_child\":%s,\"parent_visible\":%s}\n",
                valid ? "true" : "false", IsWindowVisible(site->window()) ? "true" : "false");
            ok = valid && !IsWindowVisible(site->window()) && ok;
            ok = report("set_length", timeline.set_length(30720)) && ok;
            ok = report("set_zoom", timeline.set_zoom(.125)) && ok;
            for (const LONG pixels : {0L, 192L, 500L, 0L}) {
                ok = report("set_horizontal_scroll", timeline.set_horizontal_scroll(pixels)) && ok;
                LONG start = -1;
                ok = report("get_visible_start", timeline.get_marker(3, &start)) && ok;
                VARIANT scroll{};
                const HRESULT hr = timeline.get_property(9, &scroll);
                report("get_horizontal_scroll", hr);
                std::printf("{\"operation\":\"scroll_readback\",\"requested_pixels\":%ld,\"visible_start_clocks\":%ld,\"property_type\":%u,\"property_value\":%ld}\n",
                    pixels, start, scroll.vt, scroll.lVal);
                VariantClear(&scroll);
            }
        }
        if (inPlace) { report("in_place_deactivate", inPlace->InPlaceDeactivate()); inPlace->Release(); }
        if (object) { report("close_object", object->Close(OLECLOSE_NOSAVE)); report("clear_client_site", object->SetClientSite(nullptr)); object->Release(); }
        site->Release();
        ok = report("timeline_unload", timeline.close()) && ok;
    }
    OleUninitialize();
    if (exceptionHandler) RemoveVectoredExceptionHandler(exceptionHandler);
    return ok ? 0 : 1;
}
