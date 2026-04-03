#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <pthread.h>

#include "client.h"
#include "communication.h"
#include "tcp.h"
#include "udp.h"

int run_client(configuration_flags_t *cft) {
    int client_sock;
    struct sockaddr_in server_addr;
    exp_exited_msg_t results;
    volatile int stop = 0;
    int duration;

    if(cft->has_time_parameter) {
        duration = cft->time_to_send_in_seconds;
    } else {
        duration = 5;
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(client_sock < 0) { 
        printf("Creating client socket error\n");
        return -1;
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cft->port);

    if(inet_pton(AF_INET, cft->address, &server_addr.sin_addr) <= 0) {
        printf("Server address error [TCP PART]\n");
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
    if(cft->delay_before_starting_in_seconds > 0) {
        sleep(cft->delay_before_starting_in_seconds);
    } 

    int num_streams = cft->parallel_num;
    pthread_t *threads = malloc(num_streams * sizeof(pthread_t));
    udp_client_thread_t udp_client_thread_args[num_streams];

    int created_threads = 0;
    for(int i = 0; i < num_streams; i++){
        memset(&udp_client_thread_args[i], 0, sizeof(udp_client_thread_t));
        udp_client_thread_args[i].server_ip = cft->address;
        udp_client_thread_args[i].port = cft->port + 1 + i;
        
        udp_client_thread_args[i].packet_size = cft->udp_packet_size_in_bytes;
        udp_client_thread_args[i].duration_sec = duration;
        udp_client_thread_args[i].bandwidth_bps = cft->bandwidth_in_bits_per_sec / num_streams;
        udp_client_thread_args[i].one_way_delay_flag = cft->one_way_delay_flag;
        udp_client_thread_args[i].stop = &stop;
        udp_client_thread_args[i].last_seq_sent = 0;
        udp_client_thread_args[i].status = -1;

        if(pthread_create(&threads[i], NULL, udp_client_thread_main, &udp_client_thread_args[i]) != 0) {
            printf("Thread create failed on client\n");
            stop = 1;
            for(int j = 0; j < created_threads; j++) {
                pthread_join(threads[j], NULL);
            }

            free(threads);
            close(client_sock);
            return -1;
        }
        created_threads++;
    }

    for(int i = 0; i < num_streams; i++){
        pthread_join(threads[i], NULL);
    }

    for(int i = 0; i < num_streams; i++) {
        if(udp_client_thread_args[i].status != 0) {
            printf("UDP client thread %d failed\n", i);
            free(threads);
            close(client_sock);
            return -1;
        }
    }

    if(send_stop_message(client_sock, udp_client_thread_args[0].last_seq_sent) != 0) {
        printf("Send STOP message error\n");
        free(threads);
        close(client_sock);
        return -1;
    }
    printf("STOP message sent\n");
    
    if(recv_exp_exited_message(client_sock, &results) != 0) {
        printf("Receive EXP_EXITED message error\n");
        free(threads);
        close(client_sock);
        return -1;
    }

    /* Make flag for one way delay results... */
    /* We probably need to write these in json files... */
    /* Or make a program to plot them... */
    printf("\n");
    printf("RESULTS RECEIVED\n");
    printf("Throughput in bps: %f\n", results.throughput_bps);
    printf("Goodput in bps: %f\n", results.goodput_bps);
    printf("Loss percent: %f\n", results.loss_percent);
    printf("Avg_jitter: %f\n", results.avg_jitter_ns);
    printf("Std_jitter: %f\n", results.std_jitter);
    printf("\n");

    printf("____________________________________________________________________\n");
    printf("EVERYTHING SUCCESSFUL ON CLIENT SIDE.\n");
    printf("____________________________________________________________________\n");

    free(threads);
    close(client_sock);
    return 0;
}

