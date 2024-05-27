#include <stdlib.h>
#include <stdio.h>
#include "lodepng.h"

typedef struct edge
{
  float x1, y1, x2, y2;
  struct edge *next;
} edge;

typedef struct polygon
{
  int color;
  edge *edges;
} polygon;

typedef struct active_edge
{
  float k, max_y, cur_x;
  int reversed;
} active_edge;

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

  this._visibleEdges = edges
                           .filter(({y1, y2}) = > Math.ceil(y1) < y2) // ignore edges that don't cross any scan lines
                           .sort((e1, e2) = > e1.compare(e2));
}
return this._visibleEdges;

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
return;
}

int main()
{

  unsigned char *image = malloc(SIZE * SIZE * 4);
  unsigned x, y;
  for (y = 0; y < SIZE; y++)
  {
    for (x = 0; x < SIZE; x++)
    {
      int color = 255 * !(x & y) << 16 | (x ^ y) << 8 | (x | y);
      set_pixel(image, x, y, color);
    }
  }
  unsigned error = lodepng_encode32_file("out.png", image, SIZE, SIZE);
  if (error)
    printf("error %u: %s\n", error, lodepng_error_text(error));

  free(image);
  return 0;
}
