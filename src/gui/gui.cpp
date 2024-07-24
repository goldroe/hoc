
internal GUI_View *make_gui_view() {
    Base_Allocator *allocator = get_malloc_allocator();
    Arena *arena = arena_alloc(allocator, sizeof(GUI_View));
    GUI_View *view = (GUI_View *)push_array(arena, GUI_View, 1);
    view->arena = arena;
    view->id = hoc_app->view_id_counter++;
    view->prev = nullptr;
    view->next = nullptr;
    DLLPushBack(hoc_app->gui_views.first, hoc_app->gui_views.last, view, next, prev);
    hoc_app->gui_views.count += 1;
    return view;
}

internal GUI_View *make_gui_editor() {
    GUI_View *view = make_gui_view();
    view->type = GUI_VIEW_EDITOR;
    view->editor.editor = make_editor();
    return view;
}

internal GUI_View *make_gui_file_system() {
    GUI_File_System fs{};
    fs.arena = make_arena(get_malloc_allocator());
    fs.cached_files_arena = make_arena(get_malloc_allocator());
    GUI_View *view = make_gui_view();
    view->type = GUI_VIEW_FILE_SYSTEM;
    view->fs = fs;
    return view;
}

internal void remove_gui_view(GUI_View *view) {
    GUI_View_List *views = &hoc_app->gui_views;
    DLLRemove(views->first, views->last, view, next, prev);
    views->count -= 1;
}

internal void delete_gui_view(GUI_View *view) {
    remove_gui_view(view);
    arena_release(view->arena);
}

internal void destroy_gui_view(GUI_View *view) {
    view->to_be_destroyed = true;
}

internal void gui_editor_open_file(GUI_Editor *editor, String8 file_name) {
    editor->editor->buffer = make_buffer(file_name);
    editor->scroll_pt = 0.f;
}

internal Cursor editor_mouse_to_cursor(GUI_Editor *gui_editor, v2 mouse) {
    Hoc_Editor *editor = gui_editor->editor;
    //@Todo support non-mono fonts
    Cursor result{};
    f32 y = mouse.y - gui_editor->box->view_offset.y - gui_editor->box->rect.y0;
    f32 x = mouse.x - gui_editor->box->view_offset.x - gui_editor->box->rect.x0;
    s64 line = (s64)(y / gui_editor->box->font->glyph_height);
    s64 col = (s64)(x / gui_editor->box->font->glyph_width);
    if (line >= buffer_get_line_count(editor->buffer)) {
        result = get_cursor_from_position(editor->buffer, buffer_get_length(editor->buffer));
    } else {
        line = Clamp(line, 0, buffer_get_line_count(editor->buffer) - 1);
        col = Clamp(col, 0, buffer_get_line_length(editor->buffer, line));
        s64 position = get_position_from_line(editor->buffer, line) + col;
        position = ClampTop(position, get_position_from_line(editor->buffer, line + 1) - 1);
        result = get_cursor_from_position(editor->buffer, position);
    }
    return result;
}

internal u16 os_key_to_key_mapping(OS_Key key, OS_Event_Flags modifiers) {
    u16 result = (u16)key;
    result |= (modifiers & OS_EVENT_FLAG_ALT) ? KEY_MOD_ALT : 0;
    result |= (modifiers & OS_EVENT_FLAG_CONTROL) ? KEY_MOD_CONTROL : 0;
    result |= (modifiers & OS_EVENT_FLAG_SHIFT) ? KEY_MOD_SHIFT : 0;
    return result;
}

struct Editor_Draw_Data {
    Hoc_Editor *editor;
};

