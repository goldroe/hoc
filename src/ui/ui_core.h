#ifndef UI_CORE_H
#define UI_CORE_H

typedef u64 UI_Key;

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
    UI_BOX_KEYBOARD_CLICKABLE  = (1<<1),
    UI_BOX_SCROLL              = (1<<4),
    UI_BOX_FLOATING_X          = (1<<8),
    UI_BOX_FLOATING_Y          = (1<<9),
    UI_BOX_FIXED_WIDTH         = (1<<10),
    UI_BOX_FIXED_HEIGHT        = (1<<11),
    UI_BOX_OVERFLOW_X          = (1<<12),
    UI_BOX_OVERFLOW_Y          = (1<<13),

    UI_BOX_DRAW_BACKGROUND     = (1<<16),
    UI_BOX_DRAW_BORDER         = (1<<17),
    UI_BOX_DRAW_TEXT           = (1<<18),
    UI_BOX_DRAW_HOT_EFFECTS    = (1<<19),
    UI_BOX_DRAW_ACTIVE_EFFECTS = (1<<20),
};
EnumDefineFlagOperators(UI_Box_Flags);

struct UI_Box;
#define UI_BOX_CUSTOM_DRAW_PROC(name) void name(UI_Box *box, void *user_data)
typedef UI_BOX_CUSTOM_DRAW_PROC(UI_Box_Draw_Proc);

struct UI_Box {
    UI_Box *hash_prev = nullptr;
    UI_Box *hash_next = nullptr;

    UI_Box *parent = nullptr;
    UI_Box *prev = nullptr;
    UI_Box *next = nullptr;
    UI_Box *first = nullptr;
    UI_Box *last = nullptr;
    int child_count = 0;

    UI_Key key = 0;
    UI_Box_Flags flags = UI_BOX_NIL;
    int box_id = 0 ; //@Note Shows order of creation

    v2 fixed_position = V2(); // computed relative to parent
    v2 fixed_size = V2(); // computed on requested size
    Rect rect;
    UI_Size pref_size[AXIS_COUNT];
    Axis2 child_layout_axis = AXIS_Y;
    UI_Text_Align text_alignment = UI_TEXT_ALIGN_CENTER;
    // UI_Key next_focus_key = 0;
    Face *font;
    v4 background_color = V4(1.f, 1.f, 1.f, 1.f);
    v4 text_color = V4();
    v4 border_color = V4();
    String8 string = str8_zero();
    UI_Box_Draw_Proc *custom_draw_proc;
    void *draw_data;

    //@Note Persistent data
    v2 view_offset;
    v2 view_offset_target;
    f32 hot_t = 0.f;
    f32 active_t = 0.f;
    f32 focus_active_t = 0.f;
};

struct UI_Font_Node                    {UI_Font_Node            *next; Face *v;};
struct UI_Parent_Node                  {UI_Parent_Node          *next; UI_Box *v;};
struct UI_FixedX_Node                  {UI_FixedX_Node          *next; f32 v;};
struct UI_FixedY_Node                  {UI_FixedY_Node          *next; f32 v;};
struct UI_FixedWidth_Node              {UI_FixedWidth_Node      *next; f32 v;};
struct UI_FixedHeight_Node             {UI_FixedHeight_Node     *next; f32 v;};
struct UI_PrefWidth_Node               {UI_PrefWidth_Node       *next; UI_Size v;};
struct UI_PrefHeight_Node              {UI_PrefHeight_Node      *next; UI_Size v;};
struct UI_ChildLayoutAxis_Node         {UI_ChildLayoutAxis_Node *next; Axis2 v;};
struct UI_TextAlignment_Node           {UI_TextAlignment_Node   *next; UI_Text_Align v;};
struct UI_BackgroundColor_Node         {UI_BackgroundColor_Node *next; v4 v;};
struct UI_BorderColor_Node             {UI_BorderColor_Node     *next; v4 v;};
struct UI_TextColor_Node               {UI_TextColor_Node       *next; v4 v;};

#define UI_StackDecls \
    struct {                                                            \
    struct { UI_Font_Node            *top; UI_Font_Node            *first_free; b32 auto_pop; } font_stack; \
    struct { UI_Parent_Node          *top; UI_Parent_Node          *first_free; b32 auto_pop; } parent_stack; \
    struct { UI_FixedX_Node          *top; UI_FixedX_Node          *first_free; b32 auto_pop; } fixed_x_stack; \
    struct { UI_FixedY_Node          *top; UI_FixedY_Node          *first_free; b32 auto_pop; } fixed_y_stack; \
    struct { UI_FixedWidth_Node      *top; UI_FixedWidth_Node      *first_free; b32 auto_pop; } fixed_width_stack; \
    struct { UI_FixedHeight_Node     *top; UI_FixedHeight_Node     *first_free; b32 auto_pop; } fixed_height_stack; \
    struct { UI_PrefWidth_Node       *top; UI_PrefWidth_Node       *first_free; b32 auto_pop; } pref_width_stack; \
    struct { UI_PrefHeight_Node      *top; UI_PrefHeight_Node      *first_free; b32 auto_pop; } pref_height_stack; \
    struct { UI_ChildLayoutAxis_Node *top; UI_ChildLayoutAxis_Node *first_free; b32 auto_pop; } child_layout_axis_stack; \
    struct { UI_TextAlignment_Node   *top; UI_TextAlignment_Node   *first_free; b32 auto_pop; } text_alignment_stack; \
    struct { UI_BackgroundColor_Node *top; UI_BackgroundColor_Node *first_free; b32 auto_pop; } background_color_stack; \
    struct { UI_BorderColor_Node     *top; UI_BorderColor_Node     *first_free; b32 auto_pop; } border_color_stack; \
    struct { UI_TextColor_Node       *top; UI_TextColor_Node       *first_free; b32 auto_pop; } text_color_stack; \
}

