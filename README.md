# HTTP-server
Lightweight multithreaded HTTP/1.1 server written in C on Linux. As of right now it serves static files from the www directory. I built this to learn how an HTTP works and how servers handle sockets and threads. I plan to use this in the future as the backend to another project.

## Features:
- Multithreaded - Each connection is handled by a separate POSIX (pthread) thread
- Zero copy file transfer with sendfile
- Proper HTTP responses (200, 206, 400, 404, 416, 500) status codes
- Custom string helpers for splitting and comparing strings with copying

## Requirements:
- Linux as it uses 'sendfile' and '<linux/limits.h>') 
- C complier

Build and run:
```bash
  make
```
Then run: 
```bash
  ./server
```

  Then open localhost:5050 in a browser
