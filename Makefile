CC := clang
CFLAGS := -std=c99 -fsanitize=address -Wall -Wextra -g
CFLAGS +=

SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)
EXEC := micelia

INSTALL_PATH := /usr/local/bin
MAN_INSTALL_PATH := /usr/local/share/man/man1

all: $(EXEC)
	./$(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(EXEC) micelia.1
	cp $(EXEC) $(INSTALL_PATH)
	cp micelia.1 $(MAN_INSTALL_PATH)

uninstall:
	rm $(INSTALL_PATH)/$(EXEC)
	rm $(MAN_INSTALL_PATH)/micelia.1

clean:
	rm -f $(EXEC) $(OBJS)

