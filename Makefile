CC = g++
CFLAGS = -Wall -g
PROG = sample3D

SRCS = bike.cpp imageloader.cpp vec3f.cpp
LIBS = -lglut -lGL -lGLU

all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
