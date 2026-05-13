#include "sakura.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

static SNode *create_snode(float x, float y, int is_foliage)
{
	SNode *n = (SNode *)malloc(sizeof(SNode));

	if (!n)
		return NULL;

	n->x = n->rx = x;
	n->y = n->ry = y;
	n->children = NULL;
	n->child_count = 0;
	n->is_foliage = is_foliage;
	return n;
}

static void add_schild(SNode *parent, SNode *child)
{
	if (!child)
		return;

	parent->child_count++;
	parent->children = (SNode **)realloc(parent->children, sizeof(SNode *) * parent->child_count);
	parent->children[parent->child_count - 1] = child;
}

static void generate_branch(SNode *parent, float angle, float length, int depth, int max_depth)
{
	float current_angle, zig_zag, nx, ny;
	int splits, i;
	SNode *child;

	if (depth <= 0)
		return;

	current_angle = angle;

	// Ancient Bonsai gnarliness: moderate but clear twisting
	zig_zag = (depth > 2) ? ((rand() % 100 / 100.0f - 0.5f) * 0.7f) : 0;

	nx = parent->x + cosf(current_angle + zig_zag) * length;
	ny = parent->y + sinf(current_angle + zig_zag) * length;

	child = create_snode(nx, ny, (depth <= 4));
	add_schild(parent, child);

	if (depth <= 1)
		return;

	splits = (rand() % 100 < 80) ? 2 : 1;

	for (i = 0; i < splits; i++) {
		float spread, new_len;
		if (splits == 1) {
			spread = (rand() % 60 / 100.0f - 0.3f);
			new_len = length * 0.88f;
		} else {
			spread = (i == 0) ? -(0.5f + (rand() % 60 / 100.0f)) : (0.5f + (rand() % 60 / 100.0f));
			new_len = length * 0.82f;
		}
		generate_branch(child, current_angle + spread, new_len, depth - 1, max_depth);
	}
}

static void collect_foliage(SNode *n, SakuraTree *t)
{
	int i;
	if (n->is_foliage) {
		int cloud_count = 2 + rand() % 2;
		int c;
		for (c = 0; c < cloud_count; c++) {
			int r = 8 + rand() % 6; // Airy puffs
			float ox = (float)(rand() % 30 - 15);
			float oy = (float)(rand() % 20 - 10);
			int dx, dy;
			for (dy = -r; dy <= r; dy++) {
				for (dx = -r; dx <= r; dx++) {
					if (dx*dx + dy*dy <= r*r) {
						if (rand() % 100 < 35) {
							if (t->foliage_count >= t->foliage_capacity) {
								t->foliage_capacity = (t->foliage_capacity == 0) ? 2048 : t->foliage_capacity * 2;
								t->foliage_points = (FoliagePoint *)realloc(t->foliage_points, sizeof(FoliagePoint) * t->foliage_capacity);
							}
							t->foliage_points[t->foliage_count].x = n->x + ox + (float)dx;
							t->foliage_points[t->foliage_count].y = n->y + oy + (float)dy;
							t->foliage_count++;
						}
					}
				}
			}
		}
		return;
	}

	for (i = 0; i < n->child_count; i++) {
		collect_foliage(n->children[i], t);
	}
}

SakuraTree *create_sakura(float x, float y)
{
	SakuraTree *t = (SakuraTree *)malloc(sizeof(SakuraTree));
	if (!t) return NULL;

	t->root = create_snode(x, y, 0);
	t->foliage_points = NULL;
	t->foliage_count = 0;
	t->foliage_capacity = 0;

	// Tall, powerful, slightly leaning trunk
	SNode *current = t->root;
	float trunk_len = 16.0f; 
	for (int i = 0; i < 4; i++) {
		SNode *next = create_snode(current->x + (rand()%8-4), current->y - trunk_len, 0);
		add_schild(current, next);
		current = next;
	}

	// Wide, majestic canopy
	int limbs = 6 + (rand() % 2);
	for (int i = 0; i < limbs; i++) {
		float angle = -1.57f + ((float)i - (limbs-1)/2.0f) * 0.9f;
		generate_branch(current, angle, 18.0f + (rand() % 8), 10, 10);
	}

	collect_foliage(t->root, t);
	return t;
}

static void free_snode(SNode *n)
{
	int i;
	for (i = 0; i < n->child_count; i++) {
		free_snode(n->children[i]);
	}
	free(n->children);
	free(n);
}

void free_sakura(SakuraTree *t)
{
	if (!t) return;
	free_snode(t->root);
	free(t->foliage_points);
	free(t);
}

static void force_pixel(VirtualBitmap *bmp, int x, int y, uint8_t mat) {
	if (x >= 0 && x < bmp->width && y >= 0 && y < bmp->height) {
		bmp->data[y * bmp->width + x] = mat;
	}
}

static void force_line(VirtualBitmap *bmp, int x0, int y0, int x1, int y1, uint8_t mat) {
	int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2;
	while (1) {
		force_pixel(bmp, x0, y0, mat);
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

static void draw_tree_recursive(SNode *n, VirtualBitmap *bmp, int depth)
{
	if (n->is_foliage) return;
	for (int i = 0; i < n->child_count; i++) {
		// Massive trunk tapering to thin branches
		int thickness = (depth < 2) ? (30 - depth * 8) : (depth < 4 ? 10 : (depth < 7 ? 4 : 1));
		if (thickness < 1) thickness = 1;

		for (int ox = -thickness; ox <= thickness; ox++) {
			int y_off = (thickness > 10) ? (thickness / 4) : 0;
			for (int oy = -y_off; oy <= y_off; oy++) {
				force_line(bmp, (int)n->x + ox, (int)n->y + oy, 
						   (int)n->children[i]->x + ox, (int)n->children[i]->y + oy, MAT_TRUNK);
			}
		}
		draw_tree_recursive(n->children[i], bmp, depth + 1);
	}
}

void draw_sakura(SakuraTree *t, VirtualBitmap *bmp)
{
	// 1. Draw Ground Base (Anchors the tree)
	for (int gy = 0; gy < 10; gy++) {
		int w = 50 - gy * 3;
		for (int gx = -w; gx <= w; gx++) {
			force_pixel(bmp, (int)t->root->x + gx, (int)t->root->y + gy, MAT_GROUND);
		}
	}

	// 2. Draw Anchor Roots (Prevents "floating")
	for (int r = 0; r < 6; r++) {
		float rx = t->root->x + (rand() % 100 - 50);
		force_line(bmp, (int)t->root->x, (int)t->root->y, (int)rx, bmp->height - 1, MAT_TRUNK);
	}

	// 3. Draw Foliage clouds FIRST
	for (int i = 0; i < t->foliage_count; i++) {
		draw_pixel_ext(bmp, (int)t->foliage_points[i].x, (int)t->foliage_points[i].y, MAT_SAKURA);
	}

	// 4. Draw Trunk and Branches ON TOP (The Skeleton)
	draw_tree_recursive(t->root, bmp, 0);
}
