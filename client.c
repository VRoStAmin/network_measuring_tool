#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>

#include "client.h"
#include "communication.h"
#include "tcp.h"
#include "udp.h"

// int run_client(configuration_flags_t *cft) {
//     int client_sock;
//     struct sockaddr_in server_addr;
//     exp_exited_msg_t results;
//     memset(&server_addr, 0, sizeof(server_addr));
    
//     client_sock = socket(AF_INET, SOCK_STREAM, 0);
//     if(client_sock < 0) { 
//         printf("Creating client socket error\n");
//         return -1;
//     }
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(cft->port);

//     if(inet_pton(AF_INET, cft->address, &server_addr.sin_addr) <= 0) {
//         printf("Server address error [TCP PART]\n");
//         close(client_sock);
//         return -1;
//     }

//     if(connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
//         printf("Connect error\n");
//         close(client_sock);
//         return -1;
//     }

//     printf("Connected to server %s:%d\n", cft->address, cft->port);
//     if(send_start_message(client_sock, cft) != 0) {
//         printf("Send START message error\n");
//         close(client_sock);
//         return -1;
//     }
//     printf("START message sent\n");

//     if(send_stop_message(client_sock, 1212) != 0) {
//         printf("Send STOP message error\n");
//         close(client_sock);
//         return -1;
//     }
//     printf("STOP message sent\n");
    
//     if(recv_exp_exited_message(client_sock, &results) != 0) {
//         printf("Receive EXP_EXITED message error\n");
//         close(client_sock);
//         return -1;
//     }

//     /* Make flag for one way delay results... */
//     /* We probably need to write these in json files... */
//     /* Or make a program to plot them... */
//     printf("\n");
//     printf("RESULTS RECEIVED\n");
//     printf("Throughput in bps: %f\n", results.throughput_bps);
//     printf("Goodput in bps: %f\n", results.goodput_bps);
//     printf("Loss percent: %f\n", results.loss_percent);
//     printf("Avg_jitter: %f\n", results.avg_jitter_ns);
//     printf("Std_jitter: %f\n", results.std_jitter);
//     printf("\n");

//     printf("____________________________________________________________________\n");
//     printf("EVERYTHING SUCCESSFUL ON CLIENT SIDE.\n");
//     printf("____________________________________________________________________\n");

//     close(client_sock);
//     return 0;
// }

int run_client(configuration_flags_t *cft) {
    int duration;

    if (cft->has_time_parameter) {
        duration = cft->time_to_send_in_seconds;
    } else {
        duration = 5;
    }

    printf("Running standalone UDP sender test\n");
    printf("Server IP: %s\n", cft->address);
    printf("Port: %d\n", cft->port);
    printf("Packet size: %d\n", cft->udp_packet_size_in_bytes);
    printf("Duration: %d sec\n", duration);

    return udp_client_experiment(cft->address, cft->port, (uint32_t)cft->udp_packet_size_in_bytes, duration, cft->bandwidth_in_bits_per_sec);
}