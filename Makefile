CFLAGS=-Wall -g -std=gnu11
.DEFAULT := all
.PHONY: all
all: dungeon
clean:
#	rm *.o
	rm dungeon
check:
	make all
	valgrind ./dungeon
