
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
    GUI_View *view = make_gui_view();
    Arena *arena = make_arena(get_malloc_allocator());
    view->type = GUI_VIEW_FILE_SYSTEM;
    view->fs = {};
    view->fs.arena = arena;
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

    // Draw_Clip(box->rect) {
        draw_rect(box->rect, box->background_color);
        
        v2 text_position = ui_get_text_position(box);
        text_position += box->view_offset;
        draw_string(box->string, box->font_face, box->text_color, text_position);

        v2 cursor_pos = box->rect.p0 + measure_string_size(string_before_cursor, box->font_face) + box->view_offset;
        Rect cursor_rect = make_rect(cursor_pos.x, cursor_pos.y, 1.f, box->font_face->glyph_height);
        v4 cursor_color = box->text_color;
        // cursor_color *= (1.f - draw_data->editor->cursor_dt);
        // draw_data->editor->cursor_dt += ui_animation_dt();
        // draw_data->editor->cursor_dt = ClampBot(draw_data->editor->cursor_dt, 0.f);
        if (box->key == ui_focus_active_id()) {
            cursor_rect.x1 += 1.f;
        }
        draw_rect(cursor_rect, cursor_color);
    // }
}

internal void gui_file_system_load_files(GUI_File_System *fs) {
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
                int hour, minute, second;
                os_local_time(&hour, &minute, &second);
                ui_set_next_pref_width(ui_pct(1.f, 1.f));
                ui_set_next_pref_height(ui_text_dim(2.f, 1.f));
                ui_set_next_background_color(V4(.94f, .94f, .94f, 1.f));
                UI_Box *bottom_bar = ui_make_box_from_stringf(UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_TEXT, "-\\**-  %s  (%lld,%lld) %d:%d###bottom_bar_%s", file_name, editor->cursor.line, editor->cursor.col, hour, minute, file_name);
            }

            ui_set_next_background_color(V4(.94f, .94f, .94f, 1.f));
            UI_Signal signal = ui_scroll_bar(editor->buffer->file_name, gui_editor->scroll_pt, V2(0.f, buffer_get_line_count(editor->buffer) * editor->face->glyph_height));
            if (ui_clicked(signal)) {
                gui_editor->scroll_pt = ui_get_mouse().y;
            }
        }

        code_body->view_offset_target.y = -gui_editor->scroll_pt;

        gui_editor->active_text_input = signal.text;
        gui_editor->box = code_body;

        if (ui_focus_active_id() == code_body->key) {
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

        code_body->string = buffer_to_string(ui_build_arena(), editor->buffer); 

        Editor_Draw_Data *editor_draw_data = push_array(ui_build_arena(), Editor_Draw_Data, 1);
        editor_draw_data->editor = editor;
        ui_set_custom_draw(code_body, draw_gui_editor, editor_draw_data);
        break;
    }

    case GUI_VIEW_FILE_SYSTEM:
    {
        GUI_File_System *fs = &view->fs;
        GUI_Editor *gui_editor = fs->current_editor;
        
        v2 root_dim = rect_dim(gui_editor->box->rect);
        v2 dim = V2(400.f, 600.f);
        v2 p = V2(.5f * root_dim.x - .5f * dim.x, .5f * root_dim.y - .5f * dim.y);

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

        UI_BackgroundColor(V4(.4f, .4f, .4f, 1.f))
            UI_TextColor(V4(.2f, .2f, .2f, 1.f))
            UI_BorderColor(V4(.2f, .2f, .2f, 1.f))
            UI_Parent(fs_container)
            UI_PrefWidth(ui_pct(1.f, 1.f))
            UI_PrefHeight(ui_text_dim(2.f, 0.f))
        {
            UI_Signal prompt_sig = ui_line_edit(str8_lit("fs_prompt_1"), fs->path_buffer, 2048, &fs->path_pos, &fs->path_len);

            if (ui_pressed(prompt_sig)) {
                if (prompt_sig.key == OS_KEY_SLASH || prompt_sig.key == OS_KEY_BACKSLASH) 
                    gui_file_system_load_files(fs);
            }
            if (prompt_sig.key == OS_KEY_BACKSPACE) {
                u8 last = fs->path_len > 0 ? fs->path_buffer[fs->path_len - 1] : 0;
                if (last == '\\' || last == '/') {
                    gui_file_system_load_files(fs);
                }
            }
            if (prompt_sig.key == OS_KEY_ENTER) {
                String8 file_path = str8_copy(scratch, str8(fs->path_buffer, fs->path_len));
                if (os_file_exists(file_path)) {
                    gui_editor->editor->buffer = make_buffer(file_path);
                    destroy_gui_view(view);
                }
            }
            if (prompt_sig.key == OS_KEY_ESCAPE) {
                destroy_gui_view(view);
            }
        
            for (int i = 0; i < fs->sub_file_count; i++) {
                String8 sub_path = fs->sub_file_paths[i];
                // if (ui_clicked(ui_buttonf("foo_button_%s", sub_path.data))) {
                UI_Signal button_sig = ui_button(sub_path);
                if (ui_clicked(button_sig)) {
                    //@Todo use a set normalized path for this, not current prompt path
                    String8 file_path = str8(fs->path_buffer, fs->path_len);
                    file_path = str8_concat(scratch, file_path, sub_path);
                    gui_editor->editor->buffer = make_buffer(file_path);
                    destroy_gui_view(view);
                }
            }
        }

        arena_release(scratch);
        break;
    }
    }
}
