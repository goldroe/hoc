#ifndef OS_CORE_H
#define OS_CORE_H

#if OS_WINDOWS
#include "win32/os_core_win32.h"
#elif OS_LINUX
#include "linux/os_core_linux.h"
#endif

typedef u64 OS_Handle;

enum OS_Access_Flags {
    OS_ACCESS_READ     = (1<<0),
    OS_ACCESS_WRITE    = (1<<1),
    OS_ACCESS_EXECUTE  = (1<<2),
    OS_ACCESS_APPEND   = (1<<3),
};
EnumDefineFlagOperators(OS_Access_Flags);

enum OS_Key {
    OS_KEY_NIL,
    OS_KEY_A,
    OS_KEY_B,
    OS_KEY_C,
    OS_KEY_D,
    OS_KEY_E,
    OS_KEY_F,
    OS_KEY_G,
    OS_KEY_H,
    OS_KEY_I,
    OS_KEY_J,
    OS_KEY_K,
    OS_KEY_L,
    OS_KEY_M,
    OS_KEY_N,
    OS_KEY_O,
    OS_KEY_P,
    OS_KEY_Q,
    OS_KEY_R,
    OS_KEY_S,
    OS_KEY_T,
    OS_KEY_U,
    OS_KEY_V,
    OS_KEY_W,
    OS_KEY_X,
    OS_KEY_Y,
    OS_KEY_Z,

    OS_KEY_0,
    OS_KEY_1,
    OS_KEY_2,
    OS_KEY_3,
    OS_KEY_4,
    OS_KEY_5,
    OS_KEY_6,
    OS_KEY_7,
    OS_KEY_8,
    OS_KEY_9,

    OS_KEY_SPACE,
    OS_KEY_COMMA,
    OS_KEY_PERIOD,
    OS_KEY_QUOTE,

    OS_KEY_OPENBRACKET,
    OS_KEY_CLOSEBRACKET,
    OS_KEY_SEMICOLON,
    OS_KEY_SLASH,
    OS_KEY_BACKSLASH,
    OS_KEY_MINUS,
    OS_KEY_PLUS,

    OS_KEY_TAB,
    OS_KEY_TICK,

    OS_KEY_ESCAPE,
    OS_KEY_ENTER,
    OS_KEY_BACKSPACE,
    OS_KEY_DELETE,
    OS_KEY_LEFT,
    OS_KEY_RIGHT,
    OS_KEY_UP,
    OS_KEY_DOWN,

    OS_KEY_HOME,
    OS_KEY_END,
    OS_KEY_PAGEUP,
    OS_KEY_PAGEDOWN,

    OS_KEY_F1,
    OS_KEY_F2,
    OS_KEY_F3,
    OS_KEY_F4,
    OS_KEY_F5,
    OS_KEY_F6,
    OS_KEY_F7,
    OS_KEY_F8,
    OS_KEY_F9,
    OS_KEY_F10,
    OS_KEY_F11,
    OS_KEY_F12,

    OS_KEY_SUPER,

    OS_KEY_LEFTMOUSE,
    OS_KEY_MIDDLEMOUSE,
    OS_KEY_RIGHTMOUSE,

    OS_KEY_COUNT
};

enum OS_Cursor {
    OS_CURSOR_NIL,
    OS_CURSOR_ARROW,
    OS_CURSOR_IBEAM,
    OS_CURSOR_HAND,
    OS_CURSOR_SIZE_NS,
    OS_CURSOR_SIZE_WE,
};

enum OS_Event_Type {
    OS_EVENT_ERROR,
    OS_EVENT_PRESS,
    OS_EVENT_RELEASE,
    OS_EVENT_MOUSEMOVE,
    OS_EVENT_MOUSEUP,
    OS_EVENT_MOUSEDOWN,
    OS_EVENT_SCROLL,
    OS_EVENT_TEXT,
    OS_EVENT_COUNT
};

enum OS_Event_Flags {
    OS_EVENT_FLAG_CONTROL  = (1<<0),
    OS_EVENT_FLAG_ALT   = (1<<1),
    OS_EVENT_FLAG_SHIFT = (1<<2),
};
EnumDefineFlagOperators(OS_Event_Flags);

struct OS_Event {
    OS_Event *next;
    OS_Event_Type type = OS_EVENT_ERROR;
    OS_Event_Flags flags;
    OS_Key key;
    String8 text;
    v2i delta;
    v2i pos;
};

struct OS_Event_List {
    OS_Event *first;
    OS_Event *last;
    int count;
};

enum OS_File_Flags {
    OS_FILE_NIL          = 0,
    OS_FILE_READONLY     = (1<<0),
    OS_FILE_HIDDEN       = (1<<1),
    OS_FILE_SYSTEM       = (1<<2),
    OS_FILE_DIRECTORY    = (1<<3),
    OS_FILE_NORMAL       = (1<<4),
};
EnumDefineFlagOperators(OS_File_Flags)

struct OS_File {
    OS_File_Flags flags;
    String8 file_name;
    u64 file_size;
    u64 creation_time;
    u64 last_access_time;
    u64 last_write_time;
};

internal v2 os_get_window_dim(OS_Handle window_handle);
internal void os_quit_application(int exit_code);

internal OS_Handle os_find_first_file(Arena *arena, String8 path, OS_File *file);
internal bool os_find_next_file(Arena *arena, OS_Handle find_file_handle, OS_File *file);
internal void os_find_close(OS_Handle find_file_handle);



#endif // OS_CORE_H
