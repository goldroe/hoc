#include "os_core_win32.h"

global OS_Key keycode_table[256];

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
