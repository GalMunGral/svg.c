#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmd.h"

typedef struct point {
  float x, y;
} point;

typedef struct vec3 {
  float v[3];
} vec3;

vec3 to_vec(point p) { return (vec3){.v = {p.x, p.y, 1.0f}}; }

point to_point(vec3 v) {
  return (point){.x = v.v[0] / v.v[2], .y = v.v[1] / v.v[2]};
}

typedef struct mat3 {
  float v[3][3];
} mat3;

mat3 identity(void) {
  mat3 p = {0};
  for (int i = 0; i < 3; ++i) {
    p.v[i][i] = 1.0f;
  }
  return p;
}

mat3 mult(mat3 m1, mat3 m2) {
  mat3 p = {0};
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      for (int k = 0; k < 3; ++k) {
        p.v[i][j] += m1.v[i][k] * m2.v[k][j];
      }
    }
  }
  return p;
}

vec3 apply(mat3 m, vec3 x) {
  vec3 y = {0};
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      y.v[i] += m.v[i][j] * x.v[j];
    }
  }
  return y;
}

typedef struct style {
  int fill_color;
  int stroke_color;
  float stroke_width;
  struct style *parent;
} style;

typedef struct transform {
  mat3 matrix;
  struct transform *parent;
} transform;

typedef struct vertex {
  point pos;
  struct vertex *next;
} vertex;

void free_list(vertex *l) {
  if (l) {
    free_list(l->next);
    free(l);
  }
}

typedef struct path {
  vertex *head;
  vertex *tail;
} path;

void clear(path *p) {
  free_list(p->head);
  p->head = p->tail = NULL;
}

int size(path *l) {
  int n = 0;
  for (vertex *p = l->head; p; p = p->next) ++n;
  return n;
}

typedef struct context {
  style *style;
  transform *transforms;
  path path;
  point control;
} context;

void save_style(context *ctx) {
  style *s = calloc(1, sizeof(style));
  if (!ctx->style) exit(1);
  memcpy(s, ctx->style, sizeof(style));
  s->parent = ctx->style;
  ctx->style = s;
}

void restore_style(context *ctx) {
  style *s = ctx->style;
  if (!s || !s->parent) exit(1);
  ctx->style = s->parent;
  free(s);
}

mat3 get_transform(context *ctx) {
  mat3 m = identity();
  for (transform *t = ctx->transforms; t; t = t->parent) {
    m = mult(t->matrix, m);
  }
  return m;
}

void push_transform(context *ctx, mat3 m) {
  transform *t = calloc(1, sizeof(transform));
  t->matrix = m;
  t->parent = ctx->transforms;
  ctx->transforms = t;
}

void pop_transform(context *ctx) {
  transform *t = ctx->transforms;
  if (!t) exit(1);
  ctx->transforms = t->parent;
  free(t);
}

point *start_point(context *ctx) {
  if (!ctx->path.head) exit(1);
  return &ctx->path.head->pos;
}

point *current_point(context *ctx) {
  if (!ctx->path.tail) exit(1);
  return &ctx->path.tail->pos;
}

void set_tangent(context *ctx, float tx, float ty) {
  point *p = current_point(ctx);
  ctx->control.x = p->x + tx;
  ctx->control.y = p->y + ty;
}

void add_to_path(context *ctx, float x, float y) {
  vertex *v = calloc(1, sizeof(vertex));
  v->pos.x = x;
  v->pos.y = y;
  if (!ctx->path.head) {
    ctx->path.head = ctx->path.tail = v;
  } else {
    ctx->path.tail = ctx->path.tail->next = v;
  }
}

void reset_path(context *ctx) {
  clear(&ctx->path);
  add_to_path(ctx, 0, 0);
  ctx->control = *current_point(ctx);
}

float bezier(float t, float v0, float v1, float v2, float v3) {
  float v01 = (1 - t) * v0 + t * v1;
  float v12 = (1 - t) * v1 + t * v2;
  float v23 = (1 - t) * v2 + t * v3;
  float v012 = (1 - t) * v01 + t * v12;
  float v123 = (1 - t) * v12 + t * v23;
  float v0123 = (1 - t) * v012 + t * v123;
  return v0123;
}

