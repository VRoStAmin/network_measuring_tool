#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "args.h"

int char_to_int(const char *s, int *value) {
    char *endptr;
    long x;

    if (s == NULL || *s == '\0') return -1;
    
    x = strtol(s, &endptr, 10);
    if (*endptr != '\0') return -1;

    *value = (int)x;
    return 0;
}

int check_configuration(configuration_flags_t *cft) {
    if(cft->is_server_flag == cft->is_client_flag) {
        printf("Choose -s or -c, not both, not none.\n");
        return -1;
    }

    /* Server parameter checks */

    if(cft->port <= 0 || cft->port > 65535) {
        printf("Enter port number between 0-65535.\n");
        return -1;
    }

    if(cft->time_interval <= 0) {
        printf("Time interval in -i should be above zero.\n");
        return -1;
    }

    /* 
    if the configuration is for the server side, we do not need to check everything else since we do not need it,
    all other options are for the client side. 
    Even the ip address is optional, so we do not care about checking it earlier.
    */
    if(cft->is_server_flag) {
        return 0;
    }

    if(cft->address == NULL) {
        printf("Enter an address after -a.\n");
        return -1;
    }

    if(cft->one_way_delay_flag == 0 && cft->udp_packet_size_in_bytes <= 0) {
        printf("Size in udp packet size -l should be above zero.\n");
        return -1;
    }

    if(cft->one_way_delay_flag == 0 && cft->bandwidth_in_bits_per_sec <= 0) {
        printf("Size in bandwidth of bps -b should be above zero.\n");
        return -1;
    }

    if(cft->parallel_num <= 0) {
        printf("Parallel streams -n should be above zero.\n");
        return -1;
    }

    if(cft->has_time_parameter && cft->time_to_send_in_seconds <= 0) {
        printf("Time in -t should be above zero.\n");
        return -1;
    }

    if(cft->delay_before_starting_in_seconds < 0) {
        printf("Time to wait -w should be zero and above.\n ");
        return -1;
    }
    return 0;
}

int argument_parser(int argc, char *argv[], configuration_flags_t *cft) {
    /* Initializing the configuration struct so it will not be filled with garbage. */
    memset(cft, 0, sizeof(*cft));
    cft->time_interval = 1; /* By default will be every one sec. */

    int opt;
    while((opt = getopt(argc, argv, "sca:p:i:f:l:b:n:t:dw:")) != -1) {
        switch(opt) {
            case 's':
                cft->is_server_flag = 1;
                break;
            case 'c':
                cft->is_client_flag = 1;
                break;
            case 'a':
                cft->address = optarg;
                break;
            case 'p':
                if(char_to_int(optarg, &cft->port) != 0) {
                    printf("Invalid -p option\n");
                    return -1;
                }
                break;
            case 'i':
                if(char_to_int(optarg, &cft->time_interval) != 0) {
                    printf("Invalid -i option\n");
                    return -1;
                }
                break;
            case 'f':
                cft->file = optarg;
                break;
            case 'l':
                if(char_to_int(optarg, &cft->udp_packet_size_in_bytes) != 0) {
                    printf("Invalid -l option\n");
                    return -1;
                }
                break;
            case 'b':
                if(char_to_int(optarg, &cft->bandwidth_in_bits_per_sec) != 0) {
                    printf("Invalid -b option\n");
                    return -1;
                }
                break;
            case 'n':
                if(char_to_int(optarg, &cft->parallel_num) != 0) {
                    printf("Invalid -n option\n");
                    return -1;
                }
                break;
            case 't':
                if(char_to_int(optarg, &cft->time_to_send_in_seconds) != 0) {
                    printf("Invalid -t option\n");
                    return -1;
                }
                cft->has_time_parameter = 1;
                break;
            case 'd':
                cft->one_way_delay_flag = 1;
                break;
            case 'w':
                if(char_to_int(optarg, &cft->delay_before_starting_in_seconds) != 0) {
                    printf("Invalid -w option\n");
                    return -1;
                }
                break;
            default: 
                return -1;
        }
    }

    if(optind < argc) {
        printf("Unknown arguments\n");
        return -1;
    }

    return check_configuration(cft);
}

