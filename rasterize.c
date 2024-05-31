#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/lodepng.h"

float min(float a, float b) { return a < b ? a : b; }
float max(float a, float b) { return a < b ? b : a; }

float overlap(float start1, float end1, float start2, float end2) {
  return max(0.0f, min(end1, end2) - max(start1, start2));
}

typedef struct point {
  float x, y;
  struct point *next;
} point;

void free_list(point *l) {
  if (l) {
    free_list(l->next);
    free(l);
  }
}

typedef struct point_list {
  point *head;
  point *tail;
} point_list;

void clear(point_list *l) {
  free_list(l->head);
  l->head = l->tail = NULL;
}

void add_point(point_list *l, float x, float y) {
  point *p = calloc(1, sizeof(point));
  p->x = x;
  p->y = y;

  if (!l->head)
    l->head = l->tail = p;
  else
    l->tail = l->tail->next = p;
}

typedef struct edge {
  float y_start, y_end, x, k;
  int winding;
  struct edge *next;
} edge;

typedef struct edge_list {
  edge *head;
  edge *tail;
} edge_list;

void append(edge_list *l, edge *e) {
  if (!l->head)
    l->head = l->tail = e;
  else
    l->tail = l->tail->next = e;
}

void add(edge_list *l, point *a, point *b) {
  edge *e = calloc(1, sizeof(edge));
  if (a->y > b->y) {
    e->winding = 1;
    point *tmp = a;
    a = b;
    b = tmp;
  } else {
    e->winding = -1;
  }
  e->y_start = a->y;
  e->y_end = b->y;
  e->x = a->x;
  e->k = (b->x - a->x) / (b->y - a->y);
  append(l, e);
}

edge *pop_head(edge_list *l) {
  if (!l->head) return NULL;

  edge *popped = l->head;

  if (l->head == l->tail)
    l->head = l->tail = NULL;
  else
    l->head = l->head->next;

  popped->next = NULL;
  return popped;
}

void quick_sort(edge_list *l, float (*key_func)(edge *)) {
  if (!l->head) return;

  edge_list left = {0}, right = {0};
  edge *pivot = l->head;

  edge *e = pivot->next;
  pivot->next = NULL;
  while (e) {
    edge *next = e->next;
    e->next = NULL;
    if (key_func(e) < key_func(pivot))
      append(&left, e);
    else
      append(&right, e);
    e = next;
  }

  quick_sort(&left, key_func);
  quick_sort(&right, key_func);

  if (left.head) {
    l->head = left.head;
    left.tail->next = pivot;
  } else {
    l->head = pivot;
  }

  if (right.tail) {
    l->tail = right.tail;
    pivot->next = right.head;
  } else {
    l->tail = pivot;
  }
}

float by_x(edge *e) { return e->x; }

float by_y_start(edge *e) { return e->y_start; }

typedef struct polygon {
  int color;
  point_list vertices;
} polygon;

int size = 900;
int scale = 1;
int aa = 1;
int debug = 0;

#define C(image, w, x, y, c) (image)[((y) * (w) + (x)) * 4 + (c)]

int read_polygon(polygon *p) {
  int n;
  if (scanf("%x %d\n", &p->color, &n) == EOF) return 0;

  clear(&p->vertices);
  while (n--) {
    float x, y;
    scanf("%f %f\n", &x, &y);
    if (debug) {
      add_point(&p->vertices, x * scale, y * scale);
    } else {
      add_point(&p->vertices, x * scale, y * scale * aa);
    }
  }
  return 1;
}

