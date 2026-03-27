#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "tcp.h"
#include "args.h"

ssize_t send_all(int sock_fd, const void *buf, size_t len) {
    ssize_t total_bytes_sent = 0;
    char *p = buf;
    while(total_bytes_sent < len) {
        ssize_t sent_bytes = send(sock_fd, p + total_bytes_sent, len - total_bytes_sent, 0);
        if(sent_bytes <= 0) {
            printf("Sending message error\n");
            return -1;
        }
        total_bytes_sent += sent_bytes;
    }
    return total_bytes_sent;
}

ssize_t recv_all(int sock_fd, void *buf, size_t len) {
    ssize_t total_bytes_received = 0;
    char *p = buf;
    while(total_bytes_received < len) {
        ssize_t received_bytes = recv(sock_fd, p + total_bytes_received, len - total_bytes_received, 0);
        if(received_bytes <= 0) {
            printf("Receiving message error\n");
            return -1;
        }
        total_bytes_received += received_bytes;
    }
    return total_bytes_received;
}

int send_start_message(int sock_fd, configuration_flags_t *cft) {
    start_packet_t start_packet;
    
    start_packet.header.signal_type = START;
    start_packet.header.length = sizeof(start_msg_t);

    if(cft->one_way_delay_flag) {start_packet.message.mode = 1;}
    else {start_packet.message.mode = 0;}
    start_packet.message.packet_size = cft->udp_packet_size_in_bytes; 
    start_packet.message.bandwidth = cft->bandwidth_in_bits_per_sec;
    start_packet.message.parallel_num = cft->parallel_num;
    if(cft->has_time_parameter) {start_packet.message.duration = cft->time_to_send_in_seconds;}
    else {start_packet.message.duration = 0;}
    start_packet.message.wait_duration = cft->delay_before_starting_in_seconds;

    if(send_all(sock_fd, &start_packet, sizeof(start_packet)) < 0) {
        return -1;
    }
    return 0;
}

int recv_start_message(int sock_fd, start_msg_t *start_msg) {
    start_packet_t start_packet;
    if(recv_all(sock_fd, &start_packet, sizeof(start_packet)) < 0) {
        return -1;
    }

    if(start_packet.header.signal_type != START) {
        printf("Expected START message, got: %u\n", start_packet.header.signal_type);
        return -1;
    }

    *start_msg = start_packet.message;
    return 0;
}

int send_stop_message(int sock_fd, uint32_t last_seq_num) {
    stop_packet_t stop_packet;
    stop_packet.header.signal_type = STOP;
    stop_packet.header.length = sizeof(stop_msg_t);
    stop_packet.message.last_seq_sent = last_seq_num;
    
    if(send_all(sock_fd, &stop_packet, sizeof(stop_packet)) < 0) {
        return -1;
    }
    return 0;
}

int recv_stop_message(int sock_fd, stop_msg_t *stop_msg) {
    stop_packet_t stop_packet;
    if(recv_all(sock_fd, &stop_packet, sizeof(stop_packet)) < 0) {
        return -1;
    }

    if(stop_packet.header.signal_type != STOP) {
        printf("Expected STOP message, got: %u\n", stop_packet.header.signal_type);
        return -1; 
    }

    *stop_msg = stop_packet.message;
    return 0;
}

int send_exp_exited_message(int sock_fd, exp_exited_msg_t *exp_exited_msg) {
    exp_exited_packet_t exp_exited_packet;
    exp_exited_packet.header.signal_type = EXP_EXITED;
    exp_exited_packet.header.length = sizeof(exp_exited_msg);
    exp_exited_packet.message = *exp_exited_msg;

    if(send_all(sock_fd, &exp_exited_packet, sizeof(exp_exited_packet)) < 0) {
        return -1;
    }
    return 0;
}

int recv_exp_exited_message(int sock_fd, exp_exited_msg_t *exp_exited_msg) {
    exp_exited_packet_t exp_exited_packet;
    if(recv_all(sock_fd, &exp_exited_packet, sizeof(exp_exited_packet)) < 0) {
        return -1;
    }

    if(exp_exited_packet.header.signal_type != EXP_EXITED) {
        printf("Expected EXP_EXITED message, got: %u\n", exp_exited_packet.header.signal_type);
        return -1; 
    }
    *exp_exited_msg = exp_exited_packet.message;
    return 0;
}






