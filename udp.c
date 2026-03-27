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
    char buffer[1024];
    
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
    
    if(bind(server_sock, (const struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Bind error\n");
        close(server_sock);
        return -1;    
    }

    printf("Server listenin" on port %d\n), port

    ;
    
}