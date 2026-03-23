#ifndef ARGS_H
#define ARGS_H

/* This h file will hold all information that will be passed around all other files. */

/* 
We start by defining a struct that will hold all the information of 
the given configuration when the set up is done. 
   
We could keep them as variables, but that is going to get messy having to pass different 
variables to different checks etc. 
*/

typedef struct {
    int is_server_flag; /* 1 if -s parameter is given, 0 otherwise. */
    int is_client_flag; /* 1 if -c parameter is given, 0 otherwise. */

    /* 
    -a parameter, 
    if -s is given, specifies the IP address of the network interface that the program should bind to.
    if -c is given, specifies the IP address of the server to connect to.      
   */
    char *address;

    /*
    -p parameter,
    if -s given, indicates the listening port of the primary communication TCP channel of the server.
    if -c is given, specifies the server port to connect to.
    */ 
    int port;

    /* 
    -i parameter, 
    The interval in seconds to print information for the progress of the experiment.
    Default value is 1 second, in case none is specified.
    */
    int time_interval; 
    char *file; /* -f parameter, specifies the file that the results will be stored. */
    int udp_packet_size_in_bytes; /* -l parameter, specifies UDP packet size in bytes. */
    int bandwidth_in_bits_per_sec; /* -b parameter, specifies bandwidth in bps of the data stream that the client should send to the server. */
    int parallel_num; /* -n parameter, specifies the number of parallel data streams that the client should create. */

    /*
    -t parameter,
    The experiment duration in seconds, 
    If none specified, the program will continue until termination by the user occurs.
    */
    int time_to_send_in_seconds;
    
    /*
    -d parameter, 
    If 1, measure one way delay, 
    If 0, measure throughput, jitter, packet loss. 
    */
    int one_way_delay_flag;
    int delay_before_starting_in_seconds; /* -w parameter, wait duration in seconds before starting experiment. */
} configuration_flags_t;

#endif