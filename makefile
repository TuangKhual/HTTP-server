CFLAGS = -Wall -Wextra -std=c99 -ggdb

server: server.c
	gcc ${CFLAGS} server.c -o server

.PHONY: clean

clean:
	rm -f server