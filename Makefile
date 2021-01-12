WARNINGS = -Wall -Wextra -Wshadow -Wconversion -Winline -Werror
HEADERS=utils.h
OBJECTS=utils.o
DEPS=$(HEADERS) $(OBJECTS)

ifeq ($(shell $(CC) --version | awk 'NR == 1 { print $$1 }'),clang)
FLAGS = -fsanitize=shift
else # assume GCC
WARNINGS += -Werror=format=0
FLAGS = -fmax-errors=3
endif

CFLAGS=-Ofast -ansi -pedantic -std=c99 $(WARNINGS) $(FLAGS)

default:	dismantle remantle

dismantle:	dismantle.c utils.o
	$(CC) $(CFLAGS) $^ -o $@

remantle:	remantle.c utils.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm *.o dismantle remantle || true