int bezier_sampling_rate = 10;

void approx_bezier(context *ctx, float x0, float y0, float x1, float y1,
                   float x2, float y2, float x3, float y3) {
  for (int i = 0; i < bezier_sampling_rate; ++i) {
    float t = (float)(i + 1) / bezier_sampling_rate;
    add_to_path(ctx, bezier(t, x0, x1, x2, x3), bezier(t, y0, y1, y2, y3));
  }
}

void emit_line_segment(context *ctx, point a, point b) {
  float r = ctx->style->stroke_width / 2;
  float vx = b.x - a.x, vy = b.y - a.y;
  float l = sqrtf(vx * vx + vy * vy);
  if (l == 0) return;
  float dx = -r * (vy / l), dy = r * (vx / l);
  printf("%#x %d\n", ctx->style->stroke_color, 4);
  printf("%f %f\n", a.x + dx, a.y + dy);
  printf("%f %f\n", a.x - dx, a.y - dy);
  printf("%f %f\n", b.x - dx, b.y - dy);
  printf("%f %f\n", b.x + dx, b.y + dy);
}

int circle_sampling_rate = 10;

void emit_line_joint(context *ctx, point p) {
  int n = circle_sampling_rate;
  float r = ctx->style->stroke_width / 2;

  printf("%#x %d\n", ctx->style->stroke_color, n);
  for (int i = 0; i < n; ++i) {
    float theta = (2 * M_PI / n) * i;
    printf("%f %f\n", p.x + r * cosf(theta), p.y + r * sinf(theta));
  }
}

void apply_transform(context *ctx) {
  mat3 m = get_transform(ctx);
  for (vertex *v = ctx->path.head; v; v = v->next) {
    v->pos = to_point(apply(m, to_vec(v->pos)));
  }
}

void fill_path(context *ctx) {
  if (!ctx->style) exit(1);
  if (ctx->style->fill_color == -1) return;

  int n = size(&ctx->path);
  if (n >= 2) {
    printf("%#x %d\n", ctx->style->fill_color, n);
    for (vertex *v = ctx->path.head; v; v = v->next) {
      printf("%f %f\n", v->pos.x, v->pos.y);
    }
  }
}

void stroke_path(context *ctx) {
  if (!ctx->style) exit(1);
  if (ctx->style->stroke_color == -1) return;
  if (ctx->style->stroke_width <= 0) return;
  if (size(&ctx->path) < 2) return;

  for (vertex *v = ctx->path.head; v != ctx->path.tail; v = v->next) {
    emit_line_segment(ctx, v->pos, v->next->pos);
  }
  for (vertex *v = ctx->path.head->next; v != ctx->path.tail; v = v->next) {
    emit_line_joint(ctx, v->pos);
  }
}

