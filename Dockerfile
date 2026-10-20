FROM gcc:13 AS build

WORKDIR /src
COPY server.c server.h stringFuncs.h file.h media.h ./
RUN gcc -Wall -Wextra -std=gnu99 -O2 -pthread server.c -o server

FROM debian:bookworm-slim
WORKDIR /app
COPY --from=build /src/server ./server
COPY www ./www

EXPOSE 5050
CMD ["./server"]