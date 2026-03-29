#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "udp.h"

uint64_t nanosec_now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000 + t.tv_nsec;
}

int udp_send_test(char *server_ip, int port, int packet_size, int duration_sec) {
    int client_sock;
    char *buffer;
    struct sockaddr_in server_address;
    uint64_t seq_num;

    /*Create UDP socket*/
    client_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_sock < 0) {
        printf("socket creation failed\n");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    
    if(inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0){
        printf("Server address error [UDP PART]\n ");
        close(client_sock);
        return -1;
    } 

    seq_num = 0;
    buffer = malloc(packet_size * sizeof(char));
    if(buffer == NULL) {
        printf("Malloc failure\n");
        close(client_sock);
        return -1;
    }

    uint64_t time_now_ns = nanosec_now();
    uint64_t time_end_ns = time_now_ns + duration_sec * 1000000000;
    while(nanosec_now() < time_end_ns) {
        udp_header_t header;
        header.sequence_number = seq_num;
        header.time_sent_ns = nanosec_now();

        memcpy(buffer, &header, sizeof(header));
        ssize_t bytes_sent = sendto(client_sock, buffer, packet_size, 0, (struct sockaddr *)&server_address, sizeof(server_address));
        if(bytes_sent < 0) {
            printf("Error in send to\n");
            close(client_sock);
            free(buffer);
            return -1; 
        }
        printf("Sent UDP packet with sequence number: %llu\n", seq_num);
        seq_num++;
    }
    printf("UDP send test finished, total packets sent: %llu\n", seq_num);
    
    close(client_sock);
    free(buffer);
    return 0;
}

int udp_recv_test(char *bind_ip, int port) {
    int server_sock;
    struct sockaddr_in client_address;
    struct sockaddr_in server_address;
    socklen_t client_addr_len = sizeof(client_address);
    char buffer[1024];

    uint64_t total_packets; 
    uint64_t total_bytes;
    
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_sock < 0) {
        printf("Socket creation failed\n");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    
    if(bind_ip == NULL) {
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if(inet_pton(AF_INET, bind_ip, &server_address.sin_addr) <= 0) {
            printf("Server address error\n");
            close(server_sock);
            return -1;
        }
    }
    
    if(bind(server_sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Bind error\n");
        close(server_sock);
        return -1;    
    }

    total_bytes = 0;
    total_packets = 0;
    printf("Server listening on port %d\n", port);
    while(1) {
        ssize_t bytes_recv = recvfrom(server_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_address, &client_addr_len);
        if(bytes_recv < 0) {
            printf("Error in receive\n");
            close(server_sock);
            return -1;
        }
        
        printf("Received another package\n");
        if(bytes_recv >= sizeof(udp_header_t)) {
            udp_header_t header;
            memcpy(&header, buffer, sizeof(header));
            printf("Received packet seq=%llu\n", header.sequence_number);
        }

        total_packets++;
        total_bytes += (uint64_t)bytes_recv;
        printf("Total packets received so far: %llu\n", total_packets);
        printf("Total bytes received so far: %llu\n", total_bytes);
    }
    
    close(server_sock);
    return 0;
}