CFLAGS = -Wall -Wextra -std=gnu99 -ggdb -pthread

server: server.c stringFuncs.h file.h
	gcc ${CFLAGS} server.c stringFuncs.h file.h -o server

.PHONY: clean

clean:
	rm -f server