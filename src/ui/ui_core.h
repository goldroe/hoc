#ifndef UI_CORE_H
#define UI_CORE_H

// extern Face *ui_face;

struct Face;

typedef u32 UI_Hash;

template <class T>
class UI_Stack {
public:
    Auto_Array<T> array;
    bool auto_pop = false;

    bool is_empty() {
        return array.is_empty();
    }
    void push(T elem) {
        array.push(elem);
    }
    void pop() {
        array.pop();
    }
    T& top() {
        return array.back();
    }
    void reset_count() {
        array.reset_count();
    }
};

enum UI_Size_Type {
    UI_SIZE_NIL,
    UI_SIZE_PIXELS,
    UI_SIZE_PARENT_PERCENT,
    UI_SIZE_CHILDREN_SUM,
    UI_SIZE_TEXT_CONTENT,
};

struct UI_Size {
    UI_Size_Type type = UI_SIZE_NIL;
    f32 value = 0.0f;
    f32 strictness = 1.0f;
};

enum UI_Text_Align {
    UI_TEXT_ALIGN_CENTER,
    UI_TEXT_ALIGN_LEFT,
    UI_TEXT_ALIGN_RIGHT,
};

enum UI_Box_Flags {
    UI_BOX_NIL                 = 0,
    UI_BOX_CLICKABLE           = (1<<0),
    UI_BOX_HOVERABLE           = (1<<1),
    UI_BOX_KEYBOARD_INPUT      = (1<<2),

    UI_BOX_FLOATING_X          = (1<<10),
    UI_BOX_FLOATING_Y          = (1<<11),
    UI_BOX_FIXED_WIDTH         = (1<<12),
    UI_BOX_FIXED_HEIGHT        = (1<<13),

    UI_BOX_DRAW_BACKGROUND     = (1<<16),
    UI_BOX_DRAW_BORDER         = (1<<17),
    UI_BOX_DRAW_TEXT           = (1<<18),
    UI_BOX_DRAW_HOT_EFFECTS    = (1<<19),
    UI_BOX_DRAW_ACTIVE_EFFECTS = (1<<20),
};
EnumDefineFlagOperators(UI_Box_Flags);

struct UI_Box;
typedef void (*UI_Box_Draw_Proc)(UI_Box *, void *);
#define UI_BOX_CUSTOM_DRAW_PROC(name) void name(UI_Box *box, void *box_draw_data)

struct UI_Box {
    UI_Box *hash_prev = nullptr;
    UI_Box *hash_next = nullptr;

    UI_Box *parent = nullptr;
    UI_Box *prev = nullptr;
    UI_Box *next = nullptr;
    UI_Box *first = nullptr;
    UI_Box *last = nullptr;

    UI_Hash hash = 0;
    UI_Box_Flags flags = UI_BOX_NIL;

    v2 fixed_position = V2(); // computed relative to parent
    v2 fixed_size = V2(); // computed on requested size
    Rect rect; // final box rectangle
    UI_Size pref_size[AXIS_COUNT];
    Axis2 child_layout_axis = AXIS_Y;
    UI_Text_Align text_alignment = UI_TEXT_ALIGN_CENTER;
    UI_Hash next_focus_key = 0;
    Face *font_face;
    v4 background_color = V4(1.f, 1.f, 1.f, 1.f);
    v4 text_color = V4();
    v4 border_color = V4();
    String8 string = str8_zero();

    //@Note Persistent data
    f32 hot_t = 0.f;
    f32 active_t = 0.f;
    v2 view_offset;
    v2 view_offset_target;

    void *box_draw_data;
    UI_Box_Draw_Proc custom_draw_proc;
};

enum UI_Event_Type {
    UI_EVENT_ERROR,
    UI_EVENT_MOUSE_PRESS,
    UI_EVENT_MOUSE_RELEASE,
    UI_EVENT_PRESS,
    UI_EVENT_RELEASE,
    UI_EVENT_SCROLL,
    UI_EVENT_TEXT,
    UI_EVENT_COUNT
};

struct UI_Event {
    UI_Event *prev;
    UI_Event *next;
    UI_Event_Type type;
    OS_Key key;
    OS_Event_Flags key_modifiers;
    v2i pos;
    v2i delta;
    String8 text;
};

struct UI_Event_List {
    UI_Event *first;
    UI_Event *last;
    int count;
};

struct UI_Hash_Bucket {
    UI_Box *first;
    UI_Box *last;
};

struct UI_State {
    Arena *arena;
    Arena *build_arenas[2];
    u64 build_index = 0;

    UI_Box *root = nullptr;
    UI_Hash focus_active_id = 0;
    UI_Hash active_id = 0;

    v2 mouse_position;
    bool mouse_captured = false;
    bool keyboard_captured = false;
    f32 animation_dt;

    u64 box_table_size;
    UI_Hash_Bucket *box_table;

    UI_Event_List events;
    
    // per frame construction
    UI_Stack<Face*> font_stack;
    UI_Stack<UI_Box*> parent_stack;
    UI_Stack<f32> fixed_x_stack;
    UI_Stack<f32> fixed_y_stack;
    UI_Stack<f32> fixed_width_stack;
    UI_Stack<f32> fixed_height_stack;
    UI_Stack<UI_Size> pref_width_stack;
    UI_Stack<UI_Size> pref_height_stack;
    UI_Stack<Axis2>   child_layout_axis_stack;
    UI_Stack<UI_Text_Align> text_alignment_stack;
    UI_Stack<v4> background_color_stack;
    UI_Stack<v4> border_color_stack;
    UI_Stack<v4> text_color_stack;
};

