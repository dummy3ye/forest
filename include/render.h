#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

typedef enum {
	MAT_EMPTY = 0,
	MAT_TRUNK = 1,
	MAT_FOLIAGE = 2,
	MAT_GROUND = 3,
	MAT_ROCK = 4,
	MAT_PARTICLE = 5,
	MAT_SAKURA = 6
} Material;

typedef struct {
	int width;
	int height;
	uint8_t *data; // Stores Material ID
} VirtualBitmap;

VirtualBitmap* create_bitmap(int w, int h);
void free_bitmap(VirtualBitmap *bmp);
void clear_bitmap(VirtualBitmap *bmp);
void draw_pixel_ext(VirtualBitmap *bmp, int x, int y, uint8_t mat);
void draw_line_ext(VirtualBitmap *bmp, int x0, int y0, int x1, int y1, uint8_t mat);

// ANSI Rendering
void render_to_console(VirtualBitmap **bmps, int layer_count);

#endif // RENDER_H
