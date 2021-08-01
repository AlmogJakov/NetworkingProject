#pragma once

int add_fd_to_monitoring(const unsigned int fd);
int remove_fd_from_monitoring(const unsigned int fd);
int wait_for_input();