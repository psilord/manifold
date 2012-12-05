CC = gcc

# Add source files in here that I want to compile...
SRCS = cortex.c \
	input.c \
	intqueue.c \
	main.c \
	reverse.c \
	som.c \
	symbol.c \
	slq.c \
	conv.c \
	utils.c \
	vinput.c

# name of the created program
TARGET = de

# Flags I wish to define on the compile line.
DEF_FLAGS = -g -Wall
#DEF_FLAGS = -O3 -Wall -march=native -mtune=native


# Where to find various libraries.
LINKPATH = -L/usr/X11R6/lib -L/usr/local/lib

# Where are the API header files I'm using?
INCLUDEPATH = -I/usr/local/include -I/usr/include/GL -I/usr/include/SDL

# Various libraries I'm going to need...
SDL_LIBDS = -lSDL
XLIBSWIERD = -lXt -lICE -lSM
XLIBS = -lX11 -lXext -lXmu -lXi $(XLIBSWIERD)
GLLIBS = -lGL -lGLU 
OTHERLIBS = -lSDL -lm -lpthread
LIBS = $(SDL_LIBS) $(GLLIBS) $(XLIBS) $(OTHERLIBS)

###################################################################
# Generally you don't want to mess with stuff below this line...
###################################################################

CFLAGS = $(DEF_FLAGS) $(INCLUDEPATH) 

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

OBJS := $(patsubst %.c,%.o,$(SRCS))

$(TARGET): .autodepfile $(OBJS)
	$(CC) $(CFLAGS) $(LINKPATH) $(OBJS) $(LIBS) -o $(TARGET)

.PHONY: clean
clean:
	- rm -f $(TARGET) core a.out $(OBJS) gmon.out .depfile .autodepfile *.i *.s callgrind.out.* cachegrind.out.*

.PHONY: lines
lines:
	wc -l *.c *.h

# This is set up with the dependancies and file name such that make doesn't
# try and rebuild the depfile for stuff like make clean...
.autodepfile: $(SRCS)
	$(CC) -MM $(INCLUDEPATH) $^ > .depfile
	touch .autodepfile

# Include any created dependancies...
-include .depfile



