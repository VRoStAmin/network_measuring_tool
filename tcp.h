#ifndef TCP_H
#define TCP_H

#include <stddef.h>
#include <stdint.h>

#include "communication.h"
#include "args.h"

ssize_t send_all(int sock_fd, const void *buf, size_t len);
ssize_t recv_all(int sock_fd, void *buf, size_t len);

int send_start_message(int sock_fd, configuration_flags_t *cft);
int recv_start_message(int sock_fd, start_msg_t *start_msg);

int send_stop_message(int sock_fd, uint32_t parallel_num, uint64_t *last_seq_sent);
int recv_stop_message(int sock_fd, stop_msg_t *stop_msg, uint64_t **last_seq_sent);

int send_exp_exited_message(int sock_fd, exp_exited_msg_t *exp_exited_msg);
int recv_exp_exited_message(int sock_fd, exp_exited_msg_t *exp_exited_msg);

#endif