#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>

/* We will have different signal messages depending on what we want to do. */
#define START 1
#define STOP 2
#define EXP_EXITED 3

/* Basic header that will have the type of the message, and the length of it. */
typedef struct {
    uint32_t signal_type; /* Start, stop, exp_exited */
    uint32_t length;
} tcp_header_t;

/* Defining the START message and what it will hold, based on the flags that were given. */
typedef struct {
    /* If 0 we are counting throughput/jitter/loss, 
       If 1 we are counting one way delay. 
    */
    uint32_t mode; 

    uint32_t packet_size; /* Packet size set. */
    uint64_t bandwidth; /* Bandwidth in bps. */
    uint32_t parallel_num; /* Number of parallel streams. */
    uint32_t duration; /* if 0 it means we have a duration till the user stops it. */
    uint32_t wait_duration; /* Duration in seconds to wait before the experiment starts. */
} start_msg_t;

typedef struct {
    /* 
    At the end message, we report the last sequence number,
    this way the server knows which last udp packet it has received, 
    and it can compute the last packet loss needed.
    (This may get changed later...if the logic is wrong)
    */
    uint62_t last_seq_sent; 
} stop_msg_t;

/* 
Will be useful to reported the finished result back to the client, 
when the server receives the END message.
*/
typedef struct {
    double throughput_bps;
    double goodput_bps;
    double loss_percent;
    double avg_jitter_ns;
    double std_jitter;
    double one_way_delay;
} exp_exited_msg_t;

/* Packets with header and message. That will be sent by tcp for signaling. */
typedef struct {
    tcp_header_t header;
    start_msg_t message;
} start_packet_t;

typedef struct {
    tcp_header_t header;
    stop_msg_t message;
} stop_packet_t;

typedef struct {
    tcp_header_t header;
    exp_exited_msg_t message;
} exp_exited_packet_t;

#endif