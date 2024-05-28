#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/lodepng.h"

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

#define SCALE 2

int read_polygon(polygon *p) {
  int n;
  if (scanf("%x %d\n", &p->color, &n) == EOF) return 0;

  clear(&p->vertices);
  while (n--) {
    float x, y;
    scanf("%f %f\n", &x, &y);
    add_point(&p->vertices, x * SCALE, y * SCALE);
  }
  return 1;
}

#define SIZE 2000

void set_pixel(unsigned char *image, int x, int y, int color) {
  if (x < 0 || x >= SIZE || y < 0 || y >= SIZE) return;
  image[(y * SIZE + x) * 4] = color >> 16;
  image[(y * SIZE + x) * 4 + 1] = color >> 8;
  image[(y * SIZE + x) * 4 + 2] = color;
  image[(y * SIZE + x) * 4 + 3] = 0xff;
}

void rasterize(unsigned char *image, polygon *p) {
  edge_list remaining = {0}, active = {0};

  add(&remaining, p->vertices.tail, p->vertices.head);
  for (point *v = p->vertices.head; v != p->vertices.tail; v = v->next)
    add(&remaining, v, v->next);

  if (!remaining.head) return;

  quick_sort(&remaining, by_y_start);

  int y = ceilf(remaining.head->y_start) - 1;
  while (active.head || remaining.head) {
    int cur_winding = 0;
    float prev_x = 0;
    for (edge *e = active.head; e; e = e->next) {
      if (cur_winding) {
        for (int x = ceilf(prev_x); x < e->x; ++x) {
          set_pixel(image, x, y, p->color);
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
        append(&active, e);
      }
    }
    quick_sort(&active, by_x);
  }
}

int main() {
  unsigned char *image = calloc(1, SIZE * SIZE * 4);

  polygon p = {0};
  while (read_polygon(&p)) {
    rasterize(image, &p);
  }

  unsigned error = lodepng_encode32_file("out.png", image, SIZE, SIZE);
  if (error) printf("error %u: %s\n", error, lodepng_error_text(error));
  free(image);
  return 0;
}
