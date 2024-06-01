run: all
	./compile < test/tiger.svg | ./interpret | ./rasterize 2 5 > test/out.bmp

debug/rasterize: compile interpret rasterize
	./compile < test/tiger.svg | ./interpret | ./rasterize 2 5 1 > test/debug.bmp

debug/interpret: compile interpret
	./compile < test/tiger.svg | ./interpret > test/interpret.out

debug/compile: compile
	./compile < test/tiger.svg > test/compile.out

all: compile interpret rasterize 

compile: compile.c
	gcc -O3 $^ -o $@

interpret: interpret.c
	gcc -O3 $^ -o $@

rasterize: rasterize.c
	gcc -O3 $^ -o $@

clean:
	rm -f compile interpret rasterize *.o test/*.bmp test/*.out
