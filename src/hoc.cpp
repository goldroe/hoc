#include <ft2build.h>
#include <freetype/freetype.h>

#include "core/core.h"
#include "core/core_arena.h"
#include "core/core_strings.h"
#include "core/core_math.h"

#pragma warning(push)
#pragma warning( disable : 4244)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_DECORATE(name) hoc_##name
#include <stb_sprintf.h>

#include "auto_array.h"
#include "utils.h"
#include "os/os.h"
#include "path.h"
#include "font.h"
#include "lexer.h"
#include "ui/ui_core.h"
#include "ui/ui_widgets.h"
#include "render_core.h"
#include "render_d3d11.h"
#include "draw.h"
#include "hoc.h"
#include "hoc/hoc_buffer.h"
#include "hoc/hoc_editor.h"
#include "gui.h"
#include "hoc/hoc_app.h"

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
#include "hoc/hoc_app.cpp"
#include "hoc/hoc_buffer.cpp"
#include "hoc/hoc_editor.cpp"
#include "hoc_commands.cpp"
#include "generated_hoc_commands.cpp"
#include "gui.cpp"

global s64 performance_frequency;

global Arena *win32_event_arena;
global OS_Event_List win32_events;

global Arena *key_map_arena;
global Key_Map *default_key_map;

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
        if (str8_match(key_name, name)) {
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
        if (str8_match(name, cmd->name)) {
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
            if (str8_match(key_mod.literal, str8_lit("Control"))) {
                key_value |= KEY_MOD_CONTROL;
            } else if (str8_match(key_mod.literal, str8_lit("Alt"))) {
                key_value |= KEY_MOD_ALT;
            } else if (str8_match(key_mod.literal, str8_lit("Shift"))) {
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

internal inline s64 get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return (s64)result.QuadPart;
}

internal inline f32 get_seconds_elapsed(s64 start, s64 end) {
    f32 result = ((f32)(end - start) / (f32)performance_frequency);
    return result;
}

internal void draw_ui_box(UI_Box *box) {
    if (box->flags & UI_BOX_DRAW_BACKGROUND) {
        draw_rect(box->rect, box->background_color);
    }
    if (box->flags & UI_BOX_DRAW_BORDER) {
        // draw_rect_outline(box->rect, box->border_color);
    }
    if (box->flags & UI_BOX_DRAW_TEXT) {
        v2 text_position = ui_get_text_position(box);
        text_position += box->view_offset;
        draw_string(box->string, box->font_face, box->text_color, text_position);
    }
}

internal void draw_ui_layout(UI_Box *box) {
    if (box->custom_draw_proc) {
        box->custom_draw_proc(box, box->draw_data);
    } else if (box != ui_get_root()) {
        draw_ui_box(box);
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
    UI_Box *main_body = ui_make_box_from_string(UI_BOX_NIL, str8_lit("main_code_body"));

    //@Note Default the focus to the first view
    // if (ui_focus_active_id() == 0 && hoc_app->editors.first->box) {
    //     ui_set_focus_active(hoc_app->editors.first->box->hash);
    // }

    UI_Parent(main_body)
        UI_BackgroundColor(V4(1.f, 1.f, 1.f, 1.f))
        UI_BorderColor(V4(.2f, .19f, .18f, 1.f))
        UI_TextColor(V4(.2f, .2f, .2f, 1.f))
        // UI_BackgroundColor(V4(.2f, .19f, .18f, 1.f))
        // UI_BorderColor(V4(.2f, .19f, .18f, 1.f))
        // UI_TextColor(V4(.92f, .86f, .7f, 1.f))
    {
        for (GUI_View *view = hoc_app->gui_views.first; view != nullptr; view = view->next) {
            gui_view_update(view);
        }
    }

    ui_layout_apply(ui_state->root);
    draw_ui_layout(ui_state->root);

    ui_end_build();
}

int main(int argc, char **argv) {
    argc--; argv++;
    Arena *arg_arena = make_arena(get_malloc_allocator());
    int argument_count = argc;
    String8 *arguments = push_array(arg_arena, String8, argc);
    for (int i = 0; i < argc; i++) {
        char *arg = argv[i];
        arguments[i] = str8_copy(arg_arena, str8_cstring(arg));
    }

    String8 file_name = str8_lit("*scratch*");
    for (int i = 0; i < argument_count; i++) {
        String8 argument = arguments[i];
        if (argument.data && argument.data[0] == '-') {
            continue; 
        } 
        if (argument.data) {
            file_name = argument;
            break;
        }
    }

    String8 current_directory = path_current_dir(arg_arena);
    printf("CD %s\n", current_directory.data);

    if (path_is_relative(file_name)) {
        file_name = str8_concat(arg_arena, current_directory, file_name);
    }

    printf("file name: %s\n", file_name.data);

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
   
    win32_event_arena = make_arena(get_malloc_allocator());

    hoc_app = make_hoc_application();
    ui_set_state(ui_state_new());

    default_fonts[FONT_DEFAULT] = load_font_face(str8_lit("fonts/consolas.ttf"), 16);
    default_fonts[FONT_CODE] = load_font_face(str8_lit("fonts/LiberationMono.ttf"), 14);

    key_map_arena = make_arena(get_malloc_allocator());
    default_key_map = load_key_map(key_map_arena, str8_lit("data/bindings.hoc"));

    Arena *string_arena = make_arena(get_malloc_allocator());
    hoc_app->current_directory = current_directory;

    GUI_View *def_editor_view = make_gui_editor();
    def_editor_view->key_map = default_key_map;
    Hoc_Editor *def_editor = def_editor_view->editor.editor;
    def_editor->buffer = make_buffer(file_name);
    def_editor->face = default_fonts[FONT_CODE];
    
    // View *split_view = view_new();
    // split_view->buffer = make_buffer(str8_lit("test"));
    // split_view->key_map = default_key_map;
    // split_view->face = default_font_face;
    // push_view(split_view);
    // push_buffer(split_view->buffer);
    // split_view->panel_dim = V2(900.f, 1000.f);

    // hoc_app->active_editor = default_editor;

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

        draw_begin();

        update_and_render(&win32_events, window_handle, dt);

        d3d11_render(draw_bucket);

        r_d3d11_state->swap_chain->Present(1, 0);

        win32_events.first = nullptr;
        win32_events.last = nullptr;
        win32_events.count = 0;
        arena_clear(win32_event_arena);

        draw_end();

        s64 end_clock = get_wall_clock();
        dt = get_seconds_elapsed(last_clock, end_clock);
        #if 0
        printf("DT: %f\n", dt);
        #endif
        last_clock = end_clock;
    }
    return 0;
}
