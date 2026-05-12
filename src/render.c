#include "../include/render.h"
#include "../include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Allocates and initializes a virtual bitmap */
VirtualBitmap* create_bitmap(int w, int h) {
    VirtualBitmap *bmp = (VirtualBitmap*)malloc(sizeof(VirtualBitmap));
    bmp->width = w;
    bmp->height = h;
    bmp->data = (uint8_t*)calloc(w * h, sizeof(uint8_t));
    return bmp;
}

/* Frees bitmap memory */
void free_bitmap(VirtualBitmap *bmp) {
    if (bmp) {
        free(bmp->data);
        free(bmp);
    }
}

/* Resets all pixels to 0 */
void clear_bitmap(VirtualBitmap *bmp) {
    memset(bmp->data, 0, bmp->width * bmp->height);
}

/* Draws a pixel with basic depth-based collision check */
void draw_pixel_ext(VirtualBitmap *bmp, int x, int y, uint8_t mat) {
    if (x >= 0 && x < bmp->width && y >= 0 && y < bmp->height) {
        if (mat > bmp->data[y * bmp->width + x]) {
            bmp->data[y * bmp->width + x] = mat;
        }
    }
}

/* Bresenham's line algorithm variant for bitmap plotting */
void draw_line_ext(VirtualBitmap *bmp, int x0, int y0, int x1, int y1, uint8_t mat) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        draw_pixel_ext(bmp, x0, y0, mat);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* Maps 2x4 pixel grid to Braille UTF-8 sequence */
static void get_braille(uint8_t pixels[8], char *out) {
    int code = 0;
    if (pixels[0]) code |= 0x01;
    if (pixels[1]) code |= 0x08;
    if (pixels[2]) code |= 0x02;
    if (pixels[3]) code |= 0x10;
    if (pixels[4]) code |= 0x04;
    if (pixels[5]) code |= 0x20;
    if (pixels[6]) code |= 0x40;
    if (pixels[7]) code |= 0x80;

    int unicode = BRAILLE_UNICODE_BASE + code;
    out[0] = (char)(0xE2);
    out[1] = (char)(0xA0 | ((unicode >> 6) & 0x3F));
    out[2] = (char)(0x80 | (unicode & 0x3F));
    out[3] = '\0';
}

/* Renders multi-layer bitmaps to terminal using Braille mapping and depth-fogging */
void render_to_console(VirtualBitmap **bmps, int layer_count) {
    static char *buffer = NULL;
    static int buf_size = 0;

    int term_w = bmps[0]->width / 2;
    int term_h = bmps[0]->height / 4;

    int new_size = term_w * term_h * 60 + 1024;
    if (new_size > buf_size) {
        buffer = (char*)realloc(buffer, new_size);
        buf_size = new_size;
    }

    char *p = buffer;
    p += sprintf(p, "\x1b[H"); 

    int last_r = -1, last_g = -1, last_b = -1;

    for (int y = 0; y < term_h; y++) {
        for (int x = 0; x < term_w; x++) {
            int top_layer = -1;
            uint8_t best_pixels[8] = {0};
            uint8_t dominant_mat = 0;
            
            for (int l = 0; l < layer_count; l++) {
                uint8_t current_pixels[8];
                int has_data = 0;
                uint8_t local_mat = 0;
                for (int dy = 0; dy < 4; dy++) {
                    for (int dx = 0; dx < 2; dx++) {
                        int px = x * 2 + dx;
                        int py = y * 4 + dy;
                        uint8_t val = bmps[l]->data[py * bmps[l]->width + px];
                        current_pixels[dy * 2 + dx] = (val > 0);
                        if (val > local_mat) local_mat = val;
                        if (val) has_data = 1;
                    }
                }
                if (has_data) {
                    top_layer = l;
                    dominant_mat = local_mat;
                    memcpy(best_pixels, current_pixels, 8);
                    break; 
                }
            }

            if (top_layer != -1) {
                float depth = (float)top_layer / layer_count;
                int mr = 0, mg = 0, mb = 0;
                if (dominant_mat == MAT_TRUNK) { mr = 100; mg = 70; mb = 40; }
                else if (dominant_mat == MAT_FOLIAGE) { mr = 40; mg = 100; mb = 40; }
                else if (dominant_mat == MAT_GROUND) { mr = 30; mg = 80; mb = 30; }
                else if (dominant_mat == MAT_ROCK) { mr = 80; mg = 80; mb = 80; }
                else if (dominant_mat == MAT_PARTICLE) { mr = 220; mg = 180; mb = 180; }

                /* Fog blending based on layer depth */
                int fog_r = 15, fog_g = 20, fog_b = 25;
                int r = (int)((1.0f - depth) * mr + depth * fog_r);
                int g = (int)((1.0f - depth) * mg + depth * fog_g);
                int b = (int)((1.0f - depth) * mb + depth * fog_b);
                
                if (r != last_r || g != last_g || b != last_b) {
                    p += sprintf(p, "\x1b[38;2;%d;%d;%dm", r, g, b);
                    last_r = r; last_g = g; last_b = b;
                }
                char braille[4];
                get_braille(best_pixels, braille);
                p += sprintf(p, "%s", braille);
            } else {
                *p++ = ' ';
            }
        }
        *p++ = '\n';
    }
    fwrite(buffer, 1, p - buffer, stdout);
    fflush(stdout);
}
