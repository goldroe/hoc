#include <ft2build.h>
#include <freetype/freetype.h>

#pragma warning(push)
#pragma warning( disable : 4244)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

#include "core/core.h"
#include "core/core_arena.h"
#include "core/core_strings.h"
#include "core/core_math.h"
#include "auto_array.h"
#include "utils.h"
#include "os/os.h"
#include "path.h"
#include "font.h"
#include "lexer.h"
#include "ui/ui_core.h"
#include "ui/ui_widgets.h"
#include "render_d3d11.h"
#include "draw.h"
#include "hoc.h"
#include "hoc/hoc_buffer.h"

#include "core/core_math.cpp"
#include "core/core_arena.cpp"
#include "core/core_strings.cpp"
#include "os/os.cpp"
#include "path.cpp"
#include "render_d3d11.cpp"
#include "font.cpp"
#include "lexer.cpp"
#include "draw.cpp"
#include "ui/ui_core.cpp"
#include "ui/ui_widgets.cpp"
#include "hoc/hoc_buffer.cpp"
#include "hoc_commands.cpp"
#include "generated_hoc_commands.cpp"

global bool window_should_close;
global s64 performance_frequency;

global Hoc_Application *hoc_app;

global Arena *win32_event_arena;
global OS_Event_List win32_events;

global Arena *key_map_arena;
global Key_Map *default_key_map;
global Key_Map *find_file_key_map;

global u8 text_buffer[1024];
global u64 text_buffer_count;
global u64 text_buffer_pos;

internal void push_view(View *view) {
    if (hoc_app->views.first) {
        view->prev = hoc_app->views.last;
        hoc_app->views.last->next = view;
        hoc_app->views.last = view;
    } else {
        hoc_app->views.first = hoc_app->views.last = view;
    }
}

internal void push_buffer(Buffer *buffer) {
    if (hoc_app->buffers.first) {
        buffer->prev = hoc_app->buffers.last;
        hoc_app->buffers.last->next = buffer;
        hoc_app->buffers.last = buffer;
    } else {
        hoc_app->buffers.first = hoc_app->buffers.last = buffer;
    }
}

internal void quit_hoc_application() {
    window_should_close = true;
}

internal View *get_active_view() {
    View *result = hoc_app->active_view;
    return result;
}

internal Hoc_Application *make_hoc_application() {
    Arena *arena = arena_new();
    Hoc_Application *app = push_array(arena, Hoc_Application, 1);
    app->arena = arena;
    app->view_id_counter = 0;
    return app;
}

internal OS_Event *win32_push_event(OS_Event_Type type) {
    OS_Event *result = push_array(win32_event_arena, OS_Event, 1);
    result->type = type;
    result->next = nullptr;
    result->flags = os_event_flags();
    OS_Event *last = win32_events.last;
    if (last) {
        last->next = result;
    } else {
        win32_events.first = result;
    }
    win32_events.last = result;
    win32_events.count += 1;
    return result;
}

internal LRESULT CALLBACK window_proc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    OS_Event *event = nullptr;
    bool release = false;

    LRESULT result = 0;
    switch (Msg) {
    case WM_MOUSEWHEEL:
    {
        event = win32_push_event(OS_EVENT_SCROLL);
        int delta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        event->delta.y = delta;
        break;
    }

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam); 
        int y = GET_Y_LPARAM(lParam); 
        event = win32_push_event(OS_EVENT_MOUSEMOVE);
        event->pos.x = x;
        event->pos.y = y;
        break;
    }

    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
        release = true;
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        event = win32_push_event(release ? OS_EVENT_MOUSEUP : OS_EVENT_MOUSEDOWN);
        switch (Msg) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
            event->key = OS_KEY_LEFTMOUSE;
            break;
        case WM_MBUTTONUP:
        case WM_MBUTTONDOWN:
            event->key = OS_KEY_MIDDLEMOUSE;
            break;
        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
            event->key = OS_KEY_RIGHTMOUSE; 
            break;
        }
        int x = GET_X_LPARAM(lParam); 
        int y = GET_Y_LPARAM(lParam); 
        event->pos.x = x;
        event->pos.y = y;
        break;
    }

    case WM_SYSCHAR:
    {
        // result = DefWindowProcA(hWnd, Msg, wParam, lParam);
        break;
    }
    case WM_CHAR:
    {
        u16 vk = wParam & 0x0000ffff;
        u8 c = (u8)vk;
        if (c == '\r') c = '\n';
        if (c >= 32 && c != 127) {
            event = win32_push_event(OS_EVENT_TEXT);
            event->text = str8_copy(win32_event_arena, str8(&c, 1));
        }
        break;
    }

    case WM_SYSKEYUP:
    case WM_KEYUP:
        release = true;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        u8 down = ((lParam >> 31) == 0);
        u32 virtual_keycode = (u32)wParam;
        if (virtual_keycode < 256) {
            event = win32_push_event(release ? OS_EVENT_RELEASE : OS_EVENT_PRESS);
            event->key = os_key_from_vk(virtual_keycode);
        }
        break;
    }

    case WM_SIZE:
    {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        break;
    }
    
    case WM_CREATE:
    {
        break;
    }
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    default:
        result = DefWindowProcA(hWnd, Msg, wParam, lParam);
    }
    return result;
}

