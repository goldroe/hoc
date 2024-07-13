#ifndef RENDER_CORE_H
#define RENDER_CORE_H

struct R_2D_Vertex {
    v2 dst;
    v2 src;
    v4 color;
};

enum R_Params_Type {
    R_PARAMS_UI,
};

struct R_Params_UI {
    m4 xform;
    void *tex;
};

union R_Params {
    R_Params_Type type;
    R_Params_UI ui;
};

struct R_Batch {
    R_Params params;
    u8 *v;
    int bytes;
};

struct R_Batch_Node {
    R_Batch_Node *next;
    R_Batch batch;
};

struct R_Batch_List {
    R_Batch_Node *first;
    R_Batch_Node *last;
    int count;
};

#endif // RENDER_CORE_H
