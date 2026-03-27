#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"
#include "communication.h"
#include "tcp.h"

int run_server(configuration_flags_t *cft) {
    int server_sock; /* One port where the server will be listening on. */ 
    int client_sock; /* One port where the communication between a specific client and the server will happen. */
    
    struct sockaddr_in server_addr; /* Store the servers IP and port. */
    struct sockaddr_in client_addr; /* Store the clients IP and port. */
    socklen_t client_addr_len = sizeof(client_addr); /* Needed for accept(). */

    start_msg_t start_msg;
    stop_msg_t stop_msg;
    exp_exited_msg_t exp_exited_msg;

    /* It is a good practise to initialize the structs with zero since we do not want them to contain garbage. */
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    memset(&start_msg, 0, sizeof(start_msg));
    memset(&stop_msg, 0 , sizeof(stop_msg));
    memset(&exp_exited_msg, 0, sizeof(exp_exited_msg));

    /* We are going to use socket() to create the socket descriptor for the server listening. */
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0) {
        printf("Creating server socket error\n");
        return -1;
    }

    server_addr.sin_family = AF_INET; /* Matching the family of the server address to the one of the sockets. */
    server_addr.sin_port = htons(cft->port);
    if(cft->address == NULL) {
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
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

    printf("Client connected\n");

    if(recv_start_message(client_sock, &start_msg) != 0) {
        printf("Receive START message error\n");
        close(client_sock);
        close(server_sock);
        return -1;
    }

    printf("\n");
    printf("Received START message with the following parameters\n");
    printf("Mode: %u\n", start_msg.mode);
    printf("Packet size: %u\n", start_msg.packet_size);
    printf("Bandwidth: %u\n", start_msg.bandwidth);
    printf("Parallel number of threads: %u\n", start_msg.parallel_num);
    printf("Duration: %u\n", start_msg.duration);
    printf("Wait duration: %u\n", start_msg.wait_duration);
    printf("\n");

    if(recv_stop_message(client_sock, &stop_msg) != 0) {
        printf("Receive STOP message error\n");
        close(client_sock);
        close(server_sock);
        return -1;
    }
    printf("Received STOP message\n");
    printf("Last sequence number: %u\n", stop_msg.last_seq_sent);

    exp_exited_msg.avg_jitter_ns = 20;
    exp_exited_msg.goodput_bps = 20;
    exp_exited_msg.loss_percent = 20;
    exp_exited_msg.one_way_delay = 20;
    exp_exited_msg.std_jitter = 20;
    exp_exited_msg.throughput_bps = 20;

    if(send_exp_exited_message(client_sock, &exp_exited_msg) != 0) {
        printf("Send EXP_EXITED message error\n");
        close(client_sock);
        close(server_sock);
        return -1;
    }
    
    printf("Sent EXP_EXITED message to client\n");
    printf("____________________________________________________________________\n");
    printf("EVERYTHING SUCCESSFUL ON SERVER SIDE.\n");
    printf("____________________________________________________________________\n");
    
    close(client_sock);
    close(server_sock);
    return 0;
}
