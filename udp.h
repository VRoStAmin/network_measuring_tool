#ifndef UDP_H
#define UDP_H

#include <stddef.h>
#include <stdint.h>

#include "communication.h"
#include "args.h"

typedef struct { 
    uint64_t sequence_number; /* Needed for packet loss etc..*/
    uint64_t time_sent_ns; /* Needed for one way delay. */
} udp_pseudo_header_t;

typedef struct {
    double throughput;
    double goodput;
    
} exp_results_t;

typedef struct {
    char *server_ip;
    int port;
    uint32_t packet_size;
    int duration_sec;
    uint64_t bandwidth_bps;
    int one_way_delay_flag;

    volatile int *stop;
    uint64_t last_seq_sent;
    exp_exited_msg_t results;
    int status;
} udp_client_thread_t;

typedef struct {
    char *bind_ip;
    int port;
    uint32_t packet_size;
    int one_way_delay_flag;
    int duration_sec;

    volatile int *stop;
    uint64_t final_seq_recv;
    exp_exited_msg_t results;
    int status;
} udp_server_thread_t;

uint64_t nanosec_now();
int udp_client_experiment(char *server_ip, int port, uint32_t packet_size, int duration_sec, uint64_t bandwidth_bps, int one_way_delay_flag, uint64_t *last_seq_sent, volatile int *stop, udp_client_thread_t *ct);
int udp_server_experiment(char *bind_ip, int port, uint32_t packet_size, int one_way_delay_flag, int duration_sec, volatile int *stop, udp_server_thread_t *st);
double calculate_goodput(uint64_t total_bytes, double duration_sec, uint64_t total_packets);
double calculate_throughput(uint64_t total_bytes, double duration_sec, uint64_t total_packets);
void *udp_client_thread_main(void *arg);
void *udp_server_thread_main(void *arg);


#endif