#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmd.h"

typedef struct mat3
{
  float v[3][3];
} mat3;

mat3 identity(void)
{
  mat3 p = {0};
  for (int i = 0; i < 3; ++i)
  {
    p.v[i][i] = 1.0f;
  }
  return p;
}

mat3 mult(mat3 m1, mat3 m2)
{
  mat3 p = {0};
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      for (int k = 0; k < 3; ++k)
      {
        p.v[i][j] += m1.v[i][k] * m2.v[k][j];
      }
    }
  }
  return p;
}

typedef struct transform
{
  mat3 matrix;
  struct transform *next;
} transform;

typedef struct transform_stack
{
  transform *head;
  transform *tail;
} transform_stack;

void push(transform_stack *st, mat3 m)
{
  transform *t = calloc(1, sizeof(transform));
  t->matrix = m;

  if (!st->head)
  {
    st->head = st->tail = t;
  }
  else
  {
    t->next = st->head;
    st->head = t;
  }
}

void pop(transform_stack *st)
{
  if (!st->head)
    return;

  transform *popped = st->head;
  if (st->head == st->tail)
  {
    st->head = st->tail = NULL;
  }
  else
  {
    st->head = st->head->next;
  }
  free(popped);
}

typedef struct vec3
{
  float v[3];
} vec3;

const vec3 origin = {.v = {0.0f, 0.0f, 1.0f}};

float x(vec3 *v)
{
  return v->v[0];
}

float y(vec3 *v)
{
  return v->v[1];
}

vec3 apply(mat3 m, vec3 x)
{
  vec3 y = {0};
  for (int i = 0; i < 3; ++i)
  {
    for (int j = 0; j < 3; ++j)
    {
      y.v[i] += m.v[i][j] * x.v[j];
    }
  }
  return y;
}

typedef struct vertex
{
  vec3 pos;
  struct vertex *next;
} vertex;

void free_list(vertex *l)
{
  if (l)
  {
    free_list(l->next);
    free(l);
  }
}

typedef struct path
{
  vertex *head;
  vertex *tail;
} path;

int size(path *l)
{
  int n = 0;
  for (vertex *p = l->head; p; p = p->next)
    ++n;
  return n;
}

typedef struct context
{
  int fill_color, stroke_color;
  float stroke_width;
  transform_stack transforms;
  path path;
  vec3 control;
} context;

void reset_path(context *ctx)
{
  free_list(ctx->path.head);
  vertex *start = calloc(1, sizeof(vertex));
  start->pos = origin;
  ctx->path.head = ctx->path.tail = start;
}

mat3 get_transform(context *ctx)
{
  mat3 m = identity();
  for (transform *t = ctx->transforms.head; t; t = t->next)
  {
    m = mult(t->matrix, m);
  }
  return m;
}

vec3 *get_current_point(context *ctx)
{
  if (!ctx->path.tail)
    exit(1);
  return &ctx->path.tail->pos;
}

void add_to_path(context *ctx, float x, float y)
{
  vertex *v = calloc(1, sizeof(vertex));
  v->pos = (vec3){.v = {x, y, 1.0f}};
  if (!ctx->path.head)
  {
    ctx->path.head = ctx->path.tail = v;
  }
  else
  {
    ctx->path.tail = ctx->path.tail->next = v;
  }
}

void set_tangent(context *ctx, float tx, float ty)
{
  vec3 *p = get_current_point(ctx);
  ctx->control.v[0] = x(p) + tx;
  ctx->control.v[1] = y(p) + ty;
}

float bezier(float t, float v0, float v1, float v2, float v3)
{
  float v01 = (1 - t) * v0 + t * v1;
  float v12 = (1 - t) * v1 + t * v2;
  float v23 = (1 - t) * v2 + t * v3;
  float v012 = (1 - t) * v01 + t * v12;
  float v123 = (1 - t) * v12 + t * v23;
  float v0123 = (1 - t) * v012 + t * v123;
  return v0123;
}

int bezier_sampling_rate = 10;

