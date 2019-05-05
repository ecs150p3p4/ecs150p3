# Target library
lib := libuthread.a
objs := sem.o  #tps.o 
Inc := queue.o thread.o

CC := gcc
CFLAGS := -Wall -Werror -g

all: $(lib)

## TODO: Phase 1 and Phase 2
deps := $(patsubst %.o, %.d,$(objs))


-include $(deps)

DEPFLAGS = -MMD -MF $(@: .o = .d)



libuthread.a: $(objs) $(Inc)
	ar rcs $@ $^

%.o: %.c
	@echo "CC $@"
	$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

clean:
	rm -f $(lib) $(objs) $(deps)
