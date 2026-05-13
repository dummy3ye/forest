#ifndef FOREST_H
#define FOREST_H

#include <stdint.h>
#include "render.h"

typedef struct Node {
	float x, y;          // Current position
	float rx, ry;        // Rest position
	float vx, vy;        // Velocity
	float ax, ay;        // Acceleration
	struct Node *parent;
	struct Node **children;
	int child_count;
} Node;

typedef struct {
	Node *root;
	int layer;
} Tree;

typedef struct {
	float world_x;
	Tree *trees;
	int tree_count;
	int active;
} Chunk;

typedef struct {
	float x, y;
	float vx, vy;
	int layer;
	int active;
} Particle;

typedef struct {
	Chunk **chunks; // 2D array [layer][chunk_pool_size]
	int chunk_count;
	float camera_x;
	uint32_t seed;
	Particle *particles;
	int particle_count;
	int width, height;
} Forest;

Forest* create_forest(int width, int height);
void update_forest(Forest *f, float dt, float time);
void draw_forest(Forest *f, VirtualBitmap **bmps, int layer_count);
void draw_ground(VirtualBitmap *bmp, int layer, float camera_x);

#endif // FOREST_H
