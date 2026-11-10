#pragma once
#include <windows.h>
#include <functional>
#include <cstdint>

namespace producer::app {
class ChordKeyboard {
    HWND window_=nullptr;
    bool scale_=false;
    unsigned caret_=0;
    std::uint64_t selected_=0,missing_=0;
    std::function<void(unsigned)> clicked_;
    static LRESULT CALLBACK proc(HWND,UINT,WPARAM,LPARAM);
    int hit(int x,int y)const;
    void paint()const;
public:
    void create(HWND owner,UINT id,bool scale,int x,int y,int width,int height,std::function<void(unsigned)> clicked);
    void set(std::uint64_t selected,std::uint64_t missing,bool enabled);
    void show(bool visible)const;
    void release(){window_=nullptr;clicked_={};}
};
}
