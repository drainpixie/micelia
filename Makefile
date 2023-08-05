CC 		:= clang
CFLAGS	:= -std=c99 -Wall -Wextra -g
CFLAGS	+= 

SRCS	:= $(wildcard *.c)
OBJS	:= $(SRCS:.c=.o)
EXEC	:= micelia

INSTALL_PATH := /usr/local/bin

all: $(EXEC)
	./$(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

%.o: %.c
	$(CC) $(CLAGS) -c $< -o $@


install: $(EXEC)
	cp $(EXEC) $(INSTALL_PATH)

clean:
	rm -f $(EXEC) $(OBJS) $(INSTALL_PATH)/$(EXEC)


