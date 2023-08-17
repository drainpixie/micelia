CC 		 := clang
CFLAGS := -std=gnu99 -fsanitize=address -Wall -Wextra -g
CFLAGS +=

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

INSTALL_PATH := /usr/local/bin
MAN_INSTALL_PATH := /usr/local/share/man/man1

all: micelia.c
	./micelia

micelia: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o micelia

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: micelia micelia.1
	cp micelia $(INSTALL_PATH)
	cp micelia.1 $(MAN_INSTALL_PATH)

uninstall:
	rm $(INSTALL_PATH)/micelia
	rm $(MAN_INSTALL_PATH)/micelia.1

clean:
	rm -f micelia $(OBJS)

