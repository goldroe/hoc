#include "draw.h"

global Arena *draw_arena;
global Draw_Bucket *draw_bucket;

internal void draw_begin(OS_Handle window_handle) {
    if (draw_arena == nullptr) {
        draw_arena = arena_alloc(get_malloc_allocator(), MB(16));
    }

    draw_bucket = push_array(draw_arena, Draw_Bucket, 1);
    *draw_bucket = {};

    v2 window_dim = os_get_window_dim(window_handle);
    draw_bucket->clip = make_rect(0.f, 0.f, window_dim.x, window_dim.y);
}

internal void draw_end() {
    arena_clear(draw_arena);
    draw_bucket = nullptr;
}

internal void draw_set_clip(Rect clip) {
    draw_bucket->clip = clip;
    R_Batch_Node *node = draw_bucket->batches.last;
    node = push_array(draw_arena, R_Batch_Node, 1);
    node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
    node->batch.params.type = R_PARAMS_UI;
    node->batch.params.ui.tex = draw_bucket->tex;
    // node->batch.params.
}

internal void draw_batch_push_vertex(R_Batch *batch, R_2D_Vertex src) {
    R_2D_Vertex *dst = push_array(draw_arena, R_2D_Vertex, 1);
    *dst = src;
    batch->bytes += sizeof(R_2D_Vertex);
 }

internal void draw_push_batch_node(R_Batch_List *list, R_Batch_Node *node) {
    if (list->first) {
        list->last->next = node;
    } else {
        list->first = node;
    }
    list->last = node;
    list->count += 1;
}

internal void draw_string(String8 string, Face *font_face, v4 color, v2 offset) {
    void *tex = draw_bucket->tex;
    R_Batch_Node *node = draw_bucket->batches.last;
    if (!node || tex != font_face->texture) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
        node->batch.params.type = R_PARAMS_UI;
        node->batch.params.ui.tex = font_face->texture;
        node->batch.params.ui.clip = draw_bucket->clip;
        draw_push_batch_node(&draw_bucket->batches, node);
        draw_bucket->tex = font_face->texture;
    }

    v2 cursor = offset;
    for (u64 i = 0; i < string.count; i++) {
        u8 c = string.data[i];
        if (c == '\n') {
            cursor.x = offset.x;
            cursor.y += font_face->glyph_height;
            continue;
        }
        Glyph g = font_face->glyphs[c];

        Rect dst;
        dst.x0 = cursor.x + g.bl;
        dst.x1 = dst.x0 + g.bx;
        dst.y0 = cursor.y - g.bt + font_face->ascend;
        dst.y1 = dst.y0 + g.by;

        Rect src;
        src.x0 = g.to;
        src.y0 = 0.f;
        src.x1 = src.x0 + (g.bx / (f32)font_face->width);
        src.y1 = src.y0 + (g.by / (f32)font_face->height);

        R_2D_Vertex tl = { V2(dst.x0, dst.y0), V2(src.x0, src.y0), color };
        R_2D_Vertex tr = { V2(dst.x1, dst.y0), V2(src.x1, src.y0), color };
        R_2D_Vertex bl = { V2(dst.x0, dst.y1), V2(src.x0, src.y1), color };
        R_2D_Vertex br = { V2(dst.x1, dst.y1), V2(src.x1, src.y1), color };

        draw_batch_push_vertex(&node->batch, bl);
        draw_batch_push_vertex(&node->batch, tl);
        draw_batch_push_vertex(&node->batch, tr);
        draw_batch_push_vertex(&node->batch, br);

        cursor.x += g.ax;
    }
}

internal void draw_rect(Rect dst, v4 color) {
    void *tex = draw_bucket->tex;
    R_Batch_Node *node = draw_bucket->batches.last;

    if (!node || tex != nullptr) {
        node = push_array(draw_arena, R_Batch_Node, 1);
        node->batch.v = (u8 *)draw_arena->current + draw_arena->current->pos;
        node->batch.params.type = R_PARAMS_UI;
        node->batch.params.ui.tex = nullptr;
        node->batch.params.ui.clip = draw_bucket->clip;
        draw_push_batch_node(&draw_bucket->batches, node);

        draw_bucket->tex = nullptr;
    }
    Assert(node);

    R_2D_Vertex tl = { V2(dst.x0, dst.y0), V2(0.f, 0.f), color };
    R_2D_Vertex tr = { V2(dst.x1, dst.y0), V2(0.f, 0.f), color };
    R_2D_Vertex bl = { V2(dst.x0, dst.y1), V2(0.f, 0.f), color };
    R_2D_Vertex br = { V2(dst.x1, dst.y1), V2(0.f, 0.f), color };

    draw_batch_push_vertex(&node->batch, bl);
    draw_batch_push_vertex(&node->batch, tl);
    draw_batch_push_vertex(&node->batch, tr);
    draw_batch_push_vertex(&node->batch, br);
}

internal void draw_rect_outline(Rect rect, v4 color) {
    draw_rect(make_rect(rect.x0, rect.y0, rect_width(rect), 1), color);
    draw_rect(make_rect(rect.x0, rect.y0, 1, rect_height(rect)), color);
    draw_rect(make_rect(rect.x1 - 1, rect.y0, 1, rect_height(rect)), color);
    draw_rect(make_rect(rect.x0, rect.y1 - 1, rect_width(rect), 1), color);
}
