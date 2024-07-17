#ifndef DRAW_H
#define DRAW_H

struct Draw_Bucket {
    m4 xform;
    void *tex;
    Rect clip;
    R_Batch_List batches;
};

#endif // DRAW_H
