#include "os_core_win32.h"

global HCURSOR win32_hcursor;
global OS_Key keycode_table[256];
global s64 win32_performance_frequency;

internal OS_Event_Flags os_event_flags() {
    OS_Event_Flags result = (OS_Event_Flags)0;
    if (GetKeyState(VK_MENU)  & 0x8000) result |= OS_EVENT_FLAG_ALT; 
    if (GetKeyState(VK_CONTROL) & 0x8000) result |= OS_EVENT_FLAG_CONTROL;
    if (GetKeyState(VK_SHIFT) & 0x8000)   result |= OS_EVENT_FLAG_SHIFT;
    
    return result;
}

internal OS_Key os_key_from_vk(u32 vk) {
    local_persist bool first_call = true;
    if (first_call) {
        for (int i = 0; i < 26; i++) {
            keycode_table['A' + i] = (OS_Key)(OS_KEY_A + i);
        }
        for (int i = 0; i < 10; i++) {
            keycode_table['0' + i] = (OS_Key)(OS_KEY_0 + i);
        }
        keycode_table[VK_ESCAPE] = OS_KEY_ESCAPE;
        keycode_table[VK_HOME] = OS_KEY_HOME;
        keycode_table[VK_END] = OS_KEY_END;
        keycode_table[VK_SPACE] = OS_KEY_SPACE;
        keycode_table[VK_OEM_COMMA] = OS_KEY_COMMA;
        keycode_table[VK_OEM_PERIOD] = OS_KEY_PERIOD;
        keycode_table[VK_RETURN] = OS_KEY_ENTER;
        keycode_table[VK_BACK] = OS_KEY_BACKSPACE;
        keycode_table[VK_DELETE] = OS_KEY_DELETE;
        keycode_table[VK_LEFT] = OS_KEY_LEFT;
        keycode_table[VK_RIGHT] = OS_KEY_RIGHT;
        keycode_table[VK_UP] = OS_KEY_UP;
        keycode_table[VK_DOWN] = OS_KEY_DOWN;
        keycode_table[VK_PRIOR] = OS_KEY_PAGEUP;
        keycode_table[VK_NEXT] = OS_KEY_PAGEDOWN;
        keycode_table[VK_OEM_4] = OS_KEY_OPENBRACKET;
        keycode_table[VK_OEM_6] = OS_KEY_CLOSEBRACKET;
        keycode_table[VK_OEM_1] = OS_KEY_SEMICOLON;
        keycode_table[VK_OEM_2] = OS_KEY_SLASH;
        keycode_table[VK_OEM_5] = OS_KEY_BACKSLASH;
        keycode_table[VK_OEM_MINUS] = OS_KEY_MINUS;
        keycode_table[VK_OEM_PLUS] = OS_KEY_PLUS;
        keycode_table[VK_OEM_3] = OS_KEY_TICK;
        keycode_table[VK_OEM_7] = OS_KEY_QUOTE;
        for (int i = 0; i < 12; i++) {
            keycode_table[VK_F1 + i] = (OS_Key)(OS_KEY_F1 + i);
        }
    }
    return keycode_table[vk];
}

internal inline s64 get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return (s64)result.QuadPart;
}

internal inline f32 get_ms_elapsed(s64 start, s64 end) {
    f32 result = 1000.0f * ((f32)(end - start) / (f32)win32_performance_frequency);
    return result;
}
 
internal bool os_chdir(String8 path) {
    BOOL result = SetCurrentDirectory((LPCSTR)path.data);
    return result;
}

internal v2 os_get_window_dim(OS_Handle window_handle) {
    v2 result{};
    RECT rect;
    int width = 0, height = 0;
    if (GetClientRect((HWND)window_handle, &rect)) {
        result.x = (f32)(rect.right - rect.left);
        result.y = (f32)(rect.bottom - rect.top);
    }
    return result;
}

internal bool os_file_exists(String8 file_name) {
    bool result = PathFileExistsA((LPCSTR)file_name.data);
    return result;
}

internal bool os_valid_handle(OS_Handle handle) {
    bool result = (HANDLE)handle != INVALID_HANDLE_VALUE;
    return result;
}