internal UI_BOX_CUSTOM_DRAW_PROC(draw_gui_editor) {
    Editor_Draw_Data *draw_data = (Editor_Draw_Data *)user_data;
    Hoc_Editor *editor = draw_data->editor;
    String8 string_before_cursor = box->string;
    string_before_cursor.count = editor->cursor.position;

    Face *font = box->font;

    // Draw_Clip(box->rect) {
        draw_rect(box->rect, box->background_color);

        v2 text_position = ui_text_position(box);
        text_position += box->view_offset;

        //@Note draw selection
        if (editor->mark_active) {
            v2 p0 = box->rect.p0 + box->view_offset;

            Cursor c0 = editor->cursor;
            Cursor c1 = editor->mark;
            if (c1.position < c0.position) {
                Swap(Cursor, c0, c1);
            }

            for (s64 line = c0.line; line <= c1.line; line += 1) {
                s64 start = 0;
                s64 end = buffer_get_line_length(editor->buffer, line);
                if (line == c0.line) {
                    start = c0.col;
                }
                if (line == c1.line) {
                    end = c1.col;
                }

                v2 p = p0;
                p.x += start * font->glyph_width;
                p.y += line * font->glyph_height;

                String8 line_string = str8_jump(box->string, get_position_from_line(editor->buffer, line) + start);
                line_string.count = end - start;

                f32 width = get_string_width(line_string, font);
                if (line_string.count == 0) width = font->glyph_width;
                f32 height = font->glyph_height;
                Rect rect = make_rect(p.x, p.y, width, height);
                draw_rect(rect, V4(.13f, .26f, .51f, 1.f));
            }
        }
        draw_string_truncated(box->string, font, box->text_color, text_position, box->rect);

        v2 cursor_pos = box->rect.p0 + measure_string_size(string_before_cursor, font) + box->view_offset;
        Rect cursor_rect = make_rect(cursor_pos.x, cursor_pos.y, 1.f, font->glyph_height);
        v4 cursor_color = box->text_color;
        // cursor_color *= (1.f - editor->cursor_dt);
        // editor->cursor_dt += ui_animation_dt();
        // editor->cursor_dt = ClampBot(editor->cursor_dt, 0.f);
        if (ui_key_match(box->key, ui_state->focus_active_box_key)) {
            cursor_rect.x1 += 1.f;
        }
        draw_rect(cursor_rect, cursor_color);
    // }
}

int fs_sort_compare__file_name(const void *arg1, const void *arg2) {
    OS_File *a = (OS_File *)arg1;
    OS_File *b = (OS_File *)arg2;
    int size = (int)Min(a->file_name.count, b->file_name.count);
    int result = strncmp((char *)a->file_name.data, (char *)b->file_name.data, size);
    return result;
}

int fs_sort_compare__default(const void *arg1, const void *arg2) {
    OS_File *a = (OS_File *)arg1;
    OS_File *b = (OS_File *)arg2;
    int result = 0;
    if (a->flags & OS_FILE_DIRECTORY && !(b->flags & OS_FILE_DIRECTORY)) {
        result = -1;
    } else if (b->flags & OS_FILE_DIRECTORY && !(a->flags & OS_FILE_DIRECTORY)) {
        result = 1;
    } else {
        result = fs_sort_compare__file_name(a, b);
    }
    return result;
}

internal void gui_file_system_load_path(GUI_File_System *fs, String8 full_path) {
    // String8 normalized_path = normalize_path(arena_alloc(get_malloc_allocator(), full_path.count + 1), full_path);

    MemoryCopy(fs->path_buffer, full_path.data, full_path.count + 1);
    fs->path_len = full_path.count;
    fs->path_pos = full_path.count;

    struct OS_File_Node {
        OS_File_Node *next;
        OS_File file;
    };

    Arena *scratch = make_arena(get_malloc_allocator());
    String8 find_path = str8_copy(scratch, str8(fs->path_buffer, fs->path_len));
    
    OS_File_Node *first = nullptr, *last = nullptr;
    int file_count = 0;
    OS_File file;
    OS_Handle find_handle = os_find_first_file(fs->arena, find_path, &file);
    if (os_valid_handle(find_handle)) {
        do {
            if (str8_match(file.file_name, str8_lit(".")) || str8_match(file.file_name, str8_lit(".."))) {
                continue;
            }
            OS_File_Node *file_node = push_array(scratch, OS_File_Node, 1);
            file_node->file = file;
            SLLQueuePush(first, last, file_node);
            file_count += 1;
        } while (os_find_next_file(fs->arena, find_handle, &file));
        os_find_close(find_handle);
    }

    arena_clear(fs->cached_files_arena);
    fs->cached_files = push_array(fs->cached_files_arena, OS_File, file_count);
    int file_idx = 0;
    for (OS_File_Node *node = first; node != nullptr; node = node->next, file_idx += 1) {
        fs->cached_files[file_idx] = node->file;
    }
    fs->cached_file_count = file_count;

    qsort(fs->cached_files, fs->cached_file_count, sizeof(OS_File), fs_sort_compare__default);
    
    arena_release(scratch);
}

