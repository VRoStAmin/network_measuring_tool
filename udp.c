#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "udp.h"

#define UDP_HEADER_SIZE 8
#define IP_HEADER_SIZE 20
#define ETHERNET_HEADER_SIZE 14

#define OVERHEAD_SIZE (UDP_HEADER_SIZE + IP_HEADER_SIZE + ETHERNET_HEADER_SIZE)

uint64_t nanosec_now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ULL + (uint64_t)t.tv_nsec;
}

int udp_send_test(char *server_ip, int port, uint32_t packet_size, int duration_sec) {
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
    size_t full_packet_size = packet_size + sizeof(udp_pseudo_header_t);

    seq_num = 0;
    buffer = malloc(full_packet_size * sizeof(char));
    if(buffer == NULL) {
        printf("Malloc failure\n");
        close(client_sock);
        return -1;
    }

    memset(buffer, 0, full_packet_size);
    
    uint64_t time_now_ns = nanosec_now();
    uint64_t time_end_ns = time_now_ns + (uint64_t)duration_sec * 1000000000ULL;
    
    while(nanosec_now() < time_end_ns) {
        udp_pseudo_header_t header;
        header.sequence_number = seq_num;
        header.time_sent_ns = nanosec_now();

        memcpy(buffer, &header, sizeof(header));
        ssize_t bytes_sent = sendto(client_sock, buffer, full_packet_size, 0, (struct sockaddr *)&server_address, sizeof(server_address));
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

int udp_recv_test(char *bind_ip, int port, uint32_t packet_size, int duration_sec) {
    int server_sock;
    struct sockaddr_in client_address;
    struct sockaddr_in server_address;
    socklen_t client_addr_len = sizeof(client_address);
    char *buffer;

    uint64_t total_packets; 
    uint64_t total_bytes;
    
    exp_results_t results;
    
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
    size_t full_packet_size = packet_size + sizeof(udp_pseudo_header_t);

    buffer = malloc((packet_size + 1024 ) * sizeof(char));
    if (buffer == NULL) {
        printf("Malloc failure\n");
        close(server_sock);
        return -1;
    }
    memset(buffer, 0, (packet_size + 1024) * sizeof(char));
    printf("Server listening on port %d\n", port);

    int has_started = 0;
    uint64_t time_end_ns = 0;
    uint64_t time_now_ns = 0;
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while(1) {
        ssize_t bytes_recv = recvfrom(server_sock, buffer, full_packet_size, 0, (struct sockaddr *)&client_address, &client_addr_len);

        if (bytes_recv < 0) {
            if (has_started && nanosec_now() >= time_end_ns) {
                break;
            }
            continue;
        }
        
        if(!has_started) {
            has_started = 1; 
            time_now_ns = nanosec_now();
            time_end_ns = time_now_ns + (uint64_t)duration_sec * 1000000000ULL;
        }

        //printf("Received another package\n");
        if(bytes_recv >= sizeof(udp_pseudo_header_t)) {
            udp_pseudo_header_t header;
            memcpy(&header, buffer, sizeof(header));
            total_packets++;
            total_bytes += (uint64_t)bytes_recv;
            //printf("Received packet seq=%llu\n", header.sequence_number);
        } else {
            printf("Received packet too small: %zu bytes\n", bytes_recv);
        }

        if (has_started && nanosec_now() >= time_end_ns) {
            break;
        }
    }
    
    results.throughput = calculate_throughput(total_bytes, duration_sec, total_packets);
    results.goodput = calculate_goodput(total_bytes, duration_sec, total_packets);
    
    printf("Packets received: %llu\n", total_packets);
    printf("Bytes received: %llu\n", total_bytes);
    printf("Throughput: %.3f bps\n", results.throughput);
    printf("Goodput: %.3f bps\n", results.goodput);
    
    close(server_sock);
    free(buffer);
    return 0;
}

/*calculate throughput*/
double calculate_throughput(uint64_t total_bytes, double duration_sec, uint64_t total_packets){
    double true_total_bytes = total_bytes - (total_packets * sizeof(udp_pseudo_header_t));
    double throughput_bps = (8.0 * (true_total_bytes + (total_packets * OVERHEAD_SIZE))) / duration_sec;
    return throughput_bps;
}

double calculate_goodput(uint64_t total_bytes, double duration_sec, uint64_t total_packets){
    double true_total_bytes = total_bytes - (total_packets * sizeof(udp_pseudo_header_t));
    double goodput_bps = (8.0 * true_total_bytes) / duration_sec;
    return goodput_bps;
}