#define UI_StackPush(state,upper,lower,type,value)                  \
    UI_##upper##_Node *node = state->lower##_stack.first_free;      \
    if (node != NULL) {                                             \
        SLLStackPop(state->lower##_stack.first_free);               \
    } else {                                                        \
        node = push_array(ui_build_arena(), UI_##upper##_Node, 1);   \
    }                                                               \
    node->v = value;                                                \
    SLLStackPush(state->lower##_stack.top, node);                   \
    state->lower##_stack.auto_pop = false;                          \

#define UI_StackPop(state,upper,lower)                          \
    UI_##upper##_Node *node = state->lower##_stack.top;         \
    if (node != NULL) {                                         \
        SLLStackPop(state->lower##_stack.top);                  \
        SLLStackPush(state->lower##_stack.first_free, node);    \
        state->lower##_stack.auto_pop = false;                  \
    }                                                           \

#define UI_StackSetNext(state,upper,lower,type,value)               \
    UI_##upper##_Node *node = state->lower##_stack.first_free;      \
    if (node != NULL) {                                            \
        SLLStackPop(state->lower##_stack.first_free);               \
    } else {                                                        \
        node = push_array(ui_build_arena(), UI_##upper##_Node, 1);   \
    }                                                               \
    node->v = value;                                                \
    SLLStackPush(state->lower##_stack.top, node);                   \
    state->lower##_stack.auto_pop = true;                           \

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

    int build_counter = 0;
    Auto_Array<UI_Box> last_build_collection;
    UI_Box *root = nullptr;

    UI_Key focus_active_box_key = 0;
    UI_Key active_box_key = 0;
    UI_Key hot_box_key = 0;

    v2 mouse_position;
    v2i mouse_drag_start;
    b32 mouse_dragging;

    u64 box_table_size;
    UI_Hash_Bucket *box_table;

    UI_Event_List events;
    
    f32 animation_dt;

    UI_StackDecls;
};

enum UI_Signal_Flags {
    UI_SIGNAL_CLICKED  = (1<<0),
    UI_SIGNAL_PRESSED  = (1<<1),
    UI_SIGNAL_RELEASED = (1<<2),
    UI_SIGNAL_HOVER    = (1<<3),
    UI_SIGNAL_SCROLL   = (1<<4),
    UI_SIGNAL_DRAGGING = (1<<5),
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

internal v2 ui_drag_delta();

#define ui_clicked(sig)      ((sig).flags & UI_SIGNAL_CLICKED)
#define ui_hover(sig)        ((sig).flags & UI_SIGNAL_PRESSED)
#define ui_pressed(sig)      ((sig).flags & UI_SIGNAL_PRESSED)
#define ui_scroll(sig)       ((sig).flags & UI_SIGNAL_SCROLL)
#define ui_dragging(sig)     ((sig).flags & UI_SIGNAL_DRAGGING)

internal UI_Box *ui_top_parent();

internal void ui_set_next_font(Face *v);
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

internal void ui_push_font(Face *v);
internal void ui_push_parent(UI_Box *v);
internal void ui_push_fixed_x(f32 v);
internal void ui_push_fixed_y(f32 v);
internal void ui_push_fixed_width(f32 v);
internal void ui_push_fixed_height(f32 v);
internal void ui_push_pref_width(UI_Size v);
internal void ui_push_pref_height(UI_Size v);
internal void ui_push_child_layout_axis(Axis2 v);
internal void ui_push_text_alignment(UI_Text_Align v);
internal void ui_push_background_color(v4 v);
internal void ui_push_border_color(v4 v);
internal void ui_push_text_color(v4 v);

internal void ui_pop_font();
internal void ui_pop_parent();
internal void ui_pop_fixed_x();
internal void ui_pop_fixed_y();
internal void ui_pop_fixed_width();
internal void ui_pop_fixed_height();
internal void ui_pop_pref_width();
internal void ui_pop_pref_height();
internal void ui_pop_child_layout_axis();
internal void ui_pop_text_alignment();
internal void ui_pop_background_color();
internal void ui_pop_border_color();
internal void ui_pop_text_color();

internal void ui_begin_frame(OS_Handle window_handle);
internal void ui_end_frame();

internal Arena *ui_build_arena();

internal UI_Box *ui_make_box(UI_Key key, UI_Box_Flags flags);
internal UI_Box *ui_make_box_from_string(UI_Box_Flags flags, String8 string);
internal UI_Box *ui_box_from_key(UI_Key key);
internal UI_Signal ui_signal_from_box(UI_Box *box);

internal UI_Box *ui_root();

internal v2 ui_text_position(UI_Box *box);

internal UI_Size ui_size(UI_Size_Type type, f32 value, f32 strictness);
#define ui_px(value, strictness)  (ui_size(UI_SIZE_PIXELS, value, strictness))
#define ui_pct(value, strictness) (ui_size(UI_SIZE_PARENT_PERCENT, value, strictness))
#define ui_children_sum(strictness) (ui_size(UI_SIZE_CHILDREN_SUM, 0.0f, strictness))
#define ui_text_dim(padding, strictness) (ui_size(UI_SIZE_TEXT_CONTENT, padding, strictness))

#define UI_Font(font)     DeferLoop(ui_push_font(font), ui_pop_font())
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