int exec_next_command(context *ctx) {
  cmd_type type;
  if (scanf("%d%*[^\n]\n", &type) == EOF) return 0;

  switch (type) {
    case save:
      save_style(ctx);
      break;
    case restore:
      restore_style(ctx);
      break;
    case stroke_width:
      scanf("%f\n", &ctx->style->stroke_width);
      break;
    case stroke_color:
      scanf("%x\n", &ctx->style->stroke_color);
      break;
    case fill_color:
      scanf("%x\n", &ctx->style->fill_color);
      break;
    case push_matrix: {
      mat3 m = {0};
      scanf("%f %f %f %f %f %f\n", &m.v[0][0], &m.v[1][0], &m.v[0][1],
            &m.v[1][1], &m.v[0][2], &m.v[1][2]);
      m.v[2][2] = 1.0f;
      push_transform(ctx, m);
      break;
    }
    case pop_matrix:
      pop_transform(ctx);
      break;
    case begin_path:
      reset_path(ctx);
      break;
    case move_to: {
      float x, y;
      scanf("%f %f\n", &x, &y);

      point *p = current_point(ctx);
      p->x = x;
      p->y = y;
      set_tangent(ctx, 0, 0);
      break;
    }
    case move_to_d: {
      float dx, dy;
      scanf("%f %f\n", &dx, &dy);

      point *p = current_point(ctx);
      p->x += dx;
      p->y += dy;
      set_tangent(ctx, 0, 0);
      break;
    }
    case line_to: {
      float x, y;
      scanf("%f %f\n", &x, &y);

      add_to_path(ctx, x, y);
      set_tangent(ctx, 0, 0);
      break;
    }
    case line_to_d: {
      float dx, dy;
      scanf("%f %f\n", &dx, &dy);

      point p = *current_point(ctx);
      add_to_path(ctx, p.x + dx, p.y + dy);
      set_tangent(ctx, 0, 0);
      break;
    }
    case v_line_to: {
      float y;
      scanf("%f\n", &y);

      point p = *current_point(ctx);
      add_to_path(ctx, p.x, y);
      set_tangent(ctx, 0, 0);
      break;
    }
    case v_line_to_d: {
      float dy;
      scanf("%f\n", &dy);

      point p = *current_point(ctx);
      add_to_path(ctx, p.x, p.y + dy);
      set_tangent(ctx, 0, 0);
      break;
    }
    case h_line_to: {
      float x;
      scanf("%f\n", &x);

      point p = *current_point(ctx);
      add_to_path(ctx, x, p.y);
      set_tangent(ctx, 0, 0);
      break;
    }
    case h_line_to_d: {
      float dx;
      scanf("%f\n", &dx);

      point p = *current_point(ctx);
      add_to_path(ctx, p.x + dx, p.y);
      set_tangent(ctx, 0, 0);
      break;
    }
    case curve_to: {
      float x1, y1, x2, y2, x3, y3;
      scanf("%f %f %f %f %f %f\n", &x1, &y1, &x2, &y2, &x3, &y3);

      point p = *current_point(ctx);
      approx_bezier(ctx, p.x, p.y, x1, y1, x2, y2, x3, y3);
      set_tangent(ctx, x3 - x2, y3 - y2);
      break;
    }
    case curve_to_d: {
      float dx1, dy1, dx2, dy2, dx3, dy3;
      scanf("%f %f %f %f %f %f\n", &dx1, &dy1, &dx2, &dy2, &dx3, &dy3);

      point p = *current_point(ctx);
      approx_bezier(ctx, p.x, p.y, p.x + dx1, p.y + dy1, p.x + dx2, p.y + dy2,
                    p.x + dx3, p.y + dy3);
      set_tangent(ctx, dx3 - dx2, dy3 - dy2);
      break;
    }
    case s_curve_to: {
      float x2, y2, x3, y3;
      scanf("%f %f %f %f\n", &x2, &y2, &x3, &y3);

      point p = *current_point(ctx);
      point cp = ctx->control;
      approx_bezier(ctx, p.x, p.y, cp.x, cp.y, x2, y2, x3, y3);
      set_tangent(ctx, x3 - x2, y3 - y2);
      break;
    }
    case s_curve_to_d: {
      float dx2, dy2, dx3, dy3;
      scanf("%f %f %f %f\n", &dx2, &dy2, &dx3, &dy3);

      point p = *current_point(ctx);
      point cp = ctx->control;
      approx_bezier(ctx, p.x, p.y, cp.x, cp.y, p.x + dx2, p.y + dy2, p.x + dx3,
                    p.y + dy3);
      set_tangent(ctx, dx3 - dx2, dy3 - dy2);
      break;
    }
    case close_path: {
      point p = *start_point(ctx);
      add_to_path(ctx, p.x, p.y);
      break;
    }
    case fill_and_stroke:
      apply_transform(ctx);
      fill_path(ctx);
      stroke_path(ctx);
      break;
  }

  return 1;
}

int main(int argc, char *argv[]) {
  if (argc >= 2) sscanf(argv[1], "%d", &bezier_sampling_rate);

  context ctx = {0};
  ctx.style = calloc(1, sizeof(style));
  ctx.style->fill_color = 0;
  ctx.style->stroke_color = -1;
  ctx.style->stroke_width = 1;

  while (exec_next_command(&ctx));
  return 0;
}