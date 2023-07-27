CC 		:= clang
CFLAGS	:= -std=c99 -Wall -Wextra -g

SRCS	:= $(wildcard *.c)
OBJS	:= $(SRCS:.c=.o)
EXEC	:= micelia

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

%.o: %.c
	$(CC) $(CLAGS) -c $< -o $@

clean:
	rm -f $(EXEC) $(OBJS)