inline s64 get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return (s64)result.QuadPart;
}

inline f32 get_seconds_elapsed(s64 start, s64 end) {
    f32 result = ((f32)(end - start) / (f32)performance_frequency);
    return result;
}


internal u16 os_key_to_key_mapping(OS_Key key, OS_Event_Flags modifiers) {
    u16 result = (u16)key;
    result |= (modifiers & OS_EVENT_FLAG_ALT) ? KEY_MOD_ALT : 0;
    result |= (modifiers & OS_EVENT_FLAG_CONTROL) ? KEY_MOD_CONTROL : 0;
    result |= (modifiers & OS_EVENT_FLAG_SHIFT) ? KEY_MOD_SHIFT : 0;
    return result;
}


struct UI_View_Draw_Data {
    View *view;
};

internal UI_BOX_CUSTOM_DRAW_PROC(draw_gui_view) {
    UI_View_Draw_Data *draw_data = (UI_View_Draw_Data *)box_draw_data;
    String8 string_before_cursor = box->string;
    string_before_cursor.count = draw_data->view->cursor.position;

    v2 c_pos = box->rect.p0 + measure_string_size(string_before_cursor, box->font_face) + box->view_offset;
    // Rect c_rect = make_rect(c_pos.x, c_pos.y, box->font_face->glyph_width, box->font_face->glyph_height);
    Rect c_rect = make_rect(c_pos.x, c_pos.y, 2.f, box->font_face->glyph_height);
    v4 cursor_color = V4(0.f, 0.f, 0.f, 1.f);
    if (box->hash == ui_focus_active_id()) {
        draw_rect(c_rect, cursor_color);
    } else {
        draw_rect_outline(c_rect, cursor_color);
    }
}

internal Cursor get_cursor_from_mouse(View *view, v2 mouse) {
    Cursor result{};
    f32 y = mouse.y - view->box->view_offset.y;
    f32 x = mouse.x - view->box->view_offset.x;
    s64 line = (s64)(y / view->box->font_face->glyph_height);
    // line = ClampTop(line, buffer_get_line_count(view->buffer) - 1);
    //@Todo support non-mono fonts
    s64 col = (s64)(x / view->box->font_face->glyph_width);
    col = ClampTop(col, buffer_get_line_length(view->buffer, line));
    s64 position = get_position_from_line(view->buffer, line) + col;
    position = ClampTop(position, get_position_from_line(view->buffer, line + 1) - 1);
    result = get_cursor_from_position(view->buffer, position);
    return result;
}

