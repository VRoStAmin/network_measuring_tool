#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "client.h"

int run_client(configuration_flags_t *cft) {
    int client_sock;
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(client_sock < 0) {
        printf("Creating client socket error\n");
        return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cft->port);

    if(inet_pton(AF_INET, cft->address, &server_addr.sin_addr) <= 0) {
        printf("Server address error\n");
        close(client_sock);
        return -1;
    }

    if(connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connect error\n");
        close(client_sock);
        return -1;
    }

    printf("Connect to server %s:%d\n", cft->address, cft->port);
    
    close(client_sock);
    return 0;
}