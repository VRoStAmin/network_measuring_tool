#ifndef UDP_H
#define UDP_H

#include <stddef.h>
#include <stdint.h>

#include "communication.h"
#include "args.h"

int udp_send_test(char *server_ip, int port, int packet_size, int duration_sec);
int udp_recv_test(char *bind_ip, int port);



#endif