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
#include <assert.h>

static void msg(const char *msg) {
  fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
  int err = errno;
  fprintf(stderr, "[%d] %s\n", err, msg);
  abort();
}

static void do_something(int connfd) {
  char rbuf[64];
  ssize_t n = read(connfd, rbuf, sizeof(rbuf)-1);
  if (n < 0) {
    msg("read() error");
    return;
  }
  printf("client says: %s\n", rbuf);

  char wbuf[] = "world";
  write(connfd, wbuf, strlen(wbuf));
}

static int32_t read_full(int connfd, char *rbuf, size_t n) {
  // read expected number of bytes into rbuf
  while (n > 0) {
    ssize_t read_bytes = read(connfd, rbuf, n);
    if (read_bytes <= 0) {
      return -1; // Early EOF or error
    }
    // decrement bytes to read
    assert((size_t)read_bytes <= n);
    n -= (size_t)read_bytes;
    rbuf += read_bytes;
  }
  return 0;
}

static int32_t write_all(int connfd, const char *wbuf, size_t n) {
  while (n > 0) {
    ssize_t write_bytes = write(connfd, wbuf, n);
    if (write_bytes <= 0) {
      return -1;  // write error
    }
    // decrement bytes to write
    assert((size_t)write_bytes <= n);
    n -= write_bytes;
    wbuf += write_bytes;
  }
  return 0;
}

const size_t k_max_msg = 4096;

static int32_t handle_response(int connfd, void *rbuf, uint32_t msg_len) {
  char wbuf[4 + k_max_msg];
  memcpy(wbuf, rbuf, 4 + msg_len);
  int32_t err = write_all(connfd, wbuf, msg_len);
  if (err) {
    msg("Error writing response");
    return -1;
  }
  return 0;
} 

// handle_request will call full_read() twice -- once to get msg len, then to get msg body
static int32_t one_request(int connfd) {
  char rbuf[4 + k_max_msg];
  errno = 0;
  // Read msg len to first 4 bytes of rbuf
  int32_t err = read_full(connfd, rbuf, 4);
  if (err) {
    msg(errno == 0 ? "EOF" : "read() error");
    return err;  // full_read error
  }

  // Get total msg len from first 4 bytes read
  uint32_t msg_len = 0;
  memcpy(&msg_len, rbuf, 4);
  fprintf(stdout, "%llu\n", (unsigned long long)msg_len);

  if (msg_len > k_max_msg) {
    fprintf(stdout, "%llu\n", (unsigned long long)msg_len);
    msg("Message body too long");
    return -1;
  }

  // Read msg body to rbuf
  err = read_full(connfd, &rbuf[4], msg_len);
  if (err) {
    return -1;  // full_read error
  }
  
  // err = handle_response(connfd, rbuf, msg_len);

	// do something
	fprintf(stderr, "client says: %.*s\n", msg_len, &rbuf[4]);

	// reply using the same protocol
	const char reply[] = "world";
	char wbuf[4 + sizeof(reply)];
	msg_len = (uint32_t)strlen(reply);
	memcpy(wbuf, &msg_len, 4);
	memcpy(&wbuf[4], reply, msg_len);
	return write_all(connfd, wbuf, 4 + msg_len);

  return 0;
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

    while (true) {
      uint32_t err = one_request(connfd);
      if (err) {
        break;
      }
    }
    // do_something(connfd);

    close(connfd);
  }
  return 0;
}