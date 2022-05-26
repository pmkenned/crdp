.PHONY: all run clean

CC ?= gcc
CPPFLAGS = -MMD
CFLAGS = -ggdb -std=c99 -Wall -Wextra -Werror
CFLAGS += -Wno-unused-label -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
CFLAGS += -Wno-enum-conversion

all: ./build/crdp

SRC=$(wildcard *.c)
OBJ=$(SRC:%.c=./build/%.o)
DEP=$(OBJ:%.o=%.d)

./build/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

./build/crdp: $(OBJ)
	mkdir -p $(@D)
	$(CC) -o $@ $^

-include $(DEP)

run: ./build/crdp
	./build/crdp

clean:
	rm -rf build/
