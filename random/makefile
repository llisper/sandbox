CFLAGS = -g -Wall
srcs = $(wildcard *.cc)
objs = $(patsubst %.cc, %.o, $(srcs))

random: $(objs)
	$(CC) -o $@ $< -lstdc++

%.o: %.cc
	$(CC) $(CFLAGS) -c -o $@ $<
