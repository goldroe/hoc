#include "path.h"

#define IS_SLASH(C) ((C) == '/' || (C) == '\\')

internal String8 path_strip_extension(Arena *arena, String8 path) {
    Assert(path.data);
    for (u64 i = path.count - 1; i > 0; i--) {
        switch (path.data[i]) {
        case '.':
        {
            String8 result = str8_copy(arena, str8(path.data + i + 1, path.count - i - 1));
            return result;
        }
        case '/':
        case '\\':
            //@Note no extension
            return str8_zero();
        }
    }
    return str8_zero();
}

internal String8 path_strip_dir_name(Arena *arena, String8 path) {
    Assert(path.data);
    u64 end = path.count;
    while (end) {
        if (IS_SLASH(path.data[end - 1])) break;
        end--;
    }
    String8 result = str8_copy(arena, {path.data, end});
    return result;
}

internal String8 path_strip_file_name(Arena *arena, String8 path) {
    Assert(path.data);
    String8 result = str8_zero();
    if (IS_SLASH(path.data[path.count - 1])) {
        return result;
    }
    u64 start = path.count - 1;
    while (start > 0) {
        if (IS_SLASH(path.data[start])) {
            result = str8_copy(arena, str8(path.data + start + 1, path.count - start - 1));
            break;
        }
        start--;
    }
    return result;
}

internal String8 path_normalize(Arena *arena, String8 path) {
    //@Todo should we append current directory if the path is relative?
    // Might help with dot2 being clamped to root of path
    Assert(path.data);
    uintptr_t len = 0;
    u8 *buffer = (u8 *)malloc(path.count + 1);
    u8 *stream = path.data;
    while (*stream) {
        if (IS_SLASH(*stream)) {
            stream++;
            u8 *start = stream;
            bool dot = false;
            bool dot2 = false;
            if (*stream == '.') {
                stream++;
                if (*stream == '.') {
                    dot2 = true;
                    stream++;
                } else {
                    dot = true;
                }
            }

            // Check if the current dirname is actually '..' or '.' rather than some file like '.foo' or '..foo'
            if (*stream && !IS_SLASH(*stream)) {
                dot = false;
                dot2 = false;
            }

            // If not a dot dirname then just continue on like normal, reset stream to after the seperator
            if (!dot && !dot2) {
                // buffer[len++] = '/';
                stream = start;
            } else if (dot2) {
                // Walk back directory, go until seperator or beginning of stream
                // @todo probably want to just clamp to root
                u8 *prev = buffer + len - 1;
                while (prev != buffer && !IS_SLASH(*prev)) {
                    prev--;
                }
                len = prev - buffer;
            }
        } else {
            buffer[len++] = *stream++;
        }
    }

    String8 result = str8_copy(arena, str8(buffer, len));
    free(buffer);
    return result;
}

#ifdef _WIN32
internal String8 path_home_name(Arena *arena) {
    char buffer[MAX_PATH];
    GetEnvironmentVariableA("USERPROFILE", buffer, MAX_PATH);
    String8 result = str8_copy(arena, str8_cstring(buffer));
    return result;
}
#elif defined(__linux__)
internal String8 path_home_name(Arena *arena) {
    char *c = getenv("HOME");
    if (c == NULL) {
        c = getpwuid(getuid())->pw_dir;
    } 
    String8 result = str8_copy(arena, str8_cstring(c));
    return result;
}
#endif

#ifdef _WIN32
internal String8 path_current_dir(Arena *arena) {
    DWORD length = GetCurrentDirectoryA(0, NULL);
    u8 *buffer = (u8 *)arena_push(arena, length + 2);
    DWORD ret = GetCurrentDirectoryA(length, (LPSTR)buffer);
    buffer[ret] = '\\';
    String8 result = str8(buffer, (u64)ret + 1);
    return result;
}
#elif defined(__linux__)
internal String8 path_current_dir(Arena *arena) {
    char *c = get_current_dir_name();
    String8 result = str8_cstring(c);
    return result;
}
#endif

#ifdef _WIN32
internal bool path_file_exists(String8 path) {
    return PathFileExistsA((char *)path.data);
}
#endif

internal bool path_is_absolute(String8 path) {
    switch (path.data[0]) {
#if defined(__linux__)
    case '~':
#endif
    case '\'':
    case '/':
        return true;
    }
    if (isalpha(path.data[0]) && path.data[1] == ':') return true;
    return false;
}

internal bool path_is_relative(String8 path) {
    return !path_is_absolute(path);
}

internal OS_Handle find_first_file(Arena *arena, String8 path, Find_File_Data *find_file_data) {
    String8 find_path = str8_concat(arena, path, str8_lit("/*"));
    WIN32_FIND_DATAA win32_data;
    HANDLE find_file_handle = FindFirstFileA((const char *)find_path.data, &win32_data);
    if (find_file_handle) {
        find_file_data->file_name = str8_copy(arena, str8_cstring(win32_data.cFileName));
    } else {
        DWORD err = GetLastError();
        printf("find_first_file ERROR '%d'\n", err);
    }
    return (OS_Handle)find_file_handle;
}

internal bool find_next_file(Arena *arena, OS_Handle find_file_handle, Find_File_Data *find_file_data) {
    if ((HANDLE)find_file_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    WIN32_FIND_DATAA win32_data;
    if (FindNextFileA((HANDLE)find_file_handle, &win32_data)) {
        find_file_data->file_name = str8_copy(arena, str8_cstring(win32_data.cFileName));
        return true;
    } else {
        DWORD err = GetLastError();
        if (err != ERROR_NO_MORE_FILES) {
            printf("find_next_file ERROR '%d'\n", err);
        }
        return false;
    }
}

internal void find_close(OS_Handle find_file_handle) {
    FindClose((HANDLE)find_file_handle);
}
