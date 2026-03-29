#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "udp.h"

int udp_send_test(char *server_ip, int port, int packet_size, int duration_sec) {
    int sock;
    char buffer[1024];
    struct sockaddr_in server_address;
    /*Create UDP socket*/
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("socket creation failed\n");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_NET;
    server_address.sin_port = htons(port);
    
    if(inet_pton(AF_INET, server_ip, &server_address.sin_addr)<=0){
        printf("Server address error [UDP PART]\n ");
        close(sockfd);
        return -1;
    } 
    
    /* I dont think we need bind here since we are not receiving back from the server. Check this. */
    if(bind(sockfd, (const struct sockaddr *)&server_address, sizeof(server_address)) < 0){
        printf("bind failed\n");
        close(sockfd);
        return -1;
    }

    socklen_t len = sizeof(server_address);

    sendto(sockfd, buffer, strlen(buffer), MSG_CONFIRM, (const struct sockaddr*)&server_address, len);
    printf("UDP message  sent\n");

    close(sockfd);
    
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
        if(inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0) {
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

    printf("Server listening on port %d\n", port);
    while(1) {
        ssize_t bytes_recv = recvfrom(server_sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_address, &client_addr_len);
        if(bytes_recv < 0) {
            printf("Error in receive\n");
            close(server_sock);
            return -1;
        }

        total_packets++;
        total_bytes += (uint64_t)bytes_recv;
        printf("Received another package\n");
        printf("Total packets received so far: %llu\n", total_packets);
        printf("Total bytes received so far: %llu\n", total_bytes);
    }
    close(server_sock);
    return 0;
}