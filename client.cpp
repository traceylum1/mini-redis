#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <assert.h>

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s/n", err, msg);
    abort();
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

static int32_t write_full(int connfd, const char *wbuf, size_t n) {
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

static void do_something(int fd) {
    char msg[] = "hello";
    write(fd, msg, strlen(msg));

    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf)-1);
    if (n < 0) {
        die("read()");
    }
    printf("server says: %s\n", rbuf);
}

static int32_t query(int fd, const char *text) {
    uint32_t len = strlen(text);
    char wbuf[4 + len];
    memcpy(wbuf, &len, 4);
    memcpy(wbuf, &wbuf[4], len);
    uint32_t err = write_full(fd, wbuf, 4 + len);
    if (err) {
        return err;
    }

    char rbuf[64] = {};
    ssize_t n = read(fd, rbuf, sizeof(rbuf)-1);
    if (n < 0) {
        die("read()");
    }
    printf("server says: %s\n", rbuf);
    return 0;
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1
    int rv = connect(fd, (const sockaddr *)&addr, sizeof(addr));
    if (rv) {
        die("connect()");
    }
    
    int32_t err = query(fd, "hello");
    if (err) {
        return err;
    }

    // do_something(fd);
    close(fd);
    return 0;
}