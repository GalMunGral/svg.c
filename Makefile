run: all
	./compile < tiger.svg | ./interpret | ./rasterize 2 5

debug/rasterize: compile interpret rasterize
	./compile < tiger.svg | ./interpret | ./rasterize 2 5 1

debug/interpret: compile interpret
	./compile < tiger.svg | ./interpret > interpret.out

debug/compile: compile
	./compile < tiger.svg > compile.out

all: compile interpret rasterize 

compile: compile.c
	gcc -O3 $^ -o $@

interpret: interpret.c
	gcc -O3 $^ -o $@

rasterize: rasterize.c
	gcc -O3 $^ -o $@

clean:
	rm -f compile interpret rasterize *.o *.bmp *.out
