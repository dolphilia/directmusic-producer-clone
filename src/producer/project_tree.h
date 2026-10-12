#pragma once
#include "framework.h"
#include <commctrl.h>
#include <optional>

namespace producer::app {
// The tree is a view of the Framework's owned documents. Its caret, expansion
// and visibility are editor context and must never enter document history.
class ProjectTree {
    struct Key {
        DocumentKind kind;size_t index;bool component;
        bool operator==(const Key& other)const{return kind==other.kind&&index==other.index&&component==other.component;}
    };
    struct Item {Key key;std::wstring text;HTREEITEM handle=nullptr;};
    HWND tree_=nullptr,caption_=nullptr;
    HIMAGELIST images_=nullptr;
    std::vector<Item> items_;
    std::optional<Key> active_;
    bool refreshing_=false;
    HTREEITEM insert(HTREEITEM parent,const std::wstring&,int image,size_t item);
    std::optional<Key> key(HTREEITEM)const;
    HTREEITEM handle(const Key&)const;
public:
    void create(HWND parent,UINT controlId,int width);
    void layout(int width,int height,bool visible,int top=0);
    void refresh(const Framework&,OpenedDocument active);
    void reset_context();
    void release();
    bool focus()const;
    bool owns(HWND child)const{return child==tree_||child==caption_;}
    bool owns_notification(const NMHDR& notification)const{return notification.hwndFrom==tree_;}
    std::optional<OpenedDocument> activated(const NMHDR&)const;
    std::optional<OpenedDocument> selected_component()const;
    // Both a native file node and its component identify the same property owner.
    std::optional<OpenedDocument> selected_properties_target()const;
    bool refreshing()const{return refreshing_;}
    // Mouse context selects the hit object; keyboard context uses the caret.
    bool context_point(POINT& screen);
};
}
