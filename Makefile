all: myalloc

myalloc: myalloc.c
	gcc -Wall -Wextra -o myalloc myalloc.c