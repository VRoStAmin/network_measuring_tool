#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <pthread.h>

#include "server.h"
#include "communication.h"
#include "tcp.h"
#include "udp.h"


int run_server(configuration_flags_t *cft) {
    int server_sock; /* One port where the server will be listening on. */ 
    int client_sock; /* One port where the communication between a specific client and the server will happen. */
    
    struct sockaddr_in server_addr; /* Store the servers IP and port. */
    struct sockaddr_in client_addr; /* Store the clients IP and port. */
    socklen_t client_addr_len = sizeof(client_addr); /* Needed for accept(). */

    start_msg_t start_msg;
    stop_msg_t stop_msg;
    exp_exited_msg_t exp_exited_msg;
    
    volatile int stop = 0;
    uint64_t *last_seq_sent = NULL;

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

    int opt = 1;
    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        printf("setsockopt error\n");
        close(server_sock);
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
    printf("Bandwidth: %llu\n", start_msg.bandwidth);
    printf("Parallel number of threads: %u\n", start_msg.parallel_num);
    printf("Duration: %u\n", start_msg.duration);
    printf("Wait duration: %u\n", start_msg.wait_duration);
    printf("\n");

    int num_streams = start_msg.parallel_num;
    pthread_t *threads = malloc(num_streams * sizeof(pthread_t));
    udp_server_thread_t *udp_server_thread_args = malloc(num_streams * sizeof(udp_server_thread_t));

    if (threads == NULL || udp_server_thread_args == NULL) {
        printf("Malloc failed on server\n");
        free(threads);
        free(udp_server_thread_args);
        close(client_sock);
        close(server_sock);
        return -1;
    }

    int created_threads = 0;
    for(int i = 0; i < num_streams; i++) {
        memset(&udp_server_thread_args[i], 0, sizeof(udp_server_thread_t));

        udp_server_thread_args[i].bind_ip = cft->address;
        udp_server_thread_args[i].port = cft->port + 1 + i;
        udp_server_thread_args[i].packet_size = start_msg.packet_size;
        udp_server_thread_args[i].one_way_delay_flag = start_msg.mode;
        udp_server_thread_args[i].duration_sec = start_msg.duration;
        
        udp_server_thread_args[i].stop = &stop;
        udp_server_thread_args[i].final_seq_recv = 0;
        udp_server_thread_args[i].packets_received = 0;
        udp_server_thread_args[i].status = -1;
        if(pthread_create(&threads[i], NULL, udp_server_thread_main, &udp_server_thread_args[i]) != 0) {
            printf("Failed in pthread create %d\n", i);
            stop = 1;
            for(int j = 0; j < created_threads; j++) {
                pthread_join(threads[j], NULL);
            }

            free(threads);
            free(udp_server_thread_args);
            close(client_sock);
            close(server_sock);
            return -1;
        }          
        created_threads++;
    }
    if(recv_stop_message(client_sock, &stop_msg, &last_seq_sent) != 0) {
        printf("Receive STOP message error\n");
        stop = 1;
        for(int i = 0; i < created_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        free(threads);
        free(udp_server_thread_args);
        close(client_sock);
        close(server_sock);
        return -1;
    }

    printf("Received STOP message\n");
    for(int i = 0; i < stop_msg.parallel_num; i++) {
        printf("Last sequence number stream %d: %llu\n", i, last_seq_sent[i]);
        udp_server_thread_args[i].final_seq_recv = last_seq_sent[i];
    }

    stop = 1;
    for(int i = 0; i < num_streams; i++) {
        pthread_join(threads[i], NULL);
        if(udp_server_thread_args[i].status != 0) {
            printf("UDP server thread %d failed\n", i);
            free(threads);
            free(udp_server_thread_args);
            close(client_sock);
            close(server_sock);
            return -1;
        }
    }
    memset(&exp_exited_msg, 0, sizeof(exp_exited_msg));
    
    uint64_t total_sent_packets = 0;
    uint64_t total_recv_packets = 0;

    for(int i = 0; i < num_streams; i++){
        uint64_t sent_packets = udp_server_thread_args[i].final_seq_recv + 1;
        uint64_t recv_packets = udp_server_thread_args[i].packets_received;
        uint64_t lost_packets = 0;

        if (sent_packets >= recv_packets) {
            lost_packets = sent_packets - recv_packets;
        }

        if (sent_packets > 0) {
            udp_server_thread_args[i].results.loss_percent = (double)lost_packets * 100.0 / (double)sent_packets;
        } else {
            udp_server_thread_args[i].results.loss_percent = 0.0;
        }

        total_sent_packets += sent_packets;
        total_recv_packets += recv_packets;
        
        exp_exited_msg.throughput_bps += udp_server_thread_args[i].results.throughput_bps;
        exp_exited_msg.goodput_bps += udp_server_thread_args[i].results.goodput_bps;
        exp_exited_msg.avg_jitter_ns += udp_server_thread_args[i].results.avg_jitter_ns;
        exp_exited_msg.std_jitter += udp_server_thread_args[i].results.std_jitter;
        exp_exited_msg.one_way_delay += udp_server_thread_args[i].results.one_way_delay;
    }
    
    if (num_streams > 0) {
        exp_exited_msg.loss_percent /= num_streams;
        exp_exited_msg.avg_jitter_ns /= num_streams;
        exp_exited_msg.std_jitter /= num_streams;
        exp_exited_msg.one_way_delay /= num_streams;
    }

    uint64_t total_lost_packets = 0;
    if (total_sent_packets >= total_recv_packets) {
        total_lost_packets = total_sent_packets - total_recv_packets;
    }

    if (total_sent_packets > 0) {
        exp_exited_msg.loss_percent = (double)total_lost_packets * 100.0 / (double)total_sent_packets;
    } else {
        exp_exited_msg.loss_percent = 0.0;
    }

    if(send_exp_exited_message(client_sock, &exp_exited_msg) != 0) {
        printf("Send EXP_EXITED message error\n");
        free(last_seq_sent);
        free(threads);
        free(udp_server_thread_args);
        close(client_sock);
        close(server_sock);
        return -1;
    }
    
    printf("Sent EXP_EXITED message to client\n");
    printf("____________________________________________________________________\n");
    printf("EVERYTHING SUCCESSFUL ON SERVER SIDE.\n");
    printf("____________________________________________________________________\n");
    
    free(threads);
    free(udp_server_thread_args);
    free(last_seq_sent);
    close(client_sock);
    close(server_sock);
    return 0;
}
