#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"

int run_server(configuration_flags_t *cft) {
    int server_sock; /* One port where the server will be listening on. */ 
    int client_sock; /* One port where the communication between a specific client and the server will happen. */
    
    struct sockaddr_in server_addr; /* Store the servers IP and port. */
    struct sockaddr_in client_addr; /* Store the clients IP and port. */
    socklen_t client_addr_len = sizeof(client_addr); /* Needed for accept(). */

    /* It is a good practise to initialize the structs with zero since we do not want them to contain garbage. */
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));

    /* We are going to use socket() to create the socket descriptor for the server listening. */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0) {
        printf("Creating socket error\n");
        return -1;
    }

    server_addr.family = AF_INET; /* Matching the family of the server address to the one of the sockets. */
    server_addr.port = htons(cft->port);
    if(cft->address == NULL) {
        server_addr.in_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if(inet_pton(AF_INET, cft->address, &server_addr.sin_addr) <= 0) {
            printf("Server address error\n");
            close(server_sock);
            return -1;
        }
    }

    if(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Bind error\n");
        close(server_sock);
        return -1;
    }

    if(listen(server_sock, 3) < 0) {
        printf("Listen error\n");
        close(server_sock);
        return -1;
    }

    printf("Server is started and listening on port: %d\n", cft->port);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
    if(client_sock < 0) {
        printf("Accept error\n");
        close(server_sock);
        return -1;
    }

    printf("Everything succeeded...client connected\n");



    close(client_sock);
    close(server_sock);
    return 0;
}