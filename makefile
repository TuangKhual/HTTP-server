CFLAGS = -Wall -Wextra -std=gnu99 -ggdb -pthread

server: server.c stringFuncs.h file.h server.h media.h
	gcc ${CFLAGS} server.c -o server

.PHONY: clean

clean:
	rm -f server