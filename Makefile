all: parse simplify render

parse: compile.c
	gcc compile.c -o compile

simplify: simplify.c
	gcc simplify.c -o simplify

render: render.c
	gcc render.c -o render
