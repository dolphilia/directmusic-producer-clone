#include "script_runtime.h"
#include "source_script_host.h"
#include <stdexcept>
#include <sstream>
#include <dmerror.h>
#include <atomic>
#include <cstring>
#include <utility>
namespace producer::app {
namespace {
std::atomic<std::uint64_t> diagnosticSequence{0};
void valid_name(const std::wstring& n){if(n.empty()||n.find(L'\0')!=std::wstring::npos)throw std::runtime_error("Script name is empty or contains NUL");}
std::vector<std::wstring> enumerate(runtime::Script* script,bool routines){
    if(!script)throw std::runtime_error("No initialized Script");std::vector<std::wstring> values;
    for(DWORD i=0;i<4096;++i){WCHAR name[260]{};const auto hr=routines?script->EnumRoutine(i,name):script->EnumVariable(i,name);
        if(hr==S_FALSE)return values;if(hr!=S_OK&&hr!=DMUS_S_GARBAGE_COLLECTED)throw std::runtime_error("Script enumeration returned failure or truncated name");
        size_t n=0;while(n<260&&name[n])++n;if(!n||n==260)throw std::runtime_error("Script enumeration returned invalid name");values.emplace_back(name,n);}
    throw std::runtime_error("Script enumeration exceeded bound");
}
}
ScriptSession::ScriptSession(){begin();}
ScriptSession::~ScriptSession(){if(script_)script_->Release();if(loader_)loader_->Release();}
void ScriptSession::begin(){last_={};last_.error.size=sizeof(last_.error);}
ScriptResult ScriptSession::finish(const std::wstring& operation,const std::wstring& name){if(!last_.passed()){if(diagnostics_.size()<4096)diagnostics_.push_back({operation,name,last_,++diagnosticSequence});else diagnosticOverflow_=true;}return last_;}
ScriptResult ScriptSession::load(const Bytes& bytes,const std::wstring& directory,runtime::Performance* performance,std::vector<ScriptRuntimeDependency> dependencies){
    if(script_||loader_)throw std::runtime_error("Script session already loaded");ScriptDocument document;document.load(bytes);
    if(!performance)throw std::runtime_error("Script requires initialized Performance");
    const bool sourceHost=uses_source_script_host(document);memory_=source_script_loader_bytes(document);dependencies_=std::move(dependencies);
    begin();
    last_.result=CoCreateInstance(runtime::loaderClass,nullptr,CLSCTX_INPROC_SERVER,runtime::loader8Id,reinterpret_cast<void**>(&loader_));if(FAILED(last_.result))return finish(L"Initialize",document.name());
    last_.result=loader_->EnableCache(runtime::allTypes,TRUE);if(FAILED(last_.result))return finish(L"Initialize",document.name());
    if(!directory.empty()){auto search=directory;last_.result=loader_->SetSearchDirectory(runtime::allTypes,search.data(),TRUE);if(FAILED(last_.result))return finish(L"Initialize",document.name());}
    // Child-first descriptors refer only to owned memory/GUID, never the
    // serialized filename. SetObject registers metadata; do not GetObject
    // here, so NOLOADS and source alias.Load still decide realization.
    for(auto& dependency:dependencies_){runtime::ObjectDesc d{};d.size=sizeof(d);d.valid=1|2|1024;std::memcpy(&d.classId,dependency.classId.data(),16);std::memcpy(&d.objectId,dependency.objectId.data(),16);d.memoryLength=static_cast<LONGLONG>(dependency.runtimeBytes.size());d.memory=dependency.runtimeBytes.data();
        last_.result=loader_->SetObject(&d);if(last_.result!=S_OK){if(SUCCEEDED(last_.result))last_.result=E_FAIL;return finish(L"Register dependency",dependency.path);}}
    runtime::ObjectDesc desc{};desc.size=sizeof(desc);desc.valid=2|1024;desc.classId=runtime::scriptClass;desc.memoryLength=static_cast<LONGLONG>(memory_.size());desc.memory=memory_.data();
    last_.result=loader_->GetObject(&desc,runtime::scriptId,reinterpret_cast<void**>(&script_));if(last_.result!=S_OK||!script_){if(SUCCEEDED(last_.result))last_.result=E_FAIL;return finish(L"Initialize",document.name());}
    last_.result=script_->Init(performance,&last_.error);
    if(last_.passed()&&sourceHost){runtime::Script* host=nullptr;last_.result=create_source_script_host(document,script_,performance,&host,&last_.error);if(last_.passed()){script_->Release();script_=host;}}
    initialized_=last_.passed();return finish(L"Initialize",document.name());
}
ScriptResult ScriptSession::call(const std::wstring& name){valid_name(name);if(!initialized_)throw std::runtime_error("No initialized Script");auto n=name;begin();last_.result=script_->CallRoutine(n.data(),&last_.error);return finish(L"Call routine",name);}
ScriptResult ScriptSession::set_number(const std::wstring& name,LONG value){valid_name(name);if(!initialized_)throw std::runtime_error("No initialized Script");auto n=name;begin();last_.result=script_->SetVariableNumber(n.data(),value,&last_.error);return finish(L"Set number",name);}
ScriptResult ScriptSession::get_number(const std::wstring& name,LONG& value){valid_name(name);if(!initialized_)throw std::runtime_error("No initialized Script");auto n=name;LONG pending=0;begin();last_.result=script_->GetVariableNumber(n.data(),&pending,&last_.error);if(last_.passed())value=pending;return finish(L"Get number",name);}
ScriptResult ScriptSession::get_object(const std::wstring& name,REFIID id,void*& value){valid_name(name);if(!initialized_)throw std::runtime_error("No initialized Script");auto n=name;void* pending=nullptr;begin();last_.result=script_->GetVariableObject(n.data(),id,&pending,&last_.error);
    if(last_.passed())value=pending;else if(pending)reinterpret_cast<IUnknown*>(pending)->Release();return finish(L"Get object",name);}
std::vector<std::wstring> ScriptSession::routines()const{return enumerate(initialized_?script_:nullptr,true);}
std::vector<std::wstring> ScriptSession::variables()const{return enumerate(initialized_?script_:nullptr,false);}
}
