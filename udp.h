#ifndef UDP_H
#define UDP_H

#include <stddef.h>
#include <stdint.h>

#include "communication.h"
#include "args.h"

typedef struct { 
    uint64_t sequence_number; /* Needed for packet loss etc..*/
    uint64_t time_sent_ns; /* Needed for one way delay. */
} udp_header_t;

uint64_t nanosec_now();
int udp_send_test(char *server_ip, int port, int packet_size, int duration_sec);
int udp_recv_test(char *bind_ip, int port);



#endif