void print_configuration(configuration_flags_t *cft) {
    printf("__________________________________________________________________\n");
    if(cft->is_server_flag) {
        printf("Configuration set on: SERVER\n");
        
        if(cft->address) {printf("Address to bind: %s\n", cft->address);}
        else {printf("Address to bind: All interfaces\n");}

    } else if (cft->is_client_flag){
        printf("Configuration set on: CLIENT\n");
        printf("Server address: %s\n", cft->address);
        printf("Option set to: ");
        
        if(cft->one_way_delay_flag) {
            printf("one way delay\n");
        } else {
            printf("throughput, jitter, loss\n");
        }

        printf("Parallel stream number: %d\n", cft->parallel_num);
        printf("UDP packet size: %d bytes\n", cft->udp_packet_size_in_bytes);
        printf("Bandwidth: %d bps\n", cft->bandwidth_in_bits_per_sec);
        
        if(cft->has_time_parameter) {
            printf("Duration: %d sec\n", cft->time_to_send_in_seconds);
        } else {
            printf("Duration: continuous\n");
        }

        printf("Delay before start: %d sec\n", cft->delay_before_starting_in_seconds);
    }

    printf("Port: %d\n", cft->port);
    printf("Time interval (seconds): %d\n", cft->time_interval);
    if(cft->file) {
        printf("Output file name: %s\n", cft->file);
    }
    printf("__________________________________________________________________\n");
}

void explain_usage() {
    printf("This program is a network measurement tool working in client/server mode.\n");
    printf("The server waits for connections and the client connects to start an experiment.\n");
    printf("\n");

    printf("How to run it:\n");
    printf("Server mode:\n");
    printf("./netmeasure -s -p <port> [-a <bind_ip>] [-i <interval>] [-f <file>]\n");
    printf("\n");

    printf("How to run it:\n");
    printf("Server mode:\n");
    printf("./netmeasure -s -p <port> [-a <bind_ip>] [-i <interval>] [-f <file>]\n");
    printf("\n");

    printf("Client mode for throughput / jitter / packet loss:\n");
    printf("./netmeasure -c -a <server_ip> -p <port> -l <packet_size> -b <bandwidth> -n <streams>\n");
    printf("[-t <duration>] [-w <wait>] [-i <interval>] [-f <file>]\n");
    printf("\n");

    printf("Client mode for one-way delay:\n");
    printf("./netmeasure -c -d -a <server_ip> -p <port> -n <streams>\n");
    printf("[-t <duration>] [-w <wait>] [-i <interval>] [-f <file>]\n");
    printf("\n");

    printf("Meaning of parameters:\n");
    printf("-s : run the program as server\n");
    printf("-c : run the program as client\n");
    printf("-a : in server mode, local IP to bind to\n");
    printf("     in client mode, IP address of the server to connect to\n");
    printf("-p : TCP port number\n");
    printf("-i : how often to print progress information in seconds\n");
    printf("-f : file where results will be stored\n");
    printf("-l : UDP packet size in bytes\n");
    printf("-b : bandwidth in bits per second\n");
    printf("-n : number of parallel streams\n");
    printf("-t : duration of the experiment in seconds\n");
    printf("     if not given, the client keeps running until stopped by the user\n");
    printf("-d : measure one-way delay instead of throughput/jitter/packet loss\n");
    printf("-w : waiting time before starting transmission\n");
    printf("\n");

    printf("Examples:\n");
    printf("./netmeasure -s -p 5000\n");
    printf("./netmeasure -c -a 127.0.0.1 -p 5000 -l 1000 -b 1000000 -n 1 -t 10\n");
    printf("./netmeasure -c -d -a 127.0.0.1 -p 5000 -n 1 -t 5\n");
}