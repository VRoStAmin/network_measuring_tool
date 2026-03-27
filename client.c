#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "client.h"
#include "communication.h"
#include "tcp.h"

int run_client(configuration_flags_t *cft) {
    int client_sock;
    struct sockaddr_in server_addr;
    exp_exited_msg_t results;
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

    printf("Connected to server %s:%d\n", cft->address, cft->port);
    if(send_start_message(client_sock, cft) != 0) {
        printf("Send START message error\n");
        close(client_sock);
        return -1;
    }
    printf("START message sent\n");

    if(send_stop_message(client_sock, 1212) != 0) {
        printf("Send STOP message error\n");
        close(client_sock);
        return -1;
    }
    printf("STOP message sent\n");
    
    if(recv_exp_exited_message(client_sock, &results) != 0) {
        printf("Receive EXP_EXITED message error\n");
        close(client_sock);
        return -1;
    }

    /* Make flag for one way delay results... */
    /* We probably need to write these in json files... */
    /* Or make a program to plot them... */
    printf("\n");
    printf("RESULTS RECEIVED\n");
    printf("Throughput in bps: %ld\n", results.throughput_bps);
    printf("Goodput in bps: %ld\n", results.goodput_bps);
    printf("Loss percent: %ld\n", results.loss_percent);
    printf("Avg_jitter: %ld\n", results.avg_jitter_ns);
    printf("Std_jitter: %ld\n", results.std_jitter);
    printf("\n");

    printf("____________________________________________________________________\n");
    printf("EVERYTHING SUCCESSFUL ON CLIENT SIDE.\n");
    printf("____________________________________________________________________\n");

    close(client_sock);
    return 0;
}