#pragma once
#include <windows.h>
namespace command_probe {
struct Case {const char* name;WPARAM command;LONG position;bool all=false;};
inline constexpr Case cases[]={
    {"delete_empty_beat",0x8003,192},
    {"delete_occupied",0x8003,100},
    {"delete_notification",0x56788003,192},
    {"insert_selected",0x8004,100},
    {"insert_all_selected",0x8004,100,true},
    {"select_all",0xe12a,100},
    {"properties_selected",0x8000,100},
    {"right_move_command",0x8026,100},
    {"right_copy_command",0x8028,100},
    {"right_cancel_command",0x8027,100},
    {"unknown_command",0x7999,100},
};
}