internal OS_Handle os_open_file(String8 file_name, OS_Access_Flags flags) {
    DWORD access = 0, shared = 0, creation = 0;
    if (flags & OS_ACCESS_READ)    access  |= GENERIC_READ;
    if (flags & OS_ACCESS_WRITE)   access  |= GENERIC_WRITE;
    if (flags & OS_ACCESS_EXECUTE) access  |= GENERIC_EXECUTE;
    if (flags & OS_ACCESS_READ)    shared   = FILE_SHARE_READ;
    if (flags & OS_ACCESS_WRITE)   shared   = FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    if (flags & OS_ACCESS_READ)    creation = OPEN_EXISTING;
    if (flags & OS_ACCESS_WRITE)   creation = CREATE_ALWAYS;
    if (flags & OS_ACCESS_APPEND)  creation = OPEN_ALWAYS;

    HANDLE file_handle = CreateFileA((LPCSTR)file_name.data, access, shared, NULL, creation, FILE_ATTRIBUTE_NORMAL, NULL);
    OS_Handle handle = (OS_Handle)file_handle;
    return handle;
}

internal void os_write_file(OS_Handle file_handle, void *data, u64 size) {
    DWORD bytes_written = 0;
    if (WriteFile((HANDLE)file_handle, data, (DWORD)size, &bytes_written, NULL)) {
    } else {
        DWORD error = GetLastError();
        printf("WriteFile: error writing file %d\n", error);
    }
}

internal u64 os_read_entire_file(OS_Handle file_handle, void **out_data) {
    void *data = NULL;
    u64 size = 0;
    if ((HANDLE)file_handle != INVALID_HANDLE_VALUE) {
        u64 bytes_to_read;
        if (GetFileSizeEx((HANDLE)file_handle, (PLARGE_INTEGER)&bytes_to_read)) {
            Assert(bytes_to_read <= UINT32_MAX);
            data = (u8 *)malloc(bytes_to_read);
            DWORD bytes_read;
            if (ReadFile((HANDLE)file_handle, data, (DWORD)bytes_to_read, &bytes_read, NULL) && (DWORD)bytes_to_read ==  bytes_read) {
                *out_data = data;
                size = (u64)bytes_read;
            } else {
                //@Todo Error handling
            }
        } else {
            //@Todo Error handling
        }
    } else {
        //@Todo Error handling
    }
    return size;
}

internal String8 os_read_file_string(OS_Handle file_handle) {
    u8 *data = NULL;
    u64 size = 0;
    if ((HANDLE)file_handle != INVALID_HANDLE_VALUE) {
        u64 bytes_to_read;
        if (GetFileSizeEx((HANDLE)file_handle, (PLARGE_INTEGER)&bytes_to_read)) {
            Assert(bytes_to_read <= UINT32_MAX);
            data = (u8 *)malloc(bytes_to_read + 1);
            DWORD bytes_read;
            if (ReadFile((HANDLE)file_handle, data, (DWORD)bytes_to_read, &bytes_read, NULL) && (DWORD)bytes_to_read ==  bytes_read) {
                size = (u64)bytes_read;
                data[bytes_read] = 0;
            } else {
                //@Todo Error handling
            }
        } else {
            //@Todo Error handling
        }
    } else {
        //@Todo Error handling
    }
    String8 result = str8(data, size);
    return result;
}

internal void os_close_handle(OS_Handle handle) {
    if ((HANDLE)handle != INVALID_HANDLE_VALUE) {
        CloseHandle((HANDLE)handle);
    }
}

internal void os_quit(int exit_code) {
    PostQuitMessage(exit_code);
}

internal void os_local_time(int *hour, int *minute, int *second) {
    SYSTEMTIME system_time;
    GetLocalTime(&system_time);
    if (hour) *hour = system_time.wHour;
    if (minute) *minute = system_time.wMinute; 
    if (second) *second = system_time.wSecond; 
}

internal OS_File os_file_from_win32_data(Arena *arena, WIN32_FIND_DATAA win32_data) {
    OS_File file{};
    file.file_name = str8_copy(arena, str8_cstring(win32_data.cFileName));
    file.file_size = ((u64)win32_data.nFileSizeHigh<<32) | win32_data.nFileSizeLow;
    OS_File_Flags flags = OS_FILE_NIL;
    DWORD attribs = win32_data.dwFileAttributes;
    if (attribs & FILE_ATTRIBUTE_READONLY)    flags |= OS_FILE_READONLY;
    if (attribs & FILE_ATTRIBUTE_HIDDEN)      flags |= OS_FILE_HIDDEN;
    if (attribs & FILE_ATTRIBUTE_SYSTEM)      flags |= OS_FILE_SYSTEM;
    if (attribs & FILE_ATTRIBUTE_DIRECTORY)   flags |= OS_FILE_DIRECTORY;
    if (attribs & FILE_ATTRIBUTE_NORMAL)      flags |= OS_FILE_NORMAL;
    file.flags = flags;
    return file;
}

