run: all
	./compile < tiger.svg | ./interpret | ./rasterize 1 10

debug/compile: compile
	./compile < tiger.svg > compile.out

debug/interpret: compile interpret
	./compile < tiger.svg | ./interpret > interpret.out

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
	rm -f compile interpret rasterize **/*.o *.txt
