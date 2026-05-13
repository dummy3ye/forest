#include "sakura.h"
#include <stdlib.h>
#include <math.h>

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
	float lean, current_angle, zig_zag, nx, ny;
	int splits, i;
	SNode *child;

	if (depth <= 0)
		return;

	/* Majestic trees have extreme horizontal reach. */
	lean = (max_depth - depth) * 0.25f;
	current_angle = angle;

	if (angle < -1.57f)
		current_angle -= lean;
	else
		current_angle += lean;

	/* Add gnarled "zig-zag" growth */
	zig_zag = (depth > 4) ? ((rand() % 100 / 100.0f - 0.5f) * 0.6f) : 0;

	nx = parent->x + cosf(current_angle + zig_zag) * length;
	ny = parent->y + sinf(current_angle + zig_zag) * length;

	/* Add "droop" to old heavy limbs */
	if (depth < 5)
		ny += (max_depth - depth) * 1.5f;

	child = create_snode(nx, ny, (depth <= 3));
	add_schild(parent, child);

	if (depth <= 1)
		return;

	/* EXTREME ASYMMETRY: One branch is much longer/stronger than the others */
	splits = (depth > 6) ? 2 : (rand() % 2 + 1);

	for (i = 0; i < splits; i++) {
		float spread, new_len;

		if (i == 0) { /* The dominant limb */
			spread = (rand() % 20 / 100.0f - 0.1f);
			new_len = length * 0.9f;
		} else { /* Subordinate, wide-reaching or drooping branch */
			spread = (rand() % 100 / 100.0f > 0.5 ? 1.2f : -1.2f);
			new_len = length * 0.5f;
		}

		generate_branch(child, current_angle + spread, new_len, depth - 1, max_depth);
	}
}

SakuraTree *create_sakura(float x, float y)
{
	SakuraTree *t = (SakuraTree *)malloc(sizeof(SakuraTree));
	int limbs, i;

	if (!t)
		return NULL;

	t->root = create_snode(x, y, 0);

	/* Start with 2-3 massive, asymmetric limbs */
	limbs = 2 + (rand() % 2);
	for (i = 0; i < limbs; i++) {
		/* Bias limbs to reach WIDE */
		float angle = -1.57f + (i == 0 ? -1.0f : (i == 1 ? 1.0f : 0.2f)) + (rand() % 40 / 100.0f - 0.2f);
		generate_branch(t->root, angle, 40.0f + (rand() % 20), 10, 10);
	}
	return t;
}

static void draw_snode(SNode *n, VirtualBitmap *bmp, int depth)
{
	int i;

	if (n->is_foliage) {
		/* Irregular "Cloud" clusters - patchy and majestic */
		int cloud_count = 3 + rand() % 4;
		int c;

		for (c = 0; c < cloud_count; c++) {
			int r = 8 + rand() % 10;
			float ox = (rand() % 30 - 15);
			float oy = (rand() % 20 - 10);
			int dx, dy;

			for (dy = -r; dy <= r; dy++) {
				for (dx = -r; dx <= r; dx++) {
					if (dx*dx + dy*dy <= r*r) {
						if (rand() % 100 < 60) {
							draw_pixel_ext(bmp, (int)(n->x + ox + dx), (int)(n->y + oy + dy), MAT_SAKURA);
						}
					}
				}
			}
		}
		return;
	}

	/* Majestic Tapering Trunk */
	for (i = 0; i < n->child_count; i++) {
		int thickness = (depth < 2) ? 12 : (depth < 4 ? 7 : (depth < 7 ? 3 : 1));
		int fx, off_x, off_y;

		/* MASSIVE root flare */
		if (depth == 0) {
			for (fx = -35; fx <= 35; fx++) {
				int h = (35 - abs(fx)) / 3 + 2;
				int fy;
				for (fy = 0; fy < h; fy++) {
					draw_pixel_ext(bmp, (int)n->x + fx, (int)n->y - fy, MAT_TRUNK);
				}
			}
		}

		/* Draw gnarled limb */
		for (off_x = -thickness; off_x <= thickness; off_x++) {
			int y_off_max = (thickness > 3) ? (thickness / 3) : 0;
			for (off_y = -y_off_max; off_y <= y_off_max; off_y++) {
				draw_line_ext(bmp, (int)n->x + off_x, (int)n->y + off_y,
							  (int)n->children[i]->x + off_x, (int)n->children[i]->y + off_y, MAT_TRUNK);
			}
		}
		draw_snode(n->children[i], bmp, depth + 1);
	}
}

void draw_sakura(SakuraTree *t, VirtualBitmap *bmp)
{
	draw_snode(t->root, bmp, 0);
}
