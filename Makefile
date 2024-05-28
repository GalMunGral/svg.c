run: all
	./compile < tiger.svg | ./interpret 100 | ./rasterize

all: compile interpret rasterize 

lodepng.o: lib/lodepng.c
	gcc lib/lodepng.c -c -o $@

compile: compile.c
	gcc $^ -o $@

interpret: interpret.c
	gcc $^ -o $@

rasterize: rasterize.c lib/lodepng.o
	gcc $^ -o $@

clean:
	rm -f compile interpret rasterize **/*.o *.txt *.png
