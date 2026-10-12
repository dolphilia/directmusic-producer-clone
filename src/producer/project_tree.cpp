#include "project_tree.h"
#include <shellapi.h>
#include <algorithm>
#include <filesystem>
#include <stdexcept>

namespace producer::app {
namespace {
LRESULT CALLBACK tree_keyboard(HWND window,UINT message,WPARAM w,LPARAM l,UINT_PTR subclass,DWORD_PTR){
    if(message==WM_NCDESTROY)RemoveWindowSubclass(window,tree_keyboard,subclass);
    const auto result=DefSubclassProc(window,message,w,l);
    // IsDialogMessage otherwise treats Enter as a default-button command.
    // Request only this key so TreeView emits NM_RETURN; Tab/arrows retain
    // the standard control/dialog navigation rather than being intercepted.
    if(message==WM_GETDLGCODE&&w==VK_RETURN&&l){
        const auto input=reinterpret_cast<const MSG*>(l);
        if(input->message==WM_KEYDOWN)return result|DLGC_WANTMESSAGE;
    }
    return result;
}
std::wstring component_name(const Bytes& bytes,const std::wstring& path,const wchar_t* type,size_t index){
    // Reading display metadata does not normalize or write the owned RIFF.
    // Unknown/malformed optional names still allow the file to be selected.
    try{
        const auto root=Chunk::parse(bytes);
        if(const auto info=root.find("LIST","UNFO"))if(const auto name=info->find("UNAM")){
            const auto text=decode_utf16(name->data);if(!text.empty())return text;
        }
    }catch(const std::exception&){}
    return path.empty()?std::wstring(type)+std::to_wstring(index+1):std::filesystem::path(path).stem().wstring();
}
void add_stock_icon(HIMAGELIST images,SHSTOCKICONID id){
    SHSTOCKICONINFO info{};info.cbSize=sizeof(info);
    if(FAILED(SHGetStockIconInfo(id,SHGSI_ICON|SHGSI_SMALLICON,&info)))throw std::runtime_error("Project Tree stock icon unavailable");
    const auto index=ImageList_AddIcon(images,info.hIcon);DestroyIcon(info.hIcon);
    if(index<0)throw std::runtime_error("Project Tree icon creation failed");
}
}
void ProjectTree::create(HWND parent,UINT controlId,int width){
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_TREEVIEW_CLASSES};
    if(!InitCommonControlsEx(&controls))throw std::runtime_error("Project Tree control initialization failed");
    caption_=CreateWindowW(L"STATIC",L"Project Tree",WS_CHILD|WS_VISIBLE,4,3,width-8,20,parent,nullptr,nullptr,nullptr);
    tree_=CreateWindowExW(WS_EX_CLIENTEDGE,WC_TREEVIEWW,L"Project Tree",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TVS_HASBUTTONS|TVS_HASLINES|TVS_LINESATROOT|TVS_SHOWSELALWAYS|TVS_DISABLEDRAGDROP,
                         0,24,width,100,parent,reinterpret_cast<HMENU>(static_cast<UINT_PTR>(controlId)),nullptr,nullptr);
    if(!caption_||!tree_)throw std::runtime_error("Project Tree creation failed");
    if(!SetWindowSubclass(tree_,tree_keyboard,1,0))throw std::runtime_error("Project Tree keyboard initialization failed");
    for(const auto child:{caption_,tree_})SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
    images_=ImageList_Create(16,16,ILC_COLOR32|ILC_MASK,3,0);
    if(!images_)throw std::runtime_error("Project Tree image list unavailable");
    // Declared Windows Shell stock assets, never Producer PE icon resources.
    // Component-specific original icons remain a recorded appearance gap.
    add_stock_icon(images_,SIID_FOLDER);add_stock_icon(images_,SIID_DOCNOASSOC);add_stock_icon(images_,SIID_APPLICATION);
    TreeView_SetImageList(tree_,images_,TVSIL_NORMAL);
}
void ProjectTree::layout(int width,int height,bool visible,int top){
    if(!tree_)return;
    MoveWindow(caption_,4,top+3,std::max(1,width-8),20,TRUE);
    MoveWindow(tree_,0,top+24,width,std::max(1,height-top-24),TRUE);
    ShowWindow(caption_,visible?SW_SHOW:SW_HIDE);ShowWindow(tree_,visible?SW_SHOW:SW_HIDE);
}
bool ProjectTree::focus()const{
    if(!tree_||!IsWindowVisible(tree_)||!IsWindowEnabled(tree_))return false;
    SetFocus(tree_);return GetFocus()==tree_;
}
HTREEITEM ProjectTree::insert(HTREEITEM parent,const std::wstring& text,int image,size_t item){
    TVINSERTSTRUCTW node{};node.hParent=parent;node.hInsertAfter=TVI_LAST;
    node.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
    node.item.pszText=const_cast<wchar_t*>(text.c_str());node.item.iImage=node.item.iSelectedImage=image;node.item.lParam=static_cast<LPARAM>(item+1);
    const auto result=TreeView_InsertItem(tree_,&node);if(!result)throw std::runtime_error("Project Tree item creation failed");return result;
}
std::optional<ProjectTree::Key> ProjectTree::key(HTREEITEM node)const{
    if(!node)return {};TVITEMW item{};item.mask=TVIF_PARAM;item.hItem=node;
    if(!TreeView_GetItem(tree_,&item)||item.lParam<=0||static_cast<size_t>(item.lParam)>items_.size())return {};
    return items_[static_cast<size_t>(item.lParam)-1].key;
}
HTREEITEM ProjectTree::handle(const Key& key)const{
    const auto found=std::find_if(items_.begin(),items_.end(),[&](const auto& item){return item.key==key;});return found==items_.end()?nullptr:found->handle;
}
void ProjectTree::refresh(const Framework& framework,OpenedDocument active){
    if(!tree_)return;
    struct RefreshGuard {bool& value;explicit RefreshGuard(bool& flag):value(flag){value=true;}~RefreshGuard(){value=false;}} guard(refreshing_);
    std::vector<Item> next;
    next.push_back({{DocumentKind::Project,0,false},framework.project_path().empty()?L"Untitled":std::filesystem::path(framework.project_path()).stem().wstring()});
    const auto append=[&](const auto& documents,DocumentKind kind,const wchar_t* type){
        for(size_t index=0;index<documents.size();++index){const auto& opened=documents[index];const auto& doc=*opened.document;
            const auto file=opened.path.empty()?std::wstring(L"Untitled ")+type+L" "+std::to_wstring(index+1):std::filesystem::path(opened.path).filename().wstring();
            const auto name=component_name(doc.save_bytes(),opened.path,type,index);
            next.push_back({{kind,index,false},file+(doc.dirty()?L" *":L"")});next.push_back({{kind,index,true},name});
        }
    };
    append(framework.documents(),DocumentKind::Segment,L"Segment");append(framework.style_documents(),DocumentKind::Style,L"Style");append(framework.band_documents(),DocumentKind::Band,L"Band");
    for(size_t index=0;index<framework.collections().size();++index){const auto& opened=framework.collections()[index];
        const auto name=opened.path.empty()?L"Untitled DLS "+std::to_wstring(index+1):std::filesystem::path(opened.path).filename().wstring();
        next.push_back({{DocumentKind::Collection,index,false},name+(opened.document.dirty()?L" *":L"")});
        next.push_back({{DocumentKind::Collection,index,true},component_name(opened.document.save_bytes(),opened.path,L"DLS",index)});
    }
    append(framework.audio_paths(),DocumentKind::AudioPath,L"AudioPath");append(framework.chordmaps(),DocumentKind::ChordMap,L"ChordMap");append(framework.waves(),DocumentKind::Wave,L"Wave");
    append(framework.scripts(),DocumentKind::Script,L"Script");append(framework.containers(),DocumentKind::Container,L"Container");append(framework.tool_graphs(),DocumentKind::ToolGraph,L"ToolGraph");
    const Key activeKey{active.kind,active.index,true};const bool changedActive=!active_||!(*active_==activeKey);
    const bool changed=next.size()!=items_.size()||!std::equal(next.begin(),next.end(),items_.begin(),[](const auto& a,const auto& b){return a.key==b.key&&a.text==b.text;});
    if(changed){
        const auto caret=key(TreeView_GetSelection(tree_)),first=key(TreeView_GetFirstVisible(tree_));std::vector<Key> expanded;
        for(const auto& item:items_)if(TreeView_GetItemState(tree_,item.handle,TVIS_EXPANDED)&TVIS_EXPANDED)expanded.push_back(item.key);
        const bool initial=items_.empty();SendMessageW(tree_,WM_SETREDRAW,FALSE,0);TreeView_DeleteAllItems(tree_);items_=std::move(next);
        items_[0].handle=insert(TVI_ROOT,items_[0].text,0,0);
        HTREEITEM file=nullptr;for(size_t i=1;i<items_.size();++i){auto& item=items_[i];item.handle=insert(item.key.component?file:items_[0].handle,item.text,item.key.component?2:1,i);if(!item.key.component)file=item.handle;}
        if(initial)TreeView_Expand(tree_,items_[0].handle,TVE_EXPAND);else for(const auto& key:expanded)if(const auto item=handle(key))TreeView_Expand(tree_,item,TVE_EXPAND);
        if(caret)TreeView_SelectItem(tree_,handle(*caret));if(first)TreeView_SelectSetFirstVisible(tree_,handle(*first));
        SendMessageW(tree_,WM_SETREDRAW,TRUE,0);InvalidateRect(tree_,nullptr,TRUE);
    }
    for(const auto& item:items_)TreeView_SetItemState(tree_,item.handle,item.key==activeKey?TVIS_BOLD:0,TVIS_BOLD);
    if(changedActive)if(const auto item=handle(activeKey)){TreeView_SelectItem(tree_,item);TreeView_EnsureVisible(tree_,item);}
    active_=activeKey;
}
std::optional<OpenedDocument> ProjectTree::activated(const NMHDR& notification)const{
    if(!owns_notification(notification)||(notification.code!=NM_DBLCLK&&notification.code!=NM_RETURN))return {};
    auto item=TreeView_GetSelection(tree_);
    if(notification.code==NM_DBLCLK){TVHITTESTINFO hit{};GetCursorPos(&hit.pt);ScreenToClient(tree_,&hit.pt);item=TreeView_HitTest(tree_,&hit);if(!(hit.flags&(TVHT_ONITEMLABEL|TVHT_ONITEMICON)))return {};}
    const auto selected=key(item);if(!selected||!selected->component)return {};
    return OpenedDocument{selected->kind,selected->index};
}
std::optional<OpenedDocument> ProjectTree::selected_component()const{
    const auto selected=key(TreeView_GetSelection(tree_));
    if(!selected||!selected->component)return {};
    return OpenedDocument{selected->kind,selected->index};
}
std::optional<OpenedDocument> ProjectTree::selected_properties_target()const{
    const auto selected=key(TreeView_GetSelection(tree_));
    if(!selected||selected->kind==DocumentKind::Project)return {};
    return OpenedDocument{selected->kind,selected->index};
}
bool ProjectTree::context_point(POINT& screen){
    if(!tree_)return false;
    if(screen.x==-1&&screen.y==-1){
        const auto selected=TreeView_GetSelection(tree_);RECT rect{};
        if(!selected||!TreeView_GetItemRect(tree_,selected,&rect,TRUE))return false;
        screen={rect.left,rect.bottom};ClientToScreen(tree_,&screen);return true;
    }
    TVHITTESTINFO hit{};hit.pt=screen;ScreenToClient(tree_,&hit.pt);
    const auto item=TreeView_HitTest(tree_,&hit);
    if(!item||!(hit.flags&(TVHT_ONITEMLABEL|TVHT_ONITEMICON)))return false;
    TreeView_SelectItem(tree_,item);return true;
}
void ProjectTree::reset_context(){if(tree_)TreeView_DeleteAllItems(tree_);items_.clear();active_.reset();}
void ProjectTree::release(){tree_=caption_=nullptr;items_.clear();active_.reset();if(images_)ImageList_Destroy(images_);images_=nullptr;}
}
