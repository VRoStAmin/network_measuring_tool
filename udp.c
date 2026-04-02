#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include "udp.h"

#define UDP_HEADER_SIZE 8
#define IP_HEADER_SIZE 20
#define ETHERNET_HEADER_SIZE 14

#define OVERHEAD_SIZE (UDP_HEADER_SIZE + IP_HEADER_SIZE + ETHERNET_HEADER_SIZE)

uint64_t nanosec_now() {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ULL + (uint64_t)t.tv_nsec;
}

void *udp_client_thread_main(void *arg) {
    udp_client_thread_t *ct = (udp_client_thread_t *)arg;
    int result = udp_client_experiment(ct->server_ip, ct->port, ct->packet_size, ct->duration_sec, ct->bandwidth_bps, ct->one_way_delay_flag, &ct->last_seq_sent, ct->stop, ct);
    ct->status = result;
    return NULL;
}

void *udp_server_thread_main(void *arg) {
    udp_server_thread_t *st = (udp_server_thread_t *)arg;
    int result = udp_server_experiment(st->bind_ip, st->port, st->packet_size, st->one_way_delay_flag, st->duration_sec, st->stop, st);
    st->status = result;
    return NULL;
}

int udp_client_experiment(char *server_ip, int port, uint32_t packet_size, int duration_sec, uint64_t bandwidth_bps, int one_way_delay_flag, uint64_t *last_seq_sent, volatile int *stop, udp_client_thread_t *ct) {
    int client_sock;
    char *buffer;
    struct sockaddr_in server_address;
    uint64_t seq_num;

    /*Create UDP socket*/
    client_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_sock < 0) {
        printf("socket creation failed\n");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    
    if(inet_pton(AF_INET, server_ip, &server_address.sin_addr) <= 0){
        printf("Server address error [UDP PART]\n ");
        close(client_sock);
        return -1;
    } 

    if(one_way_delay_flag) {
        ct->one_way_delay_flag = 1;
        udp_pseudo_header_t send_pack;
        udp_pseudo_header_t received_pack;
        seq_num = 0;

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        uint64_t start_ns = nanosec_now();
        uint64_t end_ns = start_ns + (uint64_t)duration_sec * 1000000000ULL;
        printf("UDP delay mode client started for %d seconds\n", duration_sec);
        
        double sum_delay_ns = 0;
        uint64_t packets_received = 0;
        while(nanosec_now() < end_ns && !*stop) {
            send_pack.sequence_number = seq_num;
            send_pack.time_sent_ns = nanosec_now();
            ssize_t bytes_sent = sendto(client_sock, &send_pack, sizeof(send_pack), 0, (struct sockaddr *)&server_address, sizeof(server_address));
            if(bytes_sent < 0) {
                printf("Error in send to\n");
                close(client_sock);
                return -1; 
            } 

            ssize_t bytes_recv = recvfrom(client_sock, &received_pack, sizeof(received_pack), 0, NULL, NULL);
            if(bytes_recv < 0) {
                seq_num++;
                continue;
            }

            uint64_t recv_time_ns = nanosec_now();
            uint64_t rtt_ns = recv_time_ns - received_pack.time_sent_ns;
            double one_way_delay_ns = rtt_ns/2.0;
            sum_delay_ns += one_way_delay_ns;
            packets_received++;
            seq_num++;
        }   
        if(packets_received > 0) {
            double avg_delay_ns = sum_delay_ns / (double)packets_received;
            ct->results.one_way_delay = avg_delay_ns;
            printf("Average one way delay: %.3f ns\n", avg_delay_ns);
        } else {
            printf("No packets received to compute one way delay.\n");
        }

        if(seq_num == 0) *last_seq_sent = 0;
        else *last_seq_sent = seq_num - 1;
        
        close(client_sock);
        return 0;
    } else {
        ct->one_way_delay_flag = 0;
        uint64_t full_packet_size = packet_size + sizeof(udp_pseudo_header_t);
        uint64_t overhead_packet_size = full_packet_size + OVERHEAD_SIZE;

        seq_num = 0;
        buffer = malloc(full_packet_size * sizeof(char));
        if(buffer == NULL) {
            printf("Malloc failure\n");
            close(client_sock);
            return -1;
        }

        memset(buffer, 0, full_packet_size);
        
        uint64_t time_now_ns = nanosec_now();
        uint64_t time_end_ns = time_now_ns + (uint64_t)duration_sec * 1000000000ULL;
        double delay_sec = (overhead_packet_size * 8ULL) / (double)bandwidth_bps;
        uint64_t delay_ns = delay_sec * 1000000000ULL;
        //maybe we need to check if the delay_ns is zero due to really small overhead_packet_size
        uint64_t next_send_time = time_now_ns;

        while(nanosec_now() < time_end_ns && !*stop) {
            uint64_t now = nanosec_now();
            if(now > time_end_ns && !*stop) {
                break;
            }

            while(nanosec_now() < next_send_time && !*stop) {
                continue;
            }

            if (*stop) {
                break;
            }

            udp_pseudo_header_t header;
            header.sequence_number = seq_num;
            header.time_sent_ns = nanosec_now();
            

            memcpy(buffer, &header, sizeof(header));
            ssize_t bytes_sent = sendto(client_sock, buffer, full_packet_size, 0, (struct sockaddr *)&server_address, sizeof(server_address));
            if(bytes_sent < 0) {
                printf("Error in send to\n");
                close(client_sock);
                free(buffer);
                return -1; 
            }
            printf("Sent UDP packet with sequence number: %llu\n", seq_num);
            seq_num++;
            next_send_time += delay_ns;
        }
        printf("UDP send test finished, total packets sent: %llu\n", seq_num);
        
        if(seq_num == 0) *last_seq_sent = 0;
        else *last_seq_sent = seq_num - 1;
        
        close(client_sock);
        free(buffer);
        return 0;
    }
    return 0;
}

