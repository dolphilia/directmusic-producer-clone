#pragma once
#include "script_document.h"
#include "compat/script_runtime.h"
namespace producer::app {
struct ScriptResult {HRESULT result=S_OK;runtime::ScriptErrorInfo error{};bool passed()const{return result!=S_FALSE&&SUCCEEDED(result);} };
struct ScriptDiagnostic {std::wstring operation,name;ScriptResult result;std::uint64_t sequence=0;};
// UI-thread owner: release before Performance/COM shutdown. Serialized source
// remains owned while Loader can refer to its memory descriptor.
class ScriptSession {
    bool initialized_=false;Bytes memory_;runtime::Loader* loader_=nullptr;runtime::Script* script_=nullptr;
    ScriptResult last_;std::vector<ScriptDiagnostic> diagnostics_;bool diagnosticOverflow_=false;
    ScriptResult finish(const std::wstring&,const std::wstring&);
    void begin();
public:
    ScriptSession();~ScriptSession();
    ScriptSession(const ScriptSession&)=delete;ScriptSession& operator=(const ScriptSession&)=delete;
    ScriptResult load(const Bytes&,const std::wstring&,runtime::Performance*);
    ScriptResult call(const std::wstring&);
    ScriptResult set_number(const std::wstring&,LONG);
    ScriptResult get_number(const std::wstring&,LONG&);
    // Success transfers one COM interface reference to the caller. Failure
    // leaves its output untouched; runtime values never become document data.
    ScriptResult get_object(const std::wstring&,REFIID,void*&);
    std::vector<std::wstring> routines()const;
    std::vector<std::wstring> variables()const;
    const std::vector<ScriptDiagnostic>& diagnostics()const{return diagnostics_;}
    bool diagnostic_overflow()const{return diagnosticOverflow_;}
    const ScriptResult& last_result()const{return last_;}
};
}
