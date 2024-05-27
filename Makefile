all: parse simplify render

parse: compile.c
	gcc compile.c -o compile

simplify: simplify.c
	gcc simplify.c -o simplify

lodepng.o: lodepng.c
	gcc lodepng.c -c

rasterize: rasterize.c lodepng.o
	gcc rasterize.c lodepng.o -o rasterize
