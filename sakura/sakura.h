#ifndef SAKURA_H
#define SAKURA_H

#include "../include/render.h"

// Massive Sakura Structure
typedef struct SNode {
    float x, y;
    float rx, ry;
    struct SNode **children;
    int child_count;
    int is_foliage; // 1 if this node is a foliage "cloud"
} SNode;

typedef struct {
    SNode *root;
    int layer;
} SakuraTree;

SakuraTree* create_sakura(float x, float y);
void draw_sakura(SakuraTree *t, VirtualBitmap *bmp);

#endif
