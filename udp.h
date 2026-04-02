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

uint64_t nanosec_now();
int udp_client_experiment(char *server_ip, int port, uint32_t packet_size, int duration_sec, uint64_t bandwidth_bps, int one_way_delay_flag);
int udp_server_experiment(char *bind_ip, int port, uint32_t packet_size, int one_way_delay_flag, int duration_sec);
double calculate_goodput(uint64_t total_bytes, double duration_sec, uint64_t total_packets);
double calculate_throughput(uint64_t total_bytes, double duration_sec, uint64_t total_packets);


#endif