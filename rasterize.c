#include <stdlib.h>
#include <stdio.h>
#include "lodepng.h"

typedef struct edge
{
  float x1, y1, x2, y2;
  float k, x;
  struct edge *next;
} edge;

typedef struct edge_list
{
  edge *head;
  edge *tail;
} edge_list;

void dispose(edge *l)
{
  if (l)
  {
    dispose(l->next);
    free(l);
  }
}

typedef struct polygon
{
  int color;
  edge_list edges;
} polygon;

int read_polygon(polygon *p)
{
  int n;
  if (scanf("%x %d\n", &p->color, &n) == EOF)
    return 0;

  edge_list *l = &p->edges;

  while (n--)
  {
    float x, y;
    scanf("%f %f\n", &x, &y);

    if (l->tail)
    {
      l->tail->x2 = x;
      l->tail->y2 = y;
    }

    edge *e = calloc(1, sizeof(edge));
    e->x1 = x;
    e->y1 = y;
    if (!l->head)
      l->head = l->tail = e;
    else
      l->tail = l->tail->next = e;
  }

  l->tail->x2 = l->head->x1;
  l->tail->y2 = l->head->y1;
  return 1;
}

#define SIZE 512

void set_pixel(unsigned char *image, int x, int y, int color)
{
  image[(y * SIZE + x) * 4] = color >> 16;
  image[(y * SIZE + x) * 4 + 1] = color >> 8;
  image[(y * SIZE + x) * 4 + 2] = color;
  image[(y * SIZE + x) * 4 + 3] = 0xff;
}

void rasterize(unsigned char *image, polygon *p)
{
  //   if (!this.visibleEdges.length) return;
  //   let y = Math.ceil(this.visibleEdges[0].y1) - 1;
  //   let active: Array<ActiveEdge> = [];
  //   let i = 0;
  //   do {
  //     if (active.length & 1) {
  //       throw "Odd number of intersections. The path is not closed!";
  //     }
  //     for (let i = 0, winding = 0; i < active.length; ++i) {
  //       if (winding) {
  //         for (let x = Math.ceil(active[i - 1].x); x < active[i].x; ++x) {
  //           fn(x, y);
  //         }
  //       }
  //       winding += active[i].reversed ? -1 : 1;
  //     }
  //     ++y;
  //     active = active.filter((e) => e.maxY > y);
  //     for (const edge of active) {
  //       edge.x += edge.k;
  //     }
  //     while (i < this.visibleEdges.length && this.visibleEdges[i].y1 <= y) {
  //       const { x1, y1, x2, y2, reversed } = this.visibleEdges[i++];
  //       const k = (x2 - x1) / (y2 - y1);
  //       active.push(new ActiveEdge(y2, x1 + k * (y - y1), k, reversed));
  //     }
  //     active.sort((e1, e2) => e1.x - e2.x);
  //   } while (active.length || i < this.visibleEdges.length);
  // }
}

int main()
{

  unsigned char *image = calloc(1, SIZE * SIZE * 4);

  polygon p = {0};
  while (read_polygon(&p))
  {
    rasterize(image, &p);
    dispose(p.edges.head);
    p = (polygon){0};
  }

  unsigned error = lodepng_encode32_file("out.png", image, SIZE, SIZE);
  if (error)
    printf("error %u: %s\n", error, lodepng_error_text(error));
  free(image);
  return 0;
}