internal void gui_view(View *view) {
    //@Note Code body
    String8 box_str = str8_concat(ui_build_arena(), str8_lit("editor_body_"), view->buffer->file_name);
    ui_set_next_child_layout(AXIS_Y);
    ui_set_next_pref_width(ui_px(view->panel_dim.x, 1.f));
    ui_set_next_pref_height(ui_px(view->panel_dim.y, 1.f));
    UI_Box *editor_body = ui_make_box_from_string(box_str, UI_BOX_NIL);
    UI_Box *code_body = nullptr;
    UI_Box *bottom_bar = nullptr;
    UI_Signal signal;

    //@Note Code view
    UI_Parent(editor_body)
    UI_TextAlignment(UI_TEXT_ALIGN_LEFT)
    {
        // ui_set_next_font_face(view->face);
        ui_set_next_pref_width(ui_px(view->panel_dim.x, 1.f));
        ui_set_next_pref_height(ui_px(view->panel_dim.y - 1.5f * view->face->glyph_height, 1.f));
        code_body = ui_make_box_from_string(view->buffer->file_name, UI_BOX_CLICKABLE | UI_BOX_KEYBOARD_INPUT | UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_TEXT);
        signal = ui_signal_from_box(code_body);

        //@Note File bar
        ui_set_next_pref_width(ui_pct(1.f, 1.f));
        ui_set_next_pref_height(ui_text_dim(2.f, 1.f));
        ui_set_next_background_color(V4(.94f, .94f, .94f, 1.f));
        String8 str = str8_concat(ui_build_arena(), str8_lit("bottom_bar_"), view->buffer->file_name);
        bottom_bar = ui_make_box_from_string(str, UI_BOX_DRAW_BACKGROUND | UI_BOX_DRAW_TEXT);
        ui_set_string(bottom_bar, view->buffer->file_name);
    }

    view->active_text_input = signal.text;
    view->box = code_body;

    if (ui_clicked(signal)) {
        hoc_app->active_view = view;
    }


    if (ui_focus_active_id() == code_body->hash) {
        if (ui_pressed(signal)) {
            u16 key = os_key_to_key_mapping(signal.key, signal.key_modifiers);
            if (key && signal.key) {
                view->key_map->mappings[key].procedure();
            }
        }

        if (ui_clicked(signal)) {
            // printf("%f %f\n", ui_state->mouse_position.x, ui_state->mouse_position.y);
            Cursor cursor = get_cursor_from_mouse(view, ui_state->mouse_position);
            view_set_cursor(view, cursor);
        }
    }

    if (signal.scroll.y != 0) {
        // box->view_offset_target.y = box->view_offset.y + view->face->glyph_height*signal.scroll.y;
        code_body->view_offset_target.y += 2.f * view->face->glyph_height*signal.scroll.y;
        // code_body->view_offset_target.y = Clamp(code_body->view_offset_target.y, 0.f, buffer_get_line_count(view->buffer));
    }

    ui_set_string(code_body, buffer_to_string(ui_build_arena(), view->buffer));

    UI_View_Draw_Data *view_draw_data = push_array(ui_build_arena(), UI_View_Draw_Data, 1);
    view_draw_data->view = view;
    code_body->custom_draw_proc = draw_gui_view;
    code_body->box_draw_data = (void *)view_draw_data;
}

internal void draw_ui_box(UI_Box *box) {
    r_d3d11_state->current_texture = box->font_face->texture;
    if (box->flags & UI_BOX_DRAW_BACKGROUND) {
        draw_rect(box->rect, box->background_color);
    }
    // if (box->flags & UI_BOX_DRAW_BORDER) {
    //     draw_rect_outline(box->rect, box->border_color);
    // }
    if (box->flags & UI_BOX_DRAW_TEXT) {
        v2 text_position = ui_get_text_position(box);
        text_position += box->view_offset;
        draw_string(box->string, box->font_face, box->text_color, text_position);
    }
}

internal void draw_ui_layout(UI_Box *box) {
    if (box != ui_state->root) {
        draw_ui_box(box);
    }

    if (box->custom_draw_proc) {
        box->custom_draw_proc(box, box->box_draw_data);
    }

    for (UI_Box *child = box->first; child != nullptr; child = child->next) {
        draw_ui_layout(child);
    }
}

internal void update_and_render(OS_Event_List *os_events, OS_Handle window_handle, f32 dt) {
    ui_begin_build(dt, window_handle, os_events);

    v2 window_dim = os_get_window_dim(window_handle);
    ui_set_next_child_layout(AXIS_X);
    ui_set_next_pref_width(ui_px(window_dim.x, 1.f));
    ui_set_next_pref_height(ui_px(window_dim.y, 1.f));
    UI_Box *main_body = ui_make_box_from_string(str8_lit("main_code_body"), UI_BOX_NIL);

    UI_Parent(main_body)
    UI_BackgroundColor(V4(1.f, 1.f, 1.f, 1.f))
    UI_BorderColor(V4(.2f, .19f, .18f, 1.f))
    UI_TextColor(V4(.2f, .2f, .2f, 1.f))
    // UI_BackgroundColor(V4(.2f, .19f, .18f, 1.f))
    // UI_BorderColor(V4(.2f, .19f, .18f, 1.f))
    // UI_TextColor(V4(.92f, .86f, .7f, 1.f))
    {
        for (View *view = hoc_app->views.first; view; view = view->next) {
            gui_view(view);
        }
    }

    ui_layout_apply(ui_state->root);
    draw_ui_layout(ui_state->root);

    ui_end_build();
}

