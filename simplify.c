#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "cmd.h"

typedef struct vec2
{
  float x, y;
} vec2;

typedef struct vertex
{
  float x, y;
  struct vertex *next;
} vertex;

void dispose(vertex *l)
{
  if (l)
  {
    dispose(l->next);
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
  path path;
  vec2 control;
} context;

void init(context *ctx)
{
  *ctx = (context){0};
  ctx->path.head = ctx->path.tail = calloc(1, sizeof(vertex));
}

void reset(context *ctx)
{
  dispose(ctx->path.head);
  init(ctx);
}

vertex *get_current_point(context *ctx)
{
  if (!ctx->path.tail)
    exit(1);
  return ctx->path.tail;
}

void add_to_path(context *ctx, float x, float y)
{
  vertex *v = calloc(1, sizeof(vertex));
  v->x = x;
  v->y = y;
  if (!ctx->path.head)
    ctx->path.head = ctx->path.tail = v;
  else
    ctx->path.tail = ctx->path.tail->next = v;
}

void set_tangent(context *ctx, float tx, float ty)
{
  vertex *p = get_current_point(ctx);
  ctx->control.x = p->x + tx;
  ctx->control.y = p->y + ty;
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

#define SAMPLING_RATE 10

void approx_bezier(context *ctx, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
{
  for (int i = 0; i < SAMPLING_RATE; ++i)
  {
    float t = (float)(i + 1) / SAMPLING_RATE;
    add_to_path(ctx, bezier(t, x0, x1, x2, x3), bezier(t, y0, y1, y2, y3));
  }
}

void emit_polygon(context *ctx)
{
  int n = size(&ctx->path);
  if (n < 2)
    return;

  printf("%#x %d\n", ctx->fill_color, n);
  for (vertex *v = ctx->path.head; v; v = v->next)
  {
    printf("%f %f\n", v->x, v->y);
  }
  // TODO: stroke
}

int read_and_exec(context *ctx)
{
  cmd_type type;
  if (scanf("%d%*[^\n]\n", &type) == EOF)
    return 0;

  vertex *p = get_current_point(ctx);
  vec2 cp = ctx->control;

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
  case move_to:
  {
    if (ctx->path.head != ctx->path.tail)
      exit(1);
    float x, y;
    scanf("%f %f\n", &x, &y);
    p->x = x;
    p->y = y;
    set_tangent(ctx, 0, 0);
    break;
  }
  case move_to_d:
  {
    if (ctx->path.head != ctx->path.tail)
      exit(1);
    float dx, dy;
    scanf("%f %f\n", &dx, &dy);
    p->x += dx;
    p->y += dy;
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
    float x = p->x + dx, y = p->y + dy;
    add_to_path(ctx, x, y);
    set_tangent(ctx, 0, 0);
    break;
  }
  case v_line_to:
  {
    float y;
    scanf("%f\n", &y);
    add_to_path(ctx, p->x, y);
    set_tangent(ctx, 0, 0);
    break;
  }
  case v_line_to_d:
  {
    float dy;
    scanf("%f\n", &dy);
    add_to_path(ctx, p->x, p->y + dy);
    set_tangent(ctx, 0, 0);
    break;
  }
  case h_line_to:
  {
    float x;
    scanf("%f\n", &x);
    add_to_path(ctx, x, p->y);
    set_tangent(ctx, 0, 0);
    break;
  }
  case h_line_to_d:
  {
    float dx;
    scanf("%f\n", &dx);
    add_to_path(ctx, p->x + dx, p->y);
    set_tangent(ctx, 0, 0);
    break;
  }
  case curve_to:
  {
    float x1, y1, x2, y2, x, y;
    scanf("%f %f %f %f %f %f\n", &x1, &y1, &x2, &y2, &x, &y);
    approx_bezier(ctx, p->x, p->y, x1, y1, x2, y2, x, y);
    set_tangent(ctx, x - x2, y - y2);
    break;
  }
  case curve_to_d:
  {
    float dx1, dy1, dx2, dy2, dx, dy;
    scanf("%f %f %f %f %f %f\n", &dx1, &dy1, &dx2, &dy2, &dx, &dy);
    approx_bezier(ctx, p->x, p->y, p->x + dx1, p->y + dy1, p->x + dx2, p->y + dy2, p->x + dx, p->y + dy);
    set_tangent(ctx, dx - dx2, dy - dy2);
    break;
  }
  case s_curve_to:
  {
    float x2, y2, x, y;
    scanf("%f %f %f %f\n", &x2, &y2, &x, &y);
    approx_bezier(ctx, p->x, p->y, cp.x, cp.y, x2, y2, x, y);
    set_tangent(ctx, x - x2, y - y2);
    break;
  }
  case s_curve_to_d:
  {
    float dx2, dy2, dx, dy;
    scanf("%f %f %f %f\n", &dx2, &dy2, &dx, &dy);
    approx_bezier(ctx, p->x, p->y, cp.x, cp.y, p->x + dx2, p->y + dy2, p->x + dx, p->y + dy);
    set_tangent(ctx, dx - dx2, dy - dy2);
    break;
  }
  case close_path:
    emit_polygon(ctx);
    reset(ctx);
    break;
  }

  return 1;
}

int main()
{
  context ctx;
  init(&ctx);
  while (read_and_exec(&ctx))
    ;
  return 0;
}