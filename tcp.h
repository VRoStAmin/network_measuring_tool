#ifndef TCP_H
#define TCP_H

#include <stddef.h>
#include <stdint.h>

#include "communication.h"

ssize_t send_all(int sock_fd, const void *buf, size_t len);
ssize_t recv_all(int sock_fd, void *buf, size_t len);

int send_message(int sock_fd, uint32_t type, const void *payload, uint32_t length);
int recv_message(int sock_fd, tcp_header_t *header, void *payload_buf, uint32_t buf_size);

#endif