enum UI_Signal_Flags {
    UI_SIGNAL_CLICKED  = (1 << 0),
    UI_SIGNAL_PRESSED  = (1 << 1),
    UI_SIGNAL_RELEASED = (1 << 2),
    UI_SIGNAL_HOVER    = (1 << 3),
};
EnumDefineFlagOperators(UI_Signal_Flags);

struct UI_Signal {
    UI_Signal_Flags flags;
    UI_Box *box;
    OS_Key key;
    OS_Event_Flags key_modifiers;
    String8 text;
    v2i scroll;
};

#define ui_clicked(sig) ((sig).flags & UI_SIGNAL_CLICKED)
#define ui_hover(sig)   ((sig).flags & UI_SIGNAL_PRESSED)
#define ui_pressed(sig) ((sig).flags & UI_SIGNAL_PRESSED)

internal bool point_in_rect(v2 v, Rect rect);

internal void ui_set_next_font_face(Face *font_face);
internal void ui_set_next_parent(UI_Box *v);
internal void ui_set_next_fixed_x(f32 v);
internal void ui_set_next_fixed_y(f32 v);
internal void ui_set_next_fixed_width(f32 v);
internal void ui_set_next_fixed_height(f32 v);
internal void ui_set_next_pref_width(UI_Size v);
internal void ui_set_next_pref_height(UI_Size v);
internal void ui_set_next_child_layout(Axis2 v);
internal void ui_set_next_text_alignment(UI_Text_Align v);
internal void ui_set_next_background_color(v4 v);
internal void ui_set_next_border_color(v4 v);
internal void ui_set_next_text_color(v4 v);

internal void ui_push_font_face(Face *font_face);
internal void ui_push_parent(UI_Box *box);
internal void ui_push_child_layout_axis(Axis2 axis);
internal void ui_push_text_alignment(UI_Text_Align alignment);
internal void ui_push_pref_width(UI_Size size);
internal void ui_push_pref_height(UI_Size size);
internal void ui_push_fixed_width(f32 width);
internal void ui_push_fixed_height(f32 width);
internal void ui_push_fixed_x(f32 x);
internal void ui_push_fixed_y(f32 y);
internal void ui_push_background_color(v4 color);
internal void ui_push_border_color(v4 color);
internal void ui_push_text_color(v4 color);

internal void ui_pop_font_face();
internal void ui_pop_parent();
internal void ui_pop_child_layout_axis();
internal void ui_pop_text_alignment();
internal void ui_pop_pref_width();
internal void ui_pop_pref_height();
internal void ui_pop_fixed_width();
internal void ui_pop_fixed_height();
internal void ui_pop_fixed_x();
internal void ui_pop_fixed_y();
internal void ui_pop_background_color();
internal void ui_pop_border_color();
internal void ui_pop_text_color();

internal void ui_begin_frame(OS_Handle window_handle);
internal void ui_end_frame();

internal Arena *ui_build_arena();

internal UI_Box *ui_make_box(UI_Hash hash, UI_Box_Flags flags);
internal UI_Box *ui_make_box_from_string(String8 string, UI_Box_Flags flags);
internal UI_Box *ui_box_from_hash(UI_Hash hash);
internal UI_Signal ui_signal_from_box(UI_Box *box);

internal v2 ui_mouse();
internal bool ui_mouse_over_box(UI_Box *box);

internal UI_Hash ui_hash(String8 text);
internal void ui_set_active(UI_Hash hash);
internal UI_Hash ui_active_id();
internal UI_Box *ui_get_root();

internal v2 ui_get_text_position(UI_Box *box);

internal UI_Size ui_size(UI_Size_Type type, f32 value, f32 strictness);
#define ui_px(value, strictness)  (ui_size(UI_SIZE_PIXELS, value, strictness))
#define ui_pct(value, strictness) (ui_size(UI_SIZE_PARENT_PERCENT, value, strictness))
#define ui_children_sum(strictness) (ui_size(UI_SIZE_CHILDREN_SUM, 0.0f, strictness))
#define ui_text_dim(padding, strictness) (ui_size(UI_SIZE_TEXT_CONTENT, padding, strictness))

#define UI_Parent(parent) DeferLoop(ui_push_parent(parent), ui_pop_parent())
#define UI_ChildLayoutAxis(axis) DeferLoop(ui_push_child_layout_axis(axis), ui_pop_child_layout_axis())
#define UI_TextAlignment(align) DeferLoop(ui_push_text_alignment(align), ui_pop_text_alignment())
#define UI_PrefWidth(pref_width) DeferLoop(ui_push_pref_width(pref_width), ui_pop_pref_width())
#define UI_PrefHeight(pref_height) DeferLoop(ui_push_pref_height(pref_height), ui_pop_pref_height())
#define UI_FixedWidth(fixed_width) DeferLoop(ui_push_fixed_width(fixed_width), ui_pop_fixed_width())
#define UI_FixedHeight(fixed_height) DeferLoop(ui_push_fixed_height(fixed_height), ui_pop_fixed_height())
#define UI_FixedX(fixed_x) DeferLoop(ui_push_fixed_x(fixed_x), ui_pop_fixed_x())
#define UI_FixedY(fixed_y) DeferLoop(ui_push_fixed_y(fixed_y), ui_pop_fixed_y())
#define UI_BackgroundColor(color) DeferLoop(ui_push_background_color(color), ui_pop_background_color())
#define UI_BorderColor(color) DeferLoop(ui_push_border_color(color), ui_pop_border_color())
#define UI_TextColor(color) DeferLoop(ui_push_text_color(color), ui_pop_text_color())

#define UI_Row() DeferLoop(ui_row_begin(), ui_row_end())

#endif // UI_CORE_H
