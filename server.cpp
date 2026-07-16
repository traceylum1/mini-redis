// stdlib
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>

#define MAX_MSG_LEN 4096

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

static int32_t full_read(int connfd, void *rbuf, uint32_t n) {
  // read expected number of bytes into rbuf
  while (n > 0) {
    ssize_t read_bytes = read(connfd, rbuf, n);
    if (read_bytes <= 0) {
      return -1; // Early EOF or error
    }
    // decrement bytes to read
    n -= read_bytes;
    if (n < 0) {
      return -1; // Too many bytes read ?
    }
  }
  // return success -- full msg read
  return 0;
}

static int32_t full_write(int connf, void *rbuf) {
  char wbuf[4 + MAX_MSG_LEN];
}

// handle_request will call full_read() twice -- once to get msg len, then to get msg body
static int32_t handle_request(int connfd) {
  char rbuf[4 + MAX_MSG_LEN];

  // Read msg len to first 4 bytes of rbuf
  int32_t err = full_read(connfd, rbuf, 4);
  if (err) {
    return -1;  // full_read error
  }

  // Get total msg len from first 4 bytes read
  uint32_t msg_len = 0;
  memcpy(&msg_len, rbuf, 4);

  if (msg_len > MAX_MSG_LEN) {
    msg("Message body too long");
    return -1;
  }

  // Read msg body to rbuf
  int32_t err = full_read(connfd, &rbuf[4], msg_len);
  if (err) {
    return -1;  // full_read error
  }
  
}

int main() {
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