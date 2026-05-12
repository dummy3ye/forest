#include "sakura.h"
#include <stdlib.h>
#include <math.h>

static SNode* create_snode(float x, float y, int is_foliage) {
    SNode *n = (SNode*)malloc(sizeof(SNode));
    n->x = n->rx = x;
    n->y = n->ry = y;
    n->children = NULL;
    n->child_count = 0;
    n->is_foliage = is_foliage;
    return n;
}

static void add_schild(SNode *parent, SNode *child) {
    parent->child_count++;
    parent->children = (SNode**)realloc(parent->children, sizeof(SNode*) * parent->child_count);
    parent->children[parent->child_count - 1] = child;
}

static void generate_branch(SNode *parent, float angle, float length, int depth) {
    if (depth <= 0) return;
    float nx = parent->x + cosf(angle) * length;
    float ny = parent->y + sinf(angle) * length;
    
    // Last depth becomes a foliage cloud
    SNode *child = create_snode(nx, ny, (depth == 1));
    add_schild(parent, child);
    
    // Split branches
    if (depth > 1) {
        generate_branch(child, angle - 0.4f, length * 0.7f, depth - 1);
        generate_branch(child, angle + 0.4f, length * 0.7f, depth - 1);
    }
}

SakuraTree* create_sakura(float x, float y) {
    SakuraTree *t = (SakuraTree*)malloc(sizeof(SakuraTree));
    t->root = create_snode(x, y, 0);
    
    // Gnarled Split-Trunk base
    for(int i=0; i<3; i++) {
        float angle = -1.57f + (i - 1.0f) * 0.5f;
        generate_branch(t->root, angle, 40.0f, 6);
    }
    return t;
}

static void draw_snode(SNode *n, VirtualBitmap *bmp) {
    if (n->is_foliage) {
        // Draw pink "Cloud"
        int r = 8;
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (dx*dx + dy*dy <= r*r) {
                    draw_pixel_ext(bmp, (int)n->x + dx, (int)n->y + dy, MAT_FOLIAGE);
                }
            }
        }
    } else {
        // Trunk / Branch
        for (int i=0; i<n->child_count; i++) {
            draw_line_ext(bmp, (int)n->x, (int)n->y, (int)n->children[i]->x, (int)n->children[i]->y, MAT_TRUNK);
            draw_snode(n->children[i], bmp);
        }
    }
}

void draw_sakura(SakuraTree *t, VirtualBitmap *bmp) {
    draw_snode(t->root, bmp);
}
