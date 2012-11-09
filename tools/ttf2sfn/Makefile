APP_NAME=ttf2sfn

CC = gcc

CFLAGS  = -Wall -O2
LDFLAGS = -lm

OBJS  = main.o stb_image.o

all:   linux

linux: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(APP_NAME) $(LDFLAGS)

clean:
	rm -f *.o $(APP_NAME)

.PHONY: all clean
