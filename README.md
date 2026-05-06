# SVG Rasterizer

- `compile` translates SVG into a sequence of canvas-style drawing commands
- `interpret` evaluates those commands into polygons via Bézier subdivision
- `rasterize` converts polygons into pixels using the scanline algorithm

```bash
make
make run
```

Intermediate outputs are written to `test/compile.out` and `test/interpret.out`.

## Rhetorical Design

### Purpose

Vector graphics is the rendering model underlying most GUI toolkits, font renderers, and document formats. Programmers who work with these systems daily are rarely exposed to the mechanism that converts geometric descriptions into a pixel grid. This project implements an SVG rasterizer from scratch to demonstrate that this conversion — one of the foundational operations of computer graphics — requires no specialized machinery beyond a straightforward algorithm.

### Strategy

The pipeline is decomposed into three Unix filters — `compile`, `interpret`, and `rasterize` — one per conceptual stage. Because each filter reads from stdin and writes to stdout, the output of any stage can be examined in isolation. Each stage is self-contained and can be understood independently of the others. The Ghostscript tiger, a reference SVG widely used across rendering projects, serves as the illustrative example.