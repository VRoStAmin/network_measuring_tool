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

    printf("Server mode selected...\n");
    return 0;
}