internal OS_Handle os_find_first_file(Arena *arena, String8 path, OS_File *file) {
    String8 find_path = str8_concat(arena, path, str8_lit("\\*"));
    WIN32_FIND_DATAA win32_data;
    HANDLE find_file_handle = FindFirstFileA((const char *)find_path.data, &win32_data);
    if (find_file_handle) {
        *file = os_file_from_win32_data(arena, win32_data);
    } else {
        DWORD err = GetLastError();
        printf("find_first_file ERROR '%d'\n", err);
    }
    return (OS_Handle)find_file_handle;
}

internal bool os_find_next_file(Arena *arena, OS_Handle find_file_handle, OS_File *file) {
    if ((HANDLE)find_file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    WIN32_FIND_DATAA win32_data;
    if (FindNextFileA((HANDLE)find_file_handle, &win32_data)) {
        *file = os_file_from_win32_data(arena, win32_data);
        return true;
    } else {
        DWORD err = GetLastError();
        if (err != ERROR_NO_MORE_FILES) {
            printf("find_next_file ERROR '%d'\n", err);
        }
        return false;
    }
}

internal void os_find_close(OS_Handle find_file_handle) {
    FindClose((HANDLE)find_file_handle);
}

internal bool os_path_exists(String8 path) {
    return PathFileExistsA((char *)path.data);
}

internal String8 os_home_path(Arena *arena) {
    char buffer[MAX_PATH];
    GetEnvironmentVariableA("USERPROFILE", buffer, MAX_PATH);
    String8 result = str8_copy(arena, str8_cstring(buffer));
    return result;
}

internal String8 os_current_dir(Arena *arena) {
    DWORD length = GetCurrentDirectoryA(0, NULL);
    u8 *buffer = (u8 *)arena_push(arena, length + 2);
    DWORD ret = GetCurrentDirectoryA(length, (LPSTR)buffer);
    for (DWORD i = 0; i < length; i++) {
        if (buffer[i] == '\\') buffer[i] = '/';
    }
    buffer[ret] = '/';
    String8 result = str8(buffer, (u64)ret + 1);
    return result;
}

internal String8 os_get_clipboard_text(Arena *arena) {
    String8 result = str8_zero();
    if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(0)) {
        HANDLE handle = GetClipboardData(CF_TEXT);
        if (handle) {
            u8 *buffer = (u8 *)GlobalLock(handle);
            if (buffer) {
                result = str8_copy(arena, str8_cstring((char *)buffer));
                GlobalUnlock(handle);
            }
        }
        CloseClipboard();
    }
    return result;
}

internal void os_set_clipboard_text(String8 text) {
    if (OpenClipboard(0)) {
        HANDLE handle = GlobalAlloc(GMEM_MOVEABLE, text.count + 1);
        if (handle) {
            u8 *buffer = (u8 *)GlobalLock(handle);
            MemoryCopy(buffer, text.data, text.count);
            buffer[text.count] = 0;
            GlobalUnlock(handle);
            SetClipboardData(CF_TEXT, handle);
        }
    }
}

internal void os_set_cursor(OS_Cursor cursor) {
    local_persist HCURSOR hcursor;
    switch (cursor) {
    default:
    case OS_CURSOR_ARROW:
        hcursor = LoadCursorA(NULL, IDC_ARROW);
        break;
    case OS_CURSOR_IBEAM:
        hcursor = LoadCursorA(NULL, IDC_IBEAM);
        break;
    case OS_CURSOR_HAND:
        hcursor = LoadCursorA(NULL, IDC_HAND);
        break;
    case OS_CURSOR_SIZE_NS:
        hcursor = LoadCursorA(NULL, IDC_SIZENS);
        break;
    case OS_CURSOR_SIZE_WE:
        hcursor = LoadCursorA(NULL, IDC_SIZEWE);
        break;
    }
    
    if (win32_hcursor != hcursor) {
        PostMessageA(0, WM_SETCURSOR, 0, 0);
        win32_hcursor = hcursor;
    }
}

internal Rect os_client_rect_from_window(OS_Handle window_handle) {
    RECT client_rect;
    GetClientRect((HWND)window_handle, &client_rect);
    Rect result;
    result.x0 = (f32)client_rect.left;
    result.x1 = (f32)client_rect.right;
    result.y0 = (f32)client_rect.top;
    result.y1 = (f32)client_rect.bottom;
    return result;
}

internal v2 os_mouse_from_window(OS_Handle window_handle) {
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient((HWND)window_handle, &pt);
    v2 result;
    result.x = (f32)pt.x;
    result.y = (f32)pt.y;
    return result;
}

internal bool os_window_is_focused(OS_Handle window_handle) {
    HWND active_hwnd = GetActiveWindow();
    bool result = (OS_Handle)active_hwnd == window_handle;
    return result;
}