void approx_bezier(context *ctx, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
{
  for (int i = 0; i < bezier_sampling_rate; ++i)
  {
    float t = (float)(i + 1) / bezier_sampling_rate;
    add_to_path(ctx, bezier(t, x0, x1, x2, x3), bezier(t, y0, y1, y2, y3));
  }
}

void emit_polygon(context *ctx)
{
  int n = size(&ctx->path);
  if (n < 2)
    return;

  printf("%#x %d\n", ctx->fill_color, n);

  mat3 m = get_transform(ctx);
  for (vertex *v = ctx->path.head; v; v = v->next)
  {
    vec3 p = apply(m, v->pos);
    printf("%f %f\n", p.v[0] / p.v[2], p.v[1] / p.v[2]);
  }

  // TODO: stroke
}

int read_and_exec(context *ctx)
{
  cmd_type type;
  if (scanf("%d%*[^\n]\n", &type) == EOF)
    return 0;

  vec3 *p = get_current_point(ctx);
  vec3 *cp = &ctx->control;

  switch (type)
  {
  case stroke_width:
    scanf("%f\n", &ctx->stroke_width);
    break;
  case stroke_color:
    scanf("%x\n", &ctx->stroke_color);
    break;
  case fill_color:
    scanf("%x\n", &ctx->fill_color);
    break;
  case push_matrix:
  {
    mat3 m;
    scanf("%f %f %f %f %f %f\n",
          &m.v[0][0], &m.v[1][0],
          &m.v[0][1], &m.v[1][1],
          &m.v[0][2], &m.v[1][2]);
    m.v[2][2] = 1.0f;
    push(&ctx->transforms, m);
    break;
  }
  case pop_matrix:
    pop(&ctx->transforms);
    break;
  case move_to:
  {
    if (ctx->path.head != ctx->path.tail)
      exit(1);
    scanf("%f %f\n", &p->v[0], &p->v[1]);
    set_tangent(ctx, 0, 0);
    break;
  }
  case move_to_d:
  {
    if (ctx->path.head != ctx->path.tail)
      exit(1);
    float dx, dy;
    scanf("%f %f\n", &dx, &dy);
    p->v[0] += dx;
    p->v[1] += dy;
    set_tangent(ctx, 0, 0);
    break;
  }
  case line_to:
  {
    float x, y;
    scanf("%f %f\n", &x, &y);
    add_to_path(ctx, x, y);
    set_tangent(ctx, 0, 0);
    break;
  }
  case line_to_d:
  {
    float dx, dy;
    scanf("%f %f\n", &dx, &dy);
    add_to_path(ctx, x(p) + dx, y(p) + dy);
    set_tangent(ctx, 0, 0);
    break;
  }
  case v_line_to:
  {
    float y;
    scanf("%f\n", &y);
    add_to_path(ctx, x(p), y);
    set_tangent(ctx, 0, 0);
    break;
  }
  case v_line_to_d:
  {
    float dy;
    scanf("%f\n", &dy);
    add_to_path(ctx, x(p), y(p) + dy);
    set_tangent(ctx, 0, 0);
    break;
  }
  case h_line_to:
  {
    float x;
    scanf("%f\n", &x);
    add_to_path(ctx, x, y(p));
    set_tangent(ctx, 0, 0);
    break;
  }
  case h_line_to_d:
  {
    float dx;
    scanf("%f\n", &dx);
    add_to_path(ctx, x(p) + dx, y(p));
    set_tangent(ctx, 0, 0);
    break;
  }
  case curve_to:
  {
    float x1, y1, x2, y2, x3, y3;
    scanf("%f %f %f %f %f %f\n", &x1, &y1, &x2, &y2, &x3, &y3);
    approx_bezier(ctx, x(p), y(p), x1, y1, x2, y2, x3, y3);
    set_tangent(ctx, x3 - x2, y3 - y2);
    break;
  }
  case curve_to_d:
  {
    float dx1, dy1, dx2, dy2, dx3, dy3;
    scanf("%f %f %f %f %f %f\n", &dx1, &dy1, &dx2, &dy2, &dx3, &dy3);
    approx_bezier(ctx, x(p), y(p), x(p) + dx1, y(p) + dy1,
                  x(p) + dx2, y(p) + dy2, x(p) + dx3, y(p) + dy3);
    set_tangent(ctx, dx3 - dx2, dy3 - dy2);
    break;
  }
  case s_curve_to:
  {
    float x2, y2, x3, y3;
    scanf("%f %f %f %f\n", &x2, &y2, &x3, &y3);
    approx_bezier(ctx, x(p), y(p), x(cp), y(cp), x2, y2, x3, y3);
    set_tangent(ctx, x3 - x2, y3 - y2);
    break;
  }
  case s_curve_to_d:
  {
    float dx2, dy2, dx3, dy3;
    scanf("%f %f %f %f\n", &dx2, &dy2, &dx3, &dy3);
    approx_bezier(ctx, x(p), y(p), x(cp), y(cp),
                  x(p) + dx2, y(p) + dy2, x(p) + dx3, y(p) + dy3);
    set_tangent(ctx, dx3 - dx2, dy3 - dy2);
    break;
  }
  case close_path:
    emit_polygon(ctx);
    reset_path(ctx);
    break;
  }

  return 1;
}

int main(int argc, char *argv[])
{
  if (argc >= 2)
    sscanf(argv[1], "%d", &bezier_sampling_rate);

  context ctx = {0};
  reset_path(&ctx);

  while (read_and_exec(&ctx))
    ;
  return 0;
}