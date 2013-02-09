APP_NAME=land.exe

PLATS= linux windows macosx iphone android

CC = gcc

CFLAGS	= -Wall -O2 -I..
LDFLAGS = -lm

OBJS	= main.o math.o world.o model.o assets.o collision.o

all : 	linux

none:
	@echo "Please do"
	@echo "   make PLATFORM"
	@echo "where PLATFORM is one of these:"
	@echo "   $(PLATS)"
	@echo "See INSTALL for complete instructions."

linux: $(OBJS) sys_linux.o
	$(CC) $(CFLAGS) sys_linux.o $(OBJS) -o $(APP_NAME) $(LDFLAGS) -lGL -lX11

windows: $(OBJS) sys_windows.o
	$(CC) $(CFLAGS) sys_windows.o $(OBJS) -o $(APP_NAME).exe $(LDFLAGS) -lopengl32 -lwin32

macosx iphone android:
	echo "Platform still unsupported, will be added soon..."

clean:
	rm -f *.o $(APP_NAME)

.PHONY: all $(PLATS) clean
