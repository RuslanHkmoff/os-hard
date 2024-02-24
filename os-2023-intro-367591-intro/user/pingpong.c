#include "user/user.h"

void *realloc(void *ptr, int old_capacity, int new_capacity) {
  void *new_ptr = malloc(new_capacity);
  memcpy(new_ptr, ptr, old_capacity);
  free(ptr);
  return new_ptr;
}
void getBytes(int *from, char *buf) {
  int size = 2, ind = 0;
  int bytes;
  while ((bytes = read(from[0], buf + ind, size - ind)) > 0) {
    if (bytes == -1) {
      printf("read error\n");
      exit(1);
    }
    ind += bytes;
    if (ind >= size) {
      buf = (char *)realloc(buf, size, 2 * size);

      size *= 2;
    }
  }
  buf[ind] = '\0';
}

void parentProcess(int *from, int *to, const char *send) {
  close(to[0]);
  close(from[1]);
  write(to[1], send, strlen(send));
  close(to[1]);

  char *buf = malloc(1);
  getBytes(from, buf);
  close(from[0]);
  printf("%d: got %s\n", getpid(), buf);
}
void childProcess(int *from, int *to, const char *send) {
  close(from[1]);
  close(to[0]);
  char *buf = malloc(1);
  getBytes(from, buf);
  close(from[0]);
  printf("%d: got %s\n", getpid(), buf);
  close(from[0]);

  write(to[1], send, strlen(send));
  close(to[1]);
}

int main() {
  int fd1[2];
  int fd2[2];
  pipe(fd1);
  pipe(fd2);
  if (pipe(fd1) != 0 || pipe(fd2) != 0) {
    printf("pipe error\n");
    exit(1);
  }
  const char *pong = "pong";
  const char *ping = "ping";
  int pid = fork();
  if (pid == 0) {
    childProcess(fd1, fd2, pong);
  } else if (pid > 0) {
    parentProcess(fd2, fd1, ping);
  } else {
    printf("fork error\n");
    exit(1);
  }
  exit(0);
}