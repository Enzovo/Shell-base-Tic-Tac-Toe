CC = gcc
CFLAGS = -g -std=c99 -Wall -Werror -fsanitize=address
# CFLAGS = -g -std=c99 -Wall -Werror
all:ttts ttt
ttts: ttts.c
	@$(CC) $(CFLAGS) $^ -o $@
ttt: ttt.c
	@$(CC) $(CFLAGS) -pthread $^ -o $@

clean:
	@rm -f ttts ttt
