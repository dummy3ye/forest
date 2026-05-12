#include "../include/forest.h"
#include "../include/config.h"
#include "../include/noise.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

/* Creates a tree segment node with initial physics state */
static Node *create_node(float x, float y, Node *parent) {
  Node *n = (Node *)malloc(sizeof(Node));
  n->x = n->rx = x;
  n->y = n->ry = y;
  n->vx = n->vy = 0;
  n->ax = n->ay = 0;
  n->parent = parent;
  n->children = NULL;
  n->child_count = 0;
  return n;
}

/* Attaches a child segment node to parent */
static void add_child(Node *parent, Node *child) {
  parent->child_count++;
  parent->children =
      (Node **)realloc(parent->children, sizeof(Node *) * parent->child_count);
  parent->children[parent->child_count - 1] = child;
}

/* Xorshift32 PRNG for deterministic procedural generation */
static uint32_t xorshift32(uint32_t *state) {
  uint32_t x = *state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  *state = x;
  return x;
}

/* Maps PRNG output to float range [0.0, 1.0] */
static float seeded_rand(uint32_t *state) {
  return (float)(xorshift32(state) & 0xFFFFFF) / 16777216.0f;
}

/* Renders needle clusters along branch tips */
static void draw_needle_cluster(VirtualBitmap *bmp, float x, float y,
                                float length, float angle, float density,
                                float offset_x) {
  int needle_count = (int)(length * density);
  uint32_t needle_seed = (uint32_t)x ^ (uint32_t)y;
  for (int i = 0; i < needle_count; i++) {
    float jitter_x = (seeded_rand(&needle_seed) * 10.0f - 5.0f) * 0.5f;
    float jitter_y = (seeded_rand(&needle_seed) * 10.0f - 5.0f) * 0.5f;

    float a = angle + (seeded_rand(&needle_seed) * 40.0f - 20.0f) * 0.01f;
    float l = length * (0.8f + seeded_rand(&needle_seed) * 0.4f);

    int x0 = (int)(x + jitter_x - offset_x);
    int y0 = (int)(y + jitter_y);
    int x1 = (int)(x + jitter_x + cosf(a) * l - offset_x);
    int y1 = (int)(y + jitter_y + sinf(a) * l);
    draw_line_ext(bmp, x0, y0, x1, y1, MAT_FOLIAGE);
  }
}

/* Draws foliage tiers around spruce branches */
static void draw_spruce_tier(VirtualBitmap *bmp, float x, float y, float radius,
                             float droop, float offset_x) {
  int clusters = (int)(radius * 1.5f);
  uint32_t tier_seed = (uint32_t)x ^ (uint32_t)y;
  for (int i = 0; i < clusters; i++) {
    float angle_offset = seeded_rand(&tier_seed) * 6.28f;
    float dist = seeded_rand(&tier_seed) * radius;
    float px = x + cosf(angle_offset) * dist;
    float py = y + sinf(angle_offset) * dist * 0.3f + (dist * 0.5f);

    float needle_angle = 1.57f + (seeded_rand(&tier_seed) * 1.0f - 0.5f);
    draw_needle_cluster(bmp, px, py, 3.0f, needle_angle, 2.0f, offset_x);
  }
}

/* Plots spruce foliage along a branch node */
static void draw_branch_foliage(Node *n, VirtualBitmap *bmp, float offset_x) {
  if (!n->parent)
    return;
  float dx = n->x - n->parent->x;
  float dy = n->y - n->parent->y;
  float len = sqrtf(dx * dx + dy * dy);
  int steps = (int)len / 2;
  for (int i = 0; i <= steps; i++) {
    float t = (float)i / (steps + 1);
    float px = n->parent->x + dx * t;
    float py = n->parent->y + dy * t;
    float radius = (1.0f - t) * 6.0f + 2.0f;
    draw_spruce_tier(bmp, px, py, radius, 1.0f, offset_x);
  }
}

/* Generates procedural tree structure with branches */
static void generate_tree(Tree *t, float base_x, float base_y, int height,
                          uint32_t *seed) {
  t->root = create_node(base_x, base_y, NULL);
  Node *curr = t->root;
  int segments = 15 + (int)(seeded_rand(seed) * 10);
  float seg_len = (float)height / segments;
  for (int i = 0; i < segments; i++) {
    float progress = (float)i / segments;
    float sway = (seeded_rand(seed) - 0.5f) * 1.5f;
    Node *next = create_node(curr->x + sway, base_y - seg_len * (i + 1), curr);
    if (i > 3) {
      float branch_chance = 0.85f - progress * 0.6f;
      if (seeded_rand(seed) < branch_chance) {
        float branch_len = (1.0f - progress) * 30.0f + 6.0f;
        int sides = 2 + (int)(seeded_rand(seed) * 3);
        for (int s = 0; s < sides; s++) {
          float angle = (float)s * (6.28f / sides) + seeded_rand(seed);
          float bx = next->x + cosf(angle) * branch_len;
          float by = next->y + 10.0f + seeded_rand(seed) * 5.0f;
          Node *branch = create_node(bx, by, next);
          add_child(next, branch);
        }
      }
    }
    add_child(curr, next);
    curr = next;
  }
}

/* Recursive tree trunk and foliage plotter helper */
static void draw_node_recursive(Node *n, VirtualBitmap *b, float ox, int depth,
                                int total_segments) {
  int x1 = (int)(n->x - ox);
  int y1 = (int)n->y;

  if (n->parent) {
    int x0 = (int)(n->parent->x - ox);
    int y0 = (int)n->parent->y;

    float progress = (float)depth / total_segments;
    int thickness = (int)((1.0f - progress) * 3.0f);
    if (thickness < 1)
      thickness = 1;

    for (int dx = -thickness / 2; dx <= thickness / 2; dx++) {
      draw_line_ext(b, x0 + dx, y0, x1 + dx, y1, MAT_TRUNK);
    }

    if (fabsf(n->x - n->parent->x) > 1.0f) {
      draw_branch_foliage(n, b, ox);
    }
  } else {
    for (int dx = -5; dx <= 5; dx++) {
      int flare_h = 2 - (abs(dx) / 2);
      if (flare_h < 1)
        flare_h = 1;
      draw_line_ext(b, x1 + dx, y1, x1 + dx, y1 - flare_h, MAT_TRUNK);
    }
  }

  for (int i = 0; i < n->child_count; i++) {
    int is_main_trunk = (fabsf(n->children[i]->x - n->x) < 2.0f);
    draw_node_recursive(n->children[i], b, ox,
                        is_main_trunk ? depth + 1 : depth, total_segments);
  }
}

/* Recursive tree trunk and foliage plotter */
static void draw_pine_tree(Tree *t, VirtualBitmap *bmp, float offset_x) {
  int segments = 0;
  Node *temp = t->root;
  while (temp && temp->child_count > 0) {
    segments++;
    int found = 0;
    for (int i = 0; i < temp->child_count; i++) {
      if (fabsf(temp->children[i]->x - temp->x) < 2.0f) {
        temp = temp->children[i];
        found = 1;
        break;
      }
    }
    if (!found)
      break;
  }

  draw_node_recursive(t->root, bmp, offset_x, 0, segments > 0 ? segments : 15);
}

/* Recursive node tree disposal */
static void free_node_recursive(Node *n) {
  for (int i = 0; i < n->child_count; i++)
    free_node_recursive(n->children[i]);
  if (n->children)
    free(n->children);
  free(n);
}

/* Chunk initializer */
static void init_chunk(Chunk *c, float world_x, int layer, int height,
                       uint32_t master_seed) {
  c->world_x = world_x;
  c->active = 1;
  uint32_t chunk_seed =
      master_seed ^ ((uint32_t)world_x * 0x1F351) ^ ((uint32_t)layer * 0x8421);
  if (chunk_seed == 0)
    chunk_seed = 0xDEADC0DE;
  c->tree_count = 1 + (xorshift32(&chunk_seed) % 2);
  c->trees = (Tree *)malloc(sizeof(Tree) * c->tree_count);
  for (int i = 0; i < c->tree_count; i++) {
    float x = world_x + seeded_rand(&chunk_seed) * CHUNK_SIZE;
    float y = (float)height - 2;
    int h = height * 0.45f + seeded_rand(&chunk_seed) * (height * 0.35f);
    if (layer == 2)
      h *= 0.7f;
    generate_tree(&c->trees[i], x, y, h, &chunk_seed);
    c->trees[i].layer = layer;
  }
}

/* Chunk memory manager */
static void free_chunk(Chunk *c) {
  if (!c->active)
    return;
  for (int i = 0; i < c->tree_count; i++)
    free_node_recursive(c->trees[i].root);
  free(c->trees);
  c->active = 0;
}

/* Initializes global forest structure */
Forest *create_forest(int width, int height) {
  Forest *f = (Forest *)malloc(sizeof(Forest));
  f->width = width;
  f->height = height;
  f->camera_x = 0;
  f->chunk_count = CHUNK_POOL_SIZE;
  f->seed = (uint32_t)time(NULL);
  if (f->seed == 0)
    f->seed = 0xACE2026;
  f->chunks = (Chunk **)malloc(sizeof(Chunk *) * LAYER_COUNT);
  for (int l = 0; l < LAYER_COUNT; l++) {
    f->chunks[l] = (Chunk *)calloc(CHUNK_POOL_SIZE, sizeof(Chunk));
    for (int i = 0; i < CHUNK_POOL_SIZE; i++)
      init_chunk(&f->chunks[l][i], (float)(i * CHUNK_SIZE), l, height, f->seed);
  }
  f->particle_count = 200;
  f->particles = (Particle *)calloc(f->particle_count, sizeof(Particle));
  return f;
}

/* Generates and plots ground terrain with parallax layers */
void draw_ground(VirtualBitmap *bmp, int layer, float camera_x) {
  int h = bmp->height;
  int w = bmp->width;
  float ground_base_y = h - 8.0f;
  float parallax = (layer == 0) ? 1.0f : (layer == 1 ? 0.6f : 0.3f);
  float world_offset = camera_x * parallax;
  for (int x = 0; x < w; x++) {
    float wx = (float)x + world_offset;
    float terrain_noise =
        simplex_noise2(wx * 0.008f, (float)layer * 50.0f) * 3.0f;
    int gy = (int)(ground_base_y + terrain_noise);
    for (int y = gy; y < h; y++)
      draw_pixel_ext(bmp, x, y, MAT_GROUND);
    if (simplex_noise2(wx * 0.3f, (float)layer * 10.0f) > 0.85f)
      draw_pixel_ext(bmp, x, gy - 1, MAT_GROUND);
  }
}

/* Updates tree physics with wind and particle simulation */
static void update_node(Node *n, float dt, float time, int layer,
                        float wind_base) {
  if (n->parent) {
    float height_factor = (1.0f - (float)n->ry / 1000.0f);
    float wind =
        simplex_noise2(n->rx * 0.005f, time * 0.4f) * wind_base * height_factor;
    n->ax = wind * 15.0f;
    float dx = n->rx - n->x;
    n->ax += dx * SPRING_FORCE;
    n->vx += n->ax * dt;
    n->vx *= DAMPING;
    n->x += n->vx * dt;
  }
  for (int i = 0; i < n->child_count; i++)
    update_node(n->children[i], dt, time, layer, wind_base);
}

/* Orchestrates tree physics and particle simulation */
void update_forest(Forest *f, float dt, float time, VirtualBitmap **bmps) {
  float wind_base = simplex_noise2(time * 0.2f, 0) * WIND_STRENGTH;
  for (int l = 0; l < LAYER_COUNT; l++) {
    float parallax = (l == 0) ? 1.0f : (l == 1 ? 0.6f : 0.3f);
    float view_start = f->camera_x * parallax;
    for (int i = 0; i < CHUNK_POOL_SIZE; i++) {
      Chunk *c = &f->chunks[l][i];
      if (c->world_x + CHUNK_SIZE < view_start - CHUNK_SIZE) {
        float furthest_x = -1e9;
        for (int j = 0; j < CHUNK_POOL_SIZE; j++)
          if (f->chunks[l][j].world_x > furthest_x)
            furthest_x = f->chunks[l][j].world_x;
        free_chunk(c);
        init_chunk(c, furthest_x + CHUNK_SIZE, l, f->height, f->seed);
      }
      if (c->active)
        for (int t = 0; t < c->tree_count; t++)
          update_node(c->trees[t].root, dt, time, l, wind_base);
    }
  }
  for (int i = 0; i < f->particle_count; i++) {
    if (!f->particles[i].active) {
      if (rand() % 80 == 0) {
        f->particles[i].active = 1;
        f->particles[i].layer = rand() % LAYER_COUNT;
        float parallax = (f->particles[i].layer == 0)
                             ? 1.0f
                             : (f->particles[i].layer == 1 ? 0.6f : 0.3f);
        f->particles[i].x = f->camera_x * parallax + (float)(rand() % f->width);
        f->particles[i].y = -5.0f;
        f->particles[i].vx = (float)(rand() % 10 - 5);
        f->particles[i].vy = (float)(rand() % 3 + 3);
      }
      continue;
    }
    f->particles[i].vy += 4.0f * dt;
    f->particles[i].x += f->particles[i].vx * dt;
    f->particles[i].y += f->particles[i].vy * dt;
    float parallax = (f->particles[i].layer == 0)
                         ? 1.0f
                         : (f->particles[i].layer == 1 ? 0.6f : 0.3f);
    if (f->particles[i].y > f->height ||
        f->particles[i].x < f->camera_x * parallax - 50 ||
        f->particles[i].x > f->camera_x * parallax + f->width + 50)
      f->particles[i].active = 0;
  }
}

/* Renders all forest objects to their respective layers */
void draw_forest(Forest *f, VirtualBitmap **bmps, int layer_count) {
  for (int l = 0; l < layer_count; l++) {
    float parallax = (l == 0) ? 1.0f : (l == 1 ? 0.6f : 0.3f);
    float offset_x = f->camera_x * parallax;
    for (int i = 0; i < CHUNK_POOL_SIZE; i++) {
      Chunk *c = &f->chunks[l][i];
      if (c->active)
        for (int t = 0; t < c->tree_count; t++)
          draw_pine_tree(&c->trees[t], bmps[l], offset_x);
    }
    for (int i = 0; i < f->particle_count; i++) {
      if (f->particles[i].active && f->particles[i].layer == l) {
        int px = (int)(f->particles[i].x - offset_x);
        int py = (int)f->particles[i].y;
        draw_pixel_ext(bmps[l], px, py, MAT_PARTICLE);
      }
    }
  }
}
