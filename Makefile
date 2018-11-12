CC=cc
FLAGS=-Ofast -Wall -Wextra -Wshadow -Werror -ansi -pedantic -std=c99
HEADERS=utils.h
OBJECTS=utils.o
DEPS=$(HEADERS) $(OBJECTS)

ifeq ($(shell uname),Darwin)
	CFLAGS=$(FLAGS)
else
	# clang no likey this flag
	CFLAGS=$(FLAGS) -fmax-errors=3
endif

default:	dismantle remantle

dismantle:	dismantle.c utils.o
	$(CC) $(CFLAGS) $^ -o $@

remantle:	remantle.c utils.o
	$(CC) $(CFLAGS) $^ -o $@
