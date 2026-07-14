// stdlib
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
// system
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
// C++
#include <string>
#include <vector>
// proj
#include "hashtable.h"

static void msg(const char *msg) {
  fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] %s\n", err, msg);
  abort();
}

static void do_something(int connfd) {
  char rbuf[64] = {};
  ssize_t n = read(connfd, rbuf, sizeof(rbuf)-1);
  if (n < 0) {
    msg("read() error");
    return;
  }
  printf("client says: %s\n", rbuf);

  char wbuf[] = "world";
  write(connfd, wbuf, strlen(wbuf));
}

void main() {
  // Start listening socket
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    die("socket()");
  }

  int val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  
  // Bind
  struct sockaddr_in addr = {};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(1234);  // port
  addr.sin_addr.s_addr = htonl(0);  // wildcard IP 0.0.0.0
  int rv = bind(server_fd, (const struct sockaddr *)&addr, sizeof(addr));
  if (rv) {
    die("bind()");
  }

  // Listen
  rv = listen(server_fd, SOMAXCONN);
  if (rv) {
    die("listen()");
  }

  while (true) {
    // Accept
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
    if (connfd < 0) {
      continue; // error - wait for next connection
    }

    do_something(connfd);
    close(connfd);
  }
  return 0;
}