#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include "select.hpp"

#define TRUE (1)
#define FALSE (0)

static fd_set rfds, rfds_copy;
static int max_fd = 0;
static int initialized = FALSE;
static int *alloced_fds = NULL;
static int alloced_fds_num = 0;

static int add_fd_to_monitoring_internal(const unsigned int fd) {
  int *tmp_alloc;
  tmp_alloc = (int*)realloc(alloced_fds, sizeof(int)*(alloced_fds_num+1));
  if (tmp_alloc == NULL)
    return -1;
  alloced_fds = tmp_alloc;
  alloced_fds[alloced_fds_num++]=fd;
  FD_SET(fd, &rfds_copy);
  if (max_fd < fd)
    max_fd = fd;
  return 0;
}

static int remove_fd_from_monitoring_internal(const unsigned int fd) {
  int new_alloced_fds_num = alloced_fds_num-1;
  int arr[new_alloced_fds_num];
  int i = 0;
  /* backup all the sockets except from fd */
  while (i<new_alloced_fds_num) {
    if (alloced_fds[i]!=fd) {
      arr[i] = alloced_fds[i];
      i++;
    }
  }
  /* init values */
  initialized = FALSE;
  alloced_fds_num = 0;
  max_fd = 0;
  /* re-add relevant sockets */
  for (int j = 0; j < new_alloced_fds_num; j++) {
    add_fd_to_monitoring(arr[j]);
  }
  return 0;
}

int remove_fd_from_monitoring(const unsigned int fd) {
  return remove_fd_from_monitoring_internal(fd);
}

int init() {
  FD_ZERO(&rfds_copy);
  if (add_fd_to_monitoring_internal(0) < 0)
    return -1; // monitoring standard input
  initialized = TRUE;
  return 0;
}

int add_fd_to_monitoring(const unsigned int fd) {
  if (!initialized)
    init();
  if (fd>0)
    return add_fd_to_monitoring_internal(fd);
  return 0;
}

int wait_for_input() {
  int i, retval;
  memcpy(&rfds, &rfds_copy, sizeof(rfds_copy));
  retval = select(max_fd+1, &rfds, NULL, NULL, NULL);
  if (retval > 0) {
    for (i=0; i<alloced_fds_num; ++i) {
      if (FD_ISSET(alloced_fds[i], &rfds)) {
        return alloced_fds[i];
      }
    }
  }
  return -1;
}