internal bool os_key_is_ascii(OS_Key key) {
    switch (key) {
    default:
        return false;
    case OS_KEY_A: case OS_KEY_B: case OS_KEY_C: case OS_KEY_D: case OS_KEY_E: case OS_KEY_F: case OS_KEY_G: case OS_KEY_H: case OS_KEY_I: case OS_KEY_J: case OS_KEY_K: case OS_KEY_L: case OS_KEY_M: case OS_KEY_N: case OS_KEY_O: case OS_KEY_P: case OS_KEY_Q: case OS_KEY_R: case OS_KEY_S: case OS_KEY_T: case OS_KEY_U: case OS_KEY_V: case OS_KEY_W: case OS_KEY_X: case OS_KEY_Y: case OS_KEY_Z:
    case OS_KEY_0: case OS_KEY_1: case OS_KEY_2: case OS_KEY_3: case OS_KEY_4: case OS_KEY_5: case OS_KEY_6: case OS_KEY_7: case OS_KEY_8: case OS_KEY_9:
    case OS_KEY_SPACE:
    case OS_KEY_COMMA:
    case OS_KEY_PERIOD:
    case OS_KEY_QUOTE:
    case OS_KEY_OPENBRACKET:
    case OS_KEY_CLOSEBRACKET:
    case OS_KEY_SEMICOLON:
    case OS_KEY_SLASH:
    case OS_KEY_BACKSLASH:
    case OS_KEY_MINUS:
    case OS_KEY_PLUS:
    case OS_KEY_TAB:
    case OS_KEY_TICK:
        return true;
    }
}

internal OS_Key search_os_key_from_string(String8 key_name) {
    OS_Key result = OS_KEY_NIL;
    for (u64 i = 0; i < ArrayCount(os_key_names); i++) {
        String8 name = os_key_names[i];
        if (str8_eq(key_name, name)) {
            result = (OS_Key)i;
            break;
        }
    }
    return result;
}

internal Hoc_Command search_hoc_command_from_string(String8 name) {
    Hoc_Command result = {str8_lit("nil_command"), nil_command};
    for (u64 i = 0; i < ArrayCount(hoc_commands); i++) {
        Hoc_Command *cmd = &hoc_commands[i];
        if (str8_eq(name, cmd->name)) {
            result = *cmd;
            break;
        }
    }
    return result;
}

internal Key_Map *load_key_map(Arena *arena, String8 file_name) {
    Key_Map *key_map = push_array(arena, Key_Map, 1);
    key_map->mappings = push_array(arena, Hoc_Command, MAX_KEY_MAPPINGS);

    //@Note Default key mappings
    for (int i = 0; i < MAX_KEY_MAPPINGS; i++) {
        key_map->mappings[i] = {str8_lit("nil_command"), nil_command};
    }
    Hoc_Command self_insert_command = {str8_lit("self_insert"), self_insert};
    for (int i = 0; i < (1<<8); i++) {
        OS_Key key = (OS_Key)i;
        if (os_key_is_ascii(key)) {
            key_map->mappings[KEY_MOD_SHIFT|i] = self_insert_command;
            key_map->mappings[i] = self_insert_command;
        }
    }

    Lexer lexer = make_lexer(file_name);
    Token token = get_token(&lexer);
    if (token.type == Token_String) {
        key_map->name = str8_copy(arena, token.literal);
    } else {
        error(&lexer, "missing name of key map");
    }

    expect_token(&lexer, Token_Assign);
    expect_token(&lexer, Token_OpenBrace);

    do {
        u16 key_value = 0;
        String8 key_mods[3] = {};
        expect_token(&lexer, Token_OpenBrace);
        Token command_token = get_token(&lexer);
        while (peek_token(&lexer).type == Token_Comma) {
            expect_token(&lexer, Token_Comma);
            Token key_mod = get_token(&lexer);
            if (str8_eq(key_mod.literal, str8_lit("Control"))) {
                key_value |= KEY_MOD_CONTROL;
            } else if (str8_eq(key_mod.literal, str8_lit("Alt"))) {
                key_value |= KEY_MOD_ALT;
            } else if (str8_eq(key_mod.literal, str8_lit("Shift"))) {
                key_value |= KEY_MOD_SHIFT;
            } else {
                key_value |= (u16)search_os_key_from_string(key_mod.literal);
            }
        }
        expect_token(&lexer, Token_CloseBrace);

        key_map->mappings[key_value] = search_hoc_command_from_string(command_token.literal);

        if (peek_token(&lexer).type != Token_Comma) break;
        expect_token(&lexer, Token_Comma);
    } while (token.type != Token_EOF);
    
    expect_token(&lexer, Token_CloseBrace);

    return key_map;
}

internal View *view_new() {
    Arena *arena = arena_new();
    View *result = push_array(arena, View, 1);
    result->arena = arena;
    result->id = hoc_app->view_id_counter++;
    return result;
}

int main(int argc, char **argv) {
    argc--; argv++;
    
    QueryPerformanceFrequency((LARGE_INTEGER *)&performance_frequency);
    timeBeginPeriod(1);

#define CLASSNAME "HOC_WINDOW_CLASS"
    HINSTANCE hinstance = GetModuleHandle(NULL);
    WNDCLASSA window_class{};
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszClassName = CLASSNAME;
    window_class.hInstance = hinstance;
    window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
    if (!RegisterClassA(&window_class)) {
        printf("RegisterClassA failed, err:%d\n", GetLastError());
    }

    HWND hWnd = CreateWindowA(CLASSNAME, "Hoc", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinstance, NULL);
    OS_Handle window_handle = (OS_Handle)hWnd;
    
    d3d11_render_initialize(hWnd);

    v2 old_window_dim = V2();
   
    win32_event_arena = arena_new();

    hoc_app = make_hoc_application();
    ui_set_state(ui_state_new());

    default_font_face = load_font_face(str8_lit("fonts/consolas.ttf"), 24);

    key_map_arena = arena_new();
    default_key_map = load_key_map(key_map_arena, str8_lit("data/bindings.hoc"));

    View *default_view = view_new();
    default_view->buffer = make_buffer_from_file(str8_lit("asdf"));
    default_view->key_map = default_key_map;
    default_view->face = default_font_face;
    push_view(default_view);
    push_buffer(default_view->buffer);
    default_view->panel_dim = V2(1920.f, 1080.f);
    
    // View *split_view = view_new();
    // split_view->buffer = make_buffer(str8_lit("test"));
    // split_view->key_map = default_key_map;
    // split_view->face = default_font_face;
    // push_view(split_view);
    // push_buffer(split_view->buffer);
    // split_view->panel_dim = V2(900.f, 1000.f);

    hoc_app->active_view = default_view;

    f32 dt = 0.0f;
    srand((s32)get_wall_clock());
    s64 start_clock, last_clock;
    start_clock = last_clock = get_wall_clock(); 

    while (!window_should_close) {
        MSG message;
        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
                window_should_close = true;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }

        v2 window_dim = os_get_window_dim(window_handle);
        if (window_dim != old_window_dim) {
            //@Note Do not resize if window is minimized
            if (window_dim.x != 0 && window_dim.y != 0) {
                d3d11_resize_render_target_view((UINT)window_dim.x, (UINT)window_dim.y);
                old_window_dim = window_dim;
            }
        }

        Rect draw_region = make_rect(0.f, 0.f, window_dim.x, window_dim.y);
        r_d3d11_state->draw_region = draw_region;

        D3D11_VIEWPORT viewport{};
        viewport.TopLeftX = draw_region.x0;
        viewport.TopLeftY = draw_region.y0;
        viewport.Width = rect_width(draw_region);
        viewport.Height = rect_height(draw_region);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        r_d3d11_state->device_context->RSSetState(r_d3d11_state->rasterizer_state);
        r_d3d11_state->device_context->RSSetViewports(1, &viewport);

        float clear_color[4] = {0, 0, 0, 1};
        r_d3d11_state->device_context->ClearRenderTargetView(r_d3d11_state->render_target_view, clear_color);
        r_d3d11_state->device_context->ClearDepthStencilView(r_d3d11_state->depth_stencil_view, D3D11_CLEAR_DEPTH, 0.0f, 0);

        r_d3d11_state->device_context->OMSetDepthStencilState(r_d3d11_state->depth_stencil_state, 0);
        r_d3d11_state->device_context->OMSetRenderTargets(1, &r_d3d11_state->render_target_view, r_d3d11_state->depth_stencil_view);

        update_and_render(&win32_events, window_handle, dt);

        d3d11_render();

        r_d3d11_state->swap_chain->Present(1, 0);

        r_d3d11_state->ui_vertices.reset_count();

        win32_events.first = nullptr;
        win32_events.last = nullptr;
        win32_events.count = 0;
        arena_clear(win32_event_arena);

        s64 end_clock = get_wall_clock();
        dt = get_seconds_elapsed(last_clock, end_clock);
        #if 0
        printf("DT: %f\n", dt);
        #endif
        last_clock = end_clock;
    }

    return 0;
}
