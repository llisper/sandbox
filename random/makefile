CFLAGS = -g -Wall
srcs = $(wildcard *.cc)
objs = $(patsubst %.cc, %.o, $(srcs))

all: random single

random: random.o
	$(CC) -o $@ $< -lstdc++

random.o: random.cc
	$(CC) $(CFLAGS) -c -o $@ $<

single: single.o
	$(CC) -o $@ $< -lstdc++

single.o: single.cc
	$(CC) $(CFLAGS) -c -o $@ $<