internal void gui_view_update(GUI_View *view) {
    switch (view->type) {
    case GUI_VIEW_NIL:
        Assert(0);
        break;

    case GUI_VIEW_EDITOR:
    {
        GUI_Editor *gui_editor = &view->editor;
        Hoc_Editor *editor = gui_editor->editor;

        char *file_name = (char *)editor->buffer->file_name.data;

        //@Note Main editor container
        ui_set_next_child_layout(AXIS_X);
        ui_set_next_pref_width(ui_pct(1.f, 0.f));
        ui_set_next_pref_height(ui_pct(1.f, 0.f));
        UI_Box *container_body = ui_make_box_from_stringf(UI_BOX_NIL, "editor_container_%s", file_name);

        UI_Box *code_body = nullptr;
        UI_Signal signal{};

        Arena *scratch = make_arena(get_malloc_allocator());

        UI_Parent(container_body)
            UI_TextAlignment(UI_TEXT_ALIGN_LEFT)
            UI_Font(default_fonts[FONT_DEFAULT])
        {
            //@Note File Editor body
            ui_set_next_pref_width(ui_pct(1.f, 0.f));
            ui_set_next_pref_height(ui_pct(1.f, 0.f));
            ui_set_next_child_layout(AXIS_Y);
            UI_Box *editor_body = ui_make_box_from_stringf(UI_BOX_NIL, "edit_body_%s", file_name);

            UI_Parent(editor_body)
            {
                //@Note Code body
                ui_set_next_pref_width(ui_pct(1.f, 0.f));
                ui_set_next_pref_height(ui_pct(1.f, 0.f));
                ui_set_next_font(editor->face);
                code_body = ui_make_box_from_stringf(UI_BOX_CLICKABLE | UI_BOX_KEYBOARD_CLICKABLE | UI_BOX_SCROLL | UI_BOX_DRAW_BACKGROUND, "code_view_%d", view->id);
                signal = ui_signal_from_box(code_body);

                
                //@Note File bar
                ui_set_next_pref_width(ui_pct(1.f, 1.f));
                ui_set_next_pref_height(ui_px(editor->face->glyph_height, 1.f));
                ui_set_next_background_color(V4(.24f, .25f, .25f, 1.f));
                ui_set_next_child_layout(AXIS_X);
                String8 bottom_bar_string = str8_zero();
                UI_Box *bottom_bar = ui_make_box_from_stringf(UI_BOX_DRAW_BACKGROUND, "bottom_bar_%s", file_name);
                UI_Parent(bottom_bar)
                    UI_PrefWidth(ui_text_dim(2.f, 1.f))
                    UI_PrefHeight(ui_pct(1.f, 1.f))
                {
                    if (editor->modified) {
                        ui_set_next_text_color(V4(1.f, 0.f, 0.f, 1.f));
                        ui_label(str8_lit("*"));
                    }
                    UI_Signal sig = ui_label(editor->buffer->file_name);
                    ui_labelf("   (%lld,%lld)", editor->cursor.line, editor->cursor.col);

                    int hour, minute, second;
                    os_local_time(&hour, &minute, &second);
                    ui_labelf("%d:%d", hour, minute);
                }
            }

            ui_set_next_background_color(V4(.18f, .18f, .18f, 1.f));
            f32 ratio = rect_height(code_body->rect) / (buffer_get_line_count(editor->buffer) * editor->face->glyph_height);
            UI_Signal signal = ui_scroll_bar(editor->buffer->file_name, AXIS_Y, gui_editor->scroll_pt, buffer_get_line_count(editor->buffer) * editor->face->glyph_height);
            if (ui_clicked(signal)) {
                gui_editor->scroll_pt = ui_mouse().y;
            }
        }

        code_body->view_offset_target.y = -gui_editor->scroll_pt;

        gui_editor->active_text_input = signal.text;
        gui_editor->box = code_body;

        if (ui_pressed(signal)) {
            u16 key = os_key_to_key_mapping(signal.key, signal.key_modifiers);
            if (key && signal.key) {
                view->key_map->mappings[key].procedure(view);
            }
        }

        if (ui_clicked(signal)) {
            Cursor cursor = editor_mouse_to_cursor(gui_editor, ui_mouse());
            editor_set_cursor(editor, cursor);
            editor->mark = cursor;
        }

        if (ui_scroll(signal)) {
            gui_editor->scroll_pt -= editor->face->glyph_height * signal.scroll.y;
        }

        if (ui_dragging(signal)) {
            v2 mouse = ui_mouse();
            if (mouse.y <= code_body->rect.y0) {
                gui_editor->scroll_pt -= editor->face->glyph_height;
            }
            if (mouse.y >= code_body->rect.y1) {
                gui_editor->scroll_pt += editor->face->glyph_height;
            }
            Cursor cursor = editor_mouse_to_cursor(gui_editor, ui_mouse());
            editor_set_cursor(editor, cursor);
            editor->mark_active = editor->mark.position != editor->cursor.position;
        }

        code_body->string = buffer_to_string(ui_build_arena(), editor->buffer); 

        Editor_Draw_Data *editor_draw_data = push_array(ui_build_arena(), Editor_Draw_Data, 1);
        editor_draw_data->editor = editor;
        ui_set_custom_draw(code_body, draw_gui_editor, editor_draw_data);
        break;
    }

    case GUI_VIEW_FILE_SYSTEM:
    {
        GUI_File_System *fs = &view->fs;
        GUI_Editor *current_editor = fs->current_editor;
        
        v2 root_dim = rect_dim(current_editor->box->rect);
        v2 dim = V2(400.f, 600.f);
        v2 p = V2(.5f * root_dim.x - .5f * dim.x, 20.f);

        ui_set_next_text_color(V4(.4f, .4f, .4f, 1.f));
        ui_set_next_background_color(V4(1.f, 1.f, 1.f, 1.f));
        ui_set_next_border_color(V4(.4f, .4f, .4f, 1.f));
        ui_set_next_fixed_x(p.x);
        ui_set_next_fixed_y(p.y);
        ui_set_next_fixed_width(dim.x);
        ui_set_next_fixed_height(dim.y);
        ui_set_next_child_layout(AXIS_Y);
        UI_Box *fs_container = ui_make_box_from_stringf(UI_BOX_OVERFLOW_Y, "file_system_%d", view->id);

        Arena *scratch = make_arena(get_malloc_allocator());

        UI_Parent(fs_container)
            UI_TextAlignment(UI_TEXT_ALIGN_CENTER)
            UI_TextColor(V4(.8f, .73f, .59f, 1.f))
            UI_BackgroundColor(V4(.16f, .16f, .16f, 1.f))
            UI_BorderColor(V4(.2f, .2f, .2f, 1.f))
            UI_PrefWidth(ui_pct(1.f, 1.f))
            UI_PrefHeight(ui_text_dim(2.f, 0.f))
        {
            UI_Signal prompt_sig = ui_line_edit(str8_lit("fs_prompt_1"), fs->path_buffer, 2048, &fs->path_pos, &fs->path_len);
            String8 file_path = str8(fs->path_buffer, fs->path_len);
            
            if (ui_pressed(prompt_sig)) {
                if (prompt_sig.key == OS_KEY_SLASH || prompt_sig.key == OS_KEY_BACKSLASH) 
                    gui_file_system_load_path(fs, file_path);
            }
            if (prompt_sig.key == OS_KEY_BACKSPACE) {
                u8 last = fs->path_len > 0 ? fs->path_buffer[fs->path_len - 1] : 0;
                if (last == '\\' || last == '/') {
                    gui_file_system_load_path(fs, file_path);
                }
            }
            if (prompt_sig.key == OS_KEY_ENTER) {
                String8 file_path = str8_copy(scratch, str8(fs->path_buffer, fs->path_len));
                if (os_file_exists(file_path)) {
                    gui_editor_open_file(current_editor, file_path);
                    destroy_gui_view(view);
                }
            }
            if (prompt_sig.key == OS_KEY_ESCAPE) {
                destroy_gui_view(view);
            }

            UI_Signal back_sig = ui_button(str8_lit("*Previous Directory*"));
            if (ui_clicked(back_sig)) {
                String8 new_path = path_strip_dir_name(scratch, file_path);
                gui_file_system_load_path(fs, new_path);
            }
            for (int i = 0; i < fs->cached_file_count; i++) {
                OS_File file = fs->cached_files[i];
                UI_Signal button_sig;
                if (file.flags & OS_FILE_DIRECTORY) {
                    ui_set_next_text_color(V4(1.f, 0.f, 0.f, 1.f));
                    button_sig = ui_buttonf("%s/", file.file_name.data);
                } else {
                    button_sig = ui_button(file.file_name);
                }
                if (ui_clicked(button_sig)) {
                    String8 new_path = path_join(scratch, file_path, file.file_name);
                    if (file.flags & OS_FILE_DIRECTORY) {
                        gui_file_system_load_path(fs, new_path);
                    } else {
                        gui_editor_open_file(current_editor, new_path);
                        destroy_gui_view(view);
                    }
                }
            }
        }

        arena_release(scratch);
        break;
    }
    }
}