int udp_server_experiment(char *bind_ip, int port, uint32_t packet_size, int one_way_delay_flag, int duration_sec, volatile int *stop, udp_server_thread_t *st) {
    int server_sock;
    struct sockaddr_in client_address;
    struct sockaddr_in server_address;
    socklen_t client_addr_len = sizeof(client_address);
    char *buffer;

    uint64_t total_packets; 
    uint64_t total_bytes;
    
    /*for the lost packets*/
    uint64_t max_seq_packet = -1;
    uint64_t total_lost_packets = 0;

    /*for the average jitter calculation and standard deviation*/
    uint64_t prev_arrival_time_ns = 0;
    uint64_t prev_inter_arrival_time_ns = 0;
    uint64_t jitter_sum = 0;
    uint64_t jitter_count = 0;

    double avg_jitter_ns = 0.0;
    double jitter_sqr_sum = 0.0;
    double std_jitter_ns = 0.0;

    exp_results_t results;
    
    server_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(server_sock < 0) {
        printf("Socket creation failed\n");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    memset(&client_address, 0, sizeof(client_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    
    if(bind_ip == NULL) {
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if(inet_pton(AF_INET, bind_ip, &server_address.sin_addr) <= 0) {
            printf("Server address error\n");
            close(server_sock);
            return -1;
        }
    }
    
    if(bind(server_sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Bind error\n");
        close(server_sock);
        return -1;    
    }
    
    total_bytes = 0;
    total_packets = 0;
    size_t full_packet_size = packet_size + sizeof(udp_pseudo_header_t);

    buffer = malloc((packet_size + 1024 ) * sizeof(char));
    if (buffer == NULL) {
        printf("Malloc failure\n");
        close(server_sock);
        return -1;
    }
    memset(buffer, 0, (packet_size + 1024) * sizeof(char));
    printf("Server listening on port %d\n", port);

    int has_started = 0;
    uint64_t time_end_ns = 0;
    uint64_t time_now_ns = 0;
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 200000;
    setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while(!*stop) {
        ssize_t bytes_recv = recvfrom(server_sock, buffer, full_packet_size, 0, (struct sockaddr *)&client_address, &client_addr_len);

        if (bytes_recv < 0) {
            continue;
        }
        
        if(!has_started) {
            has_started = 1; 
            time_now_ns = nanosec_now();
            time_end_ns = time_now_ns + (uint64_t)duration_sec * 1000000000ULL;
        }

        //printf("Received another package\n");
        if(bytes_recv >= sizeof(udp_pseudo_header_t)) {
            udp_pseudo_header_t header;
            memcpy(&header, buffer, sizeof(header));
            if(one_way_delay_flag){
                sendto(server_sock, buffer, bytes_recv, 0, (struct sockaddr *)&client_address, client_addr_len);
            }else {
                /*jitter calculation*/
                uint64_t now = nanosec_now();
                if(prev_arrival_time_ns != 0){
                    uint64_t inter_arrival_time_ns = now - prev_arrival_time_ns;
                    if(prev_inter_arrival_time_ns != 0){
                        uint64_t jitter_ns = 0;
                        if(inter_arrival_time_ns > prev_inter_arrival_time_ns){
                            jitter_ns = inter_arrival_time_ns - prev_inter_arrival_time_ns;
                        } else {
                            jitter_ns = prev_inter_arrival_time_ns - inter_arrival_time_ns;
                        }
                        jitter_sum += jitter_ns;
                        jitter_count++;
                        jitter_sqr_sum += ((double)jitter_ns * (double)jitter_ns);
                    }
                    prev_inter_arrival_time_ns = inter_arrival_time_ns;
                }
                prev_arrival_time_ns = now;      
            }
            total_packets++;
            total_bytes += (uint64_t)bytes_recv;
            if(header.sequence_number > max_seq_packet){
                max_seq_packet = header.sequence_number;
            }
            //printf("Received packet seq=%llu\n", header.sequence_number);
        } else {
            printf("Received packet too small: %zu bytes\n", bytes_recv);
        }
    }
    
    
    if(!one_way_delay_flag) {
        total_lost_packets = max_seq_packet + 1 - total_packets;
        st->results.throughput_bps = calculate_throughput(total_bytes, duration_sec, total_packets);
        st->results.goodput_bps = calculate_goodput(total_bytes, duration_sec, total_packets);

        if(jitter_count > 0){
            avg_jitter_ns = (double)jitter_sum / jitter_count;

            double mean_sqr_jitter = jitter_sqr_sum / jitter_count;
            double mean = avg_jitter_ns;
            double variance = mean_sqr_jitter - (mean * mean);
            if(variance < 0) variance = 0;
            std_jitter_ns = sqrt(variance);
        }
        printf("Packets received: %llu\n", total_packets);
        printf("Bytes received: %llu\n", total_bytes);
        printf("Throughput: %.3f bps\n", st->results.throughput_bps);
        printf("Goodput: %.3f bps\n", st->results.goodput_bps);
        printf("Packet loss percent: %.2f%%\n", (total_lost_packets / (double)(total_packets + total_lost_packets)) * 100.0);
        printf("Average jitter: %.3f ns\n", avg_jitter_ns);
        printf("Standard deviation of jitter: %.3f ns\n", std_jitter_ns);

        st->results.loss_percent = (total_lost_packets / (double)(total_packets + total_lost_packets)) * 100.0;
        st->results.avg_jitter_ns = avg_jitter_ns;
        st->results.std_jitter = std_jitter_ns;
    }

    close(server_sock);
    free(buffer);
    return 0;
}

/*calculate throughput*/
double calculate_throughput(uint64_t total_bytes, double duration_sec, uint64_t total_packets){
    double true_total_bytes = total_bytes - (total_packets * sizeof(udp_pseudo_header_t));
    double throughput_bps = (8.0 * (true_total_bytes + (total_packets * OVERHEAD_SIZE))) / duration_sec;
    return throughput_bps;
}

double calculate_goodput(uint64_t total_bytes, double duration_sec, uint64_t total_packets){
    double true_total_bytes = total_bytes - (total_packets * sizeof(udp_pseudo_header_t));
    double goodput_bps = (8.0 * true_total_bytes) / duration_sec;
    return goodput_bps;
}
