#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "tcp.h"

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

int send_message(int sock_fd, uint32_t type, const void *payload, uint32_t length) {
    tcp_header_t header;
    header.signal_type = htonl(type);
    header.length = htonl(length);

    if(send_all(sock_fd, ))
}

int recv_message(int sock_fd, tcp_header_t *header, void *payload_buf, uint32_t buf_size) {

}

