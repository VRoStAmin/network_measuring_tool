#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "server.h"
#include "client.h"

int main(int argc, char *argv[]) {
    configuration_flags_t cft;
    if(argument_parser(argc, argv, &cft) != 0) {
        explain_usage();
        return 1;
    }

    printf("Printing configuration for debugging purposes...\n");
    print_configuration(&cft);

    if(cft.is_server_flag) {
        return run_server(&cft);
    } else if(cft.is_client_flag) {
        return run_client(&cft);
    }

    explain_usage();
    return 0;
}