void put_pixel(float *I, int w, int h, int x, int y, float r1, float g1,
               float b1, float a1) {
  if (x < 0 || x >= w || y < 0 || y >= h) return;

  float r2 = C(I, w, x, y, 0);
  float g2 = C(I, w, x, y, 1);
  float b2 = C(I, w, x, y, 2);
  float a2 = C(I, w, x, y, 3);

  float a = a1 + a2 * (1 - a1);
  if (a == 0) return;

  float w1 = a1 / a, w2 = a2 * (1 - a1) / a;
  C(I, w, x, y, 0) = r1 * w1 + r2 * w2;
  C(I, w, x, y, 1) = g1 * w1 + g2 * w2;
  C(I, w, x, y, 2) = b1 * w1 + b2 * w2;
  C(I, w, x, y, 3) = a;
}

void rasterize(float *image, polygon *p) {
  int w = size * scale, h = size * scale * aa;

  edge_list remaining = {0}, active = {0};

  int mask = (1 << 8) - 1;
  float r = (p->color >> 16 & mask) / 255.0f;
  float g = (p->color >> 8 & mask) / 255.0f;
  float b = (p->color & mask) / 255.0f;

  add(&remaining, p->vertices.tail, p->vertices.head);
  for (point *v = p->vertices.head; v != p->vertices.tail; v = v->next) {
    add(&remaining, v, v->next);
  }

  if (!remaining.head) return;

  quick_sort(&remaining, by_y_start);

  int y = ceilf(remaining.head->y_start) - 1;
  while (active.head || remaining.head) {
    int cur_winding = 0;
    float prev_x = 0;
    for (edge *e = active.head; e; e = e->next) {
      if (cur_winding) {
        int start = ceilf(prev_x - 0.5);
        int end = ceilf(e->x + 0.5);
        for (int x = start; x < end; ++x) {
          float a = overlap(x - 0.5, x + 0.5, prev_x, e->x);
          put_pixel(image, w, h, x, y, r, g, b, a);
        }
      }
      cur_winding += e->winding;
      prev_x = e->x;
    }

    ++y;

    edge *e = active.head;
    active.head = active.tail = NULL;
    while (e) {
      edge *next = e->next;
      e->next = NULL;
      if (e->y_end <= y) {
        free(e);
      } else {
        e->x += e->k;
        append(&active, e);
      }
      e = next;
    }
    while (remaining.head && remaining.head->y_start <= y) {
      edge *e = pop_head(&remaining);
      if (e->y_end <= y) {
        free(e);
      } else {
        e->x += e->k * (y - e->y_start);
        append(&active, e);
      }
    }
    quick_sort(&active, by_x);
  }
}

void plot_vertices(unsigned char *image, polygon *p) {
  int w = size * scale;
  for (point *v = p->vertices.head; v; v = v->next) {
    int x = v->x, y = v->y;
    C(image, w, x, y, 0) = p->color >> 16;
    C(image, w, x, y, 1) = p->color >> 8;
    C(image, w, x, y, 2) = p->color;
    C(image, w, x, y, 3) = 0xff;
  }
}

int main(int argc, char *argv[]) {
  if (argc >= 2) sscanf(argv[1], "%d", &scale);
  if (argc >= 3) sscanf(argv[2], "%d", &aa);
  if (argc >= 4) sscanf(argv[3], "%d", &debug);

  int w = size * scale, h = size * scale;
  polygon p = {0};

  unsigned char *image = calloc(1, h * w * 4);
  const char *filename = debug ? "debug.png" : "out.png";

  if (debug) {
    while (read_polygon(&p)) {
      plot_vertices(image, &p);
    }
  } else {
    float *f_image = calloc(1, h * aa * w * 4 * sizeof(float));
    while (read_polygon(&p)) {
      rasterize(f_image, &p);
    }
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        for (int c = 0; c < 4; ++c) {
          float p = 0.0f;
          for (int k = 0; k < aa; ++k) {
            p += C(f_image, w, x, y * aa + k, c) / aa;
          }
          C(image, w, x, y, c) = p * 255.0f;
        }
      }
    }
    free(f_image);
  }

  unsigned error = lodepng_encode32_file(filename, image, w, h);
  if (error) printf("error %u: %s\n", error, lodepng_error_text(error));
  free(image);

  return 0;
}
