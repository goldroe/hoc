internal void remove_gui_view(GUI_View *view) {
    GUI_View_List *views = &hoc_app->gui_views;
    GUI_View *prev = view->prev;
    GUI_View *next = view->next;
    if (prev) prev->next = next;
    if (next) next->prev = prev;
    
    if (views->first == view && views->last == view) {
        views->first = nullptr;
        views->last = nullptr;
    } else if (views->first == view) {
        views->first = next;
    } else if (views->last == view) {
        views->last = prev;
    }
    views->count -= 1;
}

internal void push_gui_view(GUI_View *view) {
    if (hoc_app->gui_views.first) {
        view->prev = hoc_app->gui_views.last;
        hoc_app->gui_views.last->next = view;
    } else {
        hoc_app->gui_views.first = view;
    }
    hoc_app->gui_views.last = view;
    hoc_app->gui_views.count += 1;
}

internal Cursor editor_mouse_to_cursor(GUI_Editor *gui_editor, v2 mouse) {
    Hoc_Editor *editor = gui_editor->editor;
    //@Todo support non-mono fonts
    Cursor result{};
    f32 y = mouse.y - gui_editor->box->view_offset.y;
    f32 x = mouse.x - gui_editor->box->view_offset.x;
    s64 line = (s64)(y / gui_editor->box->font_face->glyph_height);
    s64 col = (s64)(x / gui_editor->box->font_face->glyph_width);
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

internal GUI_View *make_gui_view() {
    Core_Allocator *allocator = get_malloc_allocator();
    GUI_View *view = (GUI_View *)arena_alloc(allocator, sizeof(GUI_View));
    view->id = hoc_app->view_id_counter++;
    view->prev = nullptr;
    view->next = nullptr;
    push_gui_view(view);
    return view;
}

internal GUI_View *make_gui_editor() {
    GUI_View *view = make_gui_view();
    view->type = GUI_VIEW_EDITOR;
    view->editor.editor = make_editor();
    return view;
}

internal GUI_View *make_gui_file_system() {
    GUI_View *view = make_gui_view();
    Arena *arena = make_arena(get_malloc_allocator());
    view->type = GUI_VIEW_FILE_SYSTEM;
    view->fs = {};
    view->fs.arena = arena;
    return view;
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
    String8 string_before_cursor = box->string;
    string_before_cursor.count = draw_data->editor->cursor.position;

    draw_rect(box->rect, box->background_color);

    v2 text_position = ui_get_text_position(box);
    text_position += box->view_offset;
    draw_string(box->string, box->font_face, box->text_color, text_position);

    v2 cursor_pos = box->rect.p0 + measure_string_size(string_before_cursor, box->font_face) + box->view_offset;
    Rect cursor_rect = make_rect(cursor_pos.x, cursor_pos.y, 2.f, box->font_face->glyph_height);
    v4 cursor_color = box->text_color;
    // cursor_color *= (1.f - draw_data->editor->cursor_dt);
    // draw_data->editor->cursor_dt += ui_animation_dt();
    // draw_data->editor->cursor_dt = ClampBot(draw_data->editor->cursor_dt, 0.f);
    if (box->hash == ui_focus_active_id()) {
        draw_rect(cursor_rect, cursor_color);
    } else {
        draw_rect_outline(cursor_rect, cursor_color);
    }
}

internal void gui_file_system_load_files(GUI_File_System *fs) {
    printf("LOAD FILES\n");
    Arena *scratch = make_arena(get_malloc_allocator());

    String8 find_path = str8_copy(scratch, str8(fs->path_buffer, fs->path_len));
    Find_File_Data file_data{};
    OS_Handle find_handle = find_first_file(fs->arena, find_path, &file_data);
    fs->sub_file_count = 0;
    if (os_valid_handle(find_handle)) {
        do {
            fs->sub_file_paths[fs->sub_file_count] = str8_copy(fs->arena, file_data.file_name);
            fs->sub_file_count += 1;
        } while (find_next_file(fs->arena, find_handle, &file_data));
        find_close(find_handle);
    } else {
        printf("failed find_first_file\n");
    }
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
        UI_Box *bottom_bar = nullptr;
        UI_Signal signal{};

        UI_Parent(container_body)
            UI_TextAlignment(UI_TEXT_ALIGN_LEFT)
            UI_Font(default_fonts[FONT_DEFAULT])
        {
            //@Note File Editor body
            // ui_set_next_pref_width(ui_px(gui_editor->panel_dim.x, 1.f));
            // ui_set_next_pref_height(ui_px(gui_editor->panel_dim.y, 1.f));
            ui_set_next_pref_width(ui_pct(1.f, 0.f));
            ui_set_next_pref_height(ui_pct(1.f, 0.f));
            ui_set_next_child_layout(AXIS_Y);
            UI_Box *editor_body = ui_make_box_from_stringf(UI_BOX_NIL, "edit_body_%s", file_name);
        
            UI_Parent(editor_body)
            {
                //@Note Code body
                // ui_set_next_pref_width(ui_px(gui_editor->panel_dim.x, 1.f));
                // ui_set_next_pref_height(ui_px(gui_editor->panel_dim.y - 1.5f * editor->face->glyph_height, 1.f));
                ui_set_next_pref_width(ui_pct(1.f, 0.f));
                ui_set_next_pref_height(ui_pct(1.f, 0.f));
                ui_set_next_font_face(editor->face);
                code_body = ui_make_box_from_stringf(UI_BOX_CLICKABLE | UI_BOX_KEYBOARD_INPUT | UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_TEXT, "code_view_%s", file_name);
                signal = ui_signal_from_box(code_body);

                //@Note File bar
                ui_set_next_pref_width(ui_pct(1.f, 1.f));
                ui_set_next_pref_height(ui_text_dim(2.f, 1.f));
                ui_set_next_background_color(V4(.94f, .94f, .94f, 1.f));
                bottom_bar = ui_make_box_from_stringf(UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_TEXT, "bottom_bar_%s", file_name);
                int hour, minute, second;
                os_local_time(&hour, &minute, &second);
                String8 file_bar_string = str8_pushf(ui_build_arena(), "-\\**-  %s  (%lld,%lld) %d:%d", file_name, editor->cursor.line, editor->cursor.col, hour, minute);
                ui_set_string(bottom_bar, file_bar_string);
            }

            ui_set_next_background_color(V4(.94f, .94f, .94f, 1.f));
            UI_Signal signal = ui_scroll_bar(editor->buffer->file_name, gui_editor->scroll_pt, V2(0.f, buffer_get_line_count(editor->buffer) * editor->face->glyph_height));
            if (ui_clicked(signal)) {
                gui_editor->scroll_pt = ui_get_mouse().y;
                // printf("%f\n", gui_editor->scroll_pt);
            }
        }
        
        code_body->view_offset_target.y = -gui_editor->scroll_pt;

        gui_editor->active_text_input = signal.text;
        gui_editor->box = code_body;

        if (ui_focus_active_id() == code_body->hash) {
            if (ui_pressed(signal)) {
                u16 key = os_key_to_key_mapping(signal.key, signal.key_modifiers);
                if (key && signal.key) {
                    view->key_map->mappings[key].procedure(view);
                }
            }

            if (ui_clicked(signal)) {
                Cursor cursor = editor_mouse_to_cursor(gui_editor, ui_state->mouse_position);
                editor_set_cursor(editor, cursor);
            }
        }

        if (ui_scroll(signal)) {
            gui_editor->scroll_pt -= editor->face->glyph_height * signal.scroll.y;
        }

        ui_set_string(code_body, buffer_to_string(ui_build_arena(), editor->buffer));

        Editor_Draw_Data *editor_draw_data = push_array(ui_build_arena(), Editor_Draw_Data, 1);
        editor_draw_data->editor = editor;
        ui_set_custom_draw(code_body, draw_gui_editor, editor_draw_data);
        break;
    }

    case GUI_VIEW_FILE_SYSTEM:
    {
        GUI_File_System *fs = &view->fs;
        GUI_Editor *gui_editor = fs->current_editor;
        
        v2 root_dim = V2(1000.f, 600.f);
        v2 dim = V2(600.f, 400.f);
        v2 p = V2(.5f * root_dim.x - .5f * dim.x, .5f * root_dim.y - .5f * dim.y);

        ui_set_next_text_color(V4(.4f, .4f, .4f, 1.f));
        ui_set_next_background_color(V4(.2f, .2f, .2f, 1.f));
        ui_set_next_border_color(V4(.4f, .4f, .4f, 1.f));
        ui_set_next_fixed_x(p.x);
        ui_set_next_fixed_y(p.y);
        ui_set_next_fixed_width(dim.x);
        ui_set_next_fixed_height(dim.y);
        ui_set_next_child_layout(AXIS_Y);
        UI_Box *fs_container = ui_make_box_from_stringf(UI_BOX_DRAW_BACKGROUND, "file_system_%d", view->id);

        bool kill_file_system = false;

        UI_BackgroundColor(V4(.4f, .4f, .4f, 1.f))
            UI_TextColor(V4(.2f, .2f, .2f, 1.f))
            UI_BorderColor(V4(.2f, .2f, .2f, 1.f))
            UI_Parent(fs_container)
            UI_PrefWidth(ui_pct(1.f, 1.f))
            UI_PrefHeight(ui_text_dim(2.f, 1.f))
        {
            UI_Signal prompt_sig = ui_line_edit(str8_lit("Navigate to File"), fs->path_buffer, 2048, &fs->path_pos, &fs->path_len);
            if (!prompt_sig.box->string.count) {
                ui_set_string(prompt_sig.box, str8_lit("Navigate to File"));
            }

            Arena *scratch = make_arena(get_malloc_allocator());
            if (ui_pressed(prompt_sig)) {
                if (prompt_sig.key == OS_KEY_SLASH || prompt_sig.key == OS_KEY_BACKSLASH) {
                    gui_file_system_load_files(fs);
                } else if (prompt_sig.key == OS_KEY_BACKSPACE) {
                    u8 last = fs->path_len > 0 ? fs->path_buffer[fs->path_len - 1] : 0;
                    if (last == '\\' || last == '/') {
                        gui_file_system_load_files(fs);
                    }
                } else if (prompt_sig.key == OS_KEY_ENTER) {
                    String8 file_path = str8_copy(scratch, str8(fs->path_buffer, fs->path_len));
                    if (os_file_exists(file_path)) {
                        gui_editor->editor->buffer = make_buffer(file_path);
                        kill_file_system = true;
                    }
                } else if (prompt_sig.key == OS_KEY_ESCAPE) {
                    kill_file_system = true;
                }
            }
        
            for (int i = 0; i < fs->sub_file_count; i++) {
                String8 sub_path = fs->sub_file_paths[i];
                if (ui_clicked(ui_button(sub_path))) {
                    //@Todo use a set normalized path for this, not current prompt path
                    String8 file_path = str8(fs->path_buffer, fs->path_len);
                    file_path = str8_concat(scratch, file_path, sub_path);
                    // printf("CLICKED %s\n", file_path.data);
                    gui_editor->editor->buffer = make_buffer(file_path);
                    kill_file_system = true;
                }
            }
            arena_release(scratch);
        }

        if (kill_file_system) {
            remove_gui_view(view);
        }
        break;
    }

    }
}