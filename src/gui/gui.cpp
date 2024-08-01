
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

// @todo add options for adjusting editor focus (top, center, bottom)
void ensure_cursor_in_view(GUI_View *view, Cursor cursor) {
    GUI_Editor *ged = &view->editor;
    v2 code_area_dim = rect_dim(ged->box->rect);
    s64 line_count = buffer_get_line_count(ged->editor->buffer);
    s64 viewable_lines = (s64)ceil(code_area_dim.y / ged->box->font->glyph_height);
    Rng_S64 view_rng = rng_s64(view->scroll_pos.y.idx, view->scroll_pos.y.idx + viewable_lines);
    UI_Scroll_Pt new_pt = view->scroll_pos.y;
    if (cursor.line < view_rng.min || cursor.line >= view_rng.max) {
        new_pt.idx = cursor.line - (viewable_lines / 2);
    }
    view->scroll_pos.y = new_pt;
}

internal void gui_editor_open_file(GUI_Editor *editor, String8 file_name) {
    editor->editor->buffer = make_buffer(file_name);
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
    GUI_View *view;
    GUI_Editor *editor;
};

internal UI_BOX_CUSTOM_DRAW_PROC(draw_gui_editor) {
    Editor_Draw_Data *draw_data = (Editor_Draw_Data *)user_data;
    GUI_View *view = draw_data->view;
    GUI_Editor *ged = draw_data->editor;
    Hoc_Editor *editor = ged->editor;
    Face *font = box->font;

    v2 code_area_dim = rect_dim(ged->box->rect);
    s64 line_count = buffer_get_line_count(editor->buffer);
    s64 viewable_lines = (s64)ceil(code_area_dim.y / font->glyph_height);
    Rng_S64 view_rng = rng_s64(view->scroll_pos.y.idx, view->scroll_pos.y.idx + viewable_lines);

    String8 string_before_cursor = box->string;
    string_before_cursor.count = editor->cursor.position;
    v2 cursor_pos = box->rect.p0 + measure_string_size(string_before_cursor, font) + box->view_offset;

    draw_rect(box->rect, box->background_color);

    v2 text_position = ui_text_position(box);
    text_position += box->view_offset;

    Rect line_rect = make_rect(box->rect.x0, cursor_pos.y, rect_width(box->rect), font->glyph_height);
    draw_rect(line_rect, V4(.24f, .22f, .21f, 1.f));

    if (ged->searching) {
        for (int i = 0; i < ged->search_matches.count; i++) {
            Rng_S64 match = ged->search_matches[i];
            String8 match_str = str8(box->string.data + match.min, rng_s64_len(match));
            Cursor match_cursor = get_cursor_from_position(editor->buffer, match.min);
            s64 line_position = get_position_from_line(editor->buffer, match_cursor.line);
            f32 start = get_string_width(str8(box->string.data + line_position, match_cursor.col), font);
            
            Rect match_rect = make_rect(start, match_cursor.line * font->glyph_height + ged->box->view_offset.y, get_string_width(match_str, font), font->glyph_height);
            draw_rect(match_rect, V4(1.f, 1.f, 1.f, .2f));
        }
    }
    
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

    f32 cursor_thickness = 2.f;
    cursor_thickness = get_string_width(str8(box->string.data + editor->cursor.position, 1), font);
    Rect cursor_rect = make_rect(cursor_pos.x, cursor_pos.y, cursor_thickness, font->glyph_height);
    v4 cursor_color = V4(.92f, .86f, .7f, 1.f);
    draw_rect(cursor_rect, cursor_color);
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
        Hoc_Buffer *buffer = editor->buffer;

        String8 file_name = editor->buffer->file_name;

        //@Note Main editor container
        ui_set_next_child_layout_axis(AXIS_X);
        ui_set_next_pref_width(ui_pct(1.f, 0.f));
        ui_set_next_pref_height(ui_pct(1.f, 0.f));
        UI_Box *container_body = ui_make_box_from_stringf(UI_BOX_NIL, "editor_container_%s", file_name.data);

        UI_Box *code_body = nullptr;
        UI_Signal signal{};

        UI_Parent(container_body)
            UI_TextAlignment(UI_TEXT_ALIGN_LEFT)
            UI_Font(default_fonts[FONT_DEFAULT])
        {
            //@Note File Editor body
            ui_set_next_pref_width(ui_pct(1.f, 0.f));
            ui_set_next_pref_height(ui_pct(1.f, 0.f));
            ui_set_next_child_layout_axis(AXIS_Y);
            UI_Box *editor_body = ui_make_box_from_stringf(UI_BOX_NIL, "edit_body_%s", file_name.data);

            UI_Parent(editor_body)
            {
                //@Note Code body
                ui_set_next_pref_width(ui_pct(1.f, 0.f));
                ui_set_next_pref_height(ui_pct(1.f, 0.f));
                ui_set_next_font(editor->font);
                ui_set_next_hover_cursor(OS_CURSOR_IBEAM);
                code_body = ui_make_box_from_stringf(UI_BOX_CLICKABLE | UI_BOX_KEYBOARD_CLICKABLE | UI_BOX_SCROLL | UI_BOX_DRAW_BACKGROUND, "code_view_%d", view->id);
                signal = ui_signal_from_box(code_body);

                //@Note File bar
                ui_set_next_pref_width(ui_pct(1.f, 1.f));
                ui_set_next_pref_height(ui_text_dim(2.f, 1.f));
                ui_set_next_background_color(V4(.24f, .25f, .25f, 1.f));
                ui_set_next_child_layout_axis(AXIS_X);
                String8 bottom_bar_string = str8_zero();
                UI_Box *bottom_bar = ui_make_box_from_stringf(UI_BOX_DRAW_BACKGROUND, "bottom_bar_%s", file_name.data);
                UI_Parent(bottom_bar)
                    UI_PrefWidth(ui_text_dim(2.f, 1.f))
                    UI_PrefHeight(ui_pct(1.f, 1.f))
                {
                    if (editor->buffer->modified) {
                        ui_set_next_text_color(V4(1.f, 0.f, 0.f, 1.f));
                        ui_label(str8_lit("*"));
                    }
                    UI_Signal sig = ui_label(file_name);
                    ui_labelf("   (%lld,%lld)", editor->cursor.line + 1, editor->cursor.col);

                    // spacer
                    ui_set_next_pref_width(ui_pct(1.f, 0.f));
                    UI_Box *spacer = ui_make_box_from_string(UI_BOX_NIL, str8_lit("###spacer"));

                    //@Note hms_time
                    int hour, minute, second;
                    os_local_time(&hour, &minute, &second);
                    ui_labelf("%d:%02d", hour, minute);

                    ui_labelf(" %s", buffer->line_end==LINE_ENDING_CRLF ? "crlf" : "lf");
                }
            }

            v2 code_area_dim = rect_dim(code_body->rect);

            if (gui_editor->searching) {
                ui_set_next_parent(editor_body);
                ui_set_next_fixed_x(code_area_dim.x/2.f);
                ui_set_next_fixed_y(0.f);
                ui_set_next_pref_width(ui_px(code_area_dim.x / 2.f, 1.f));
                ui_set_next_pref_height(ui_px(code_body->font->glyph_height * 1.5f, 1.f));
                ui_set_next_background_color(V4(.16f, .16f, .16f, 1.f));
                ui_set_next_child_layout_axis(AXIS_X);
                UI_Box *search_container = ui_make_box_from_string(UI_BOX_DRAW_BACKGROUND, str8_lit("search_container"));

                UI_Signal search_sig{};
                UI_Parent(search_container) {
                    ui_set_next_pref_width(ui_text_dim(2.f, 1.f));
                    ui_set_next_pref_height(ui_text_dim(2.f, 1.f));
                    ui_set_next_background_color(V4(.16f, .16f, .16f, 1.f));
                    ui_make_box_from_string(UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_TEXT, str8_lit("Find Text  "));

                    ui_set_next_pref_width(ui_pct(1.f, 0.f));
                    ui_set_next_pref_height(ui_px(code_body->font->glyph_height, 1.f));
                    ui_set_next_background_color(V4(.16f, .16f, .16f, 1.f));
                    search_sig = ui_line_edit(str8_lit("search_line"), gui_editor->search_buffer, 2048, &gui_editor->search_pos, &gui_editor->search_len);

                    ui_set_next_pref_width(ui_text_dim(2.f, 1.f));
                    ui_set_next_pref_height(ui_text_dim(2.f, 1.f));
                    ui_set_next_background_color(V4(.16f, .16f, .16f, 1.f));
                    ui_labelf("%d matches", gui_editor->search_matches.count);
                }

                if (ui_pressed(search_sig)) {
                    if (search_sig.key == OS_KEY_ESCAPE) {
                        gui_editor->searching = false;
                        editor_set_cursor(editor, gui_editor->search_cursor);
                        ensure_cursor_in_view(view, gui_editor->search_cursor);
                        // ui_set_focus_active_key(gui_editor->box->key);
                    } else if (search_sig.key == OS_KEY_UP || search_sig.key == OS_KEY_DOWN) {
                        bool next = search_sig.key == OS_KEY_DOWN;
                        int match_idx = gui_editor->match_idx + (next ? 1 : -1);
                        //@Note wrap
                        if (match_idx < 0) {
                            match_idx = (int)gui_editor->search_matches.count - 1;
                        } else if (match_idx >= gui_editor->search_matches.count) {
                            match_idx = 0;
                        }

                        if (gui_editor->search_matches.count > 0) {
                            Rng_S64 match = gui_editor->search_matches[match_idx];
                            Cursor cursor = get_cursor_from_position(editor->buffer, match.min);
                            editor_set_cursor(editor, cursor);
                            ensure_cursor_in_view(view, cursor);
                        }
                        gui_editor->match_idx = match_idx;
                    } else if (search_sig.key == OS_KEY_ENTER) {
                        gui_editor->searching = false;
                        ui_set_focus_active_key(gui_editor->box->key);
                    } else {
                        String8 search_string = str8(gui_editor->search_buffer, gui_editor->search_len);
                        gui_editor->search_matches.reset_count();
                        gui_editor->search_matches = buffer_find_text_matches(editor->buffer, search_string);

                        gui_editor->match_idx = 0;
                        for (int i = 0; i < gui_editor->search_matches.count; i++) {
                            Rng_S64 match = gui_editor->search_matches[i];
                            if (match.min >= gui_editor->search_cursor.position) {
                                gui_editor->match_idx = i;
                                break;
                            }
                        }
                    }
                }
            }

            s64 line_count = buffer_get_line_count(editor->buffer);
            s64 viewable_lines = (s64)ceil(code_area_dim.y / code_body->font->glyph_height);
            Rng_S64 view_rng = rng_s64(view->scroll_pos.y.idx, view->scroll_pos.y.idx + viewable_lines);
            ui_set_next_background_color(V4(.18f, .18f, .18f, 1.f));
            view->scroll_pos.y = ui_scroll_bar(file_name, AXIS_Y, ui_px(editor->font->glyph_width, 1.f), view->scroll_pos.y, view_rng, line_count);
        }

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
            view->scroll_pos.y.idx -= 3 * signal.scroll.y;
        }

        if (ui_dragging(signal)) {
            v2 mouse = ui_mouse();
            int line_dt = 3;
            if (mouse.y <= code_body->rect.y0) {
                view->scroll_pos.y.idx -= line_dt;
            }
            if (mouse.y >= code_body->rect.y1) {
                view->scroll_pos.y.idx += line_dt;
            }
            Cursor cursor = editor_mouse_to_cursor(gui_editor, ui_mouse());
            editor_set_cursor(editor, cursor);
            editor->mark_active = editor->mark.position != editor->cursor.position;
        }

        view->scroll_pos.x.idx = ClampBot(view->scroll_pos.x.idx, 0);
        view->scroll_pos.y.idx = ClampBot(view->scroll_pos.y.idx, 0);

        code_body->view_offset_target.x = -view->scroll_pos.x.idx * code_body->font->glyph_width;
        code_body->view_offset_target.y = -view->scroll_pos.y.idx * code_body->font->glyph_height;

        code_body->string = buffer_to_string(ui_build_arena(), editor->buffer, false); 

        Editor_Draw_Data *editor_draw_data = push_array(ui_build_arena(), Editor_Draw_Data, 1);
        editor_draw_data->view = view;
        editor_draw_data->editor = gui_editor;
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
        ui_set_next_child_layout_axis(AXIS_Y);
        UI_Box *fs_container = ui_make_box_from_stringf(UI_BOX_OVERFLOW_Y, "file_system_%d", view->id);

        Arena *scratch = make_arena(get_malloc_allocator());

        UI_Parent(fs_container)
            UI_TextAlignment(UI_TEXT_ALIGN_LEFT)
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
                    ui_set_focus_active_key(current_editor->box->key);
                }
            }
            if (prompt_sig.key == OS_KEY_ESCAPE) {
                destroy_gui_view(view);
                ui_set_focus_active_key(current_editor->box->key);
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
                        // ui_set_focus_active_key(current_editor->box->key);
                    }
                }
            }
        }

        arena_release(scratch);
        break;
    }
    }
}
