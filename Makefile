CC=gcc
CFLAGS=-Wall -Wextra
LIBS=-L lib -lraylib -lgdi32 -lwinmm
INCLUDES=-I include

build: tetris.c
	$(CC) $(CFLAGS) -o tetris tetris.c $(INCLUDES) $(LIBS)

clean: tetris.exe
	rm tetris.exe
