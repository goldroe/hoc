#pragma once

internal String8 path_strip_extension(String8 path);
internal String8 path_strip_dir_name(String8 path);
internal String8 path_strip_file_name(String8 path);
internal String8 path_normalize(String8 path);
internal String8 path_home_name();
internal String8 path_current_dir();
internal bool path_file_exists(String8 path);
internal bool path_is_absolute(String8 path);
internal bool path_is_relative(String8 path);

internal OS_Handle find_first_file(Arena *arena, String8 path, Find_File_Data *find_file_data);
internal bool find_next_file(Arena *arena, OS_Handle find_file_handle, Find_File_Data *find_file_data);
internal void find_close(OS_Handle find_file_handle);
