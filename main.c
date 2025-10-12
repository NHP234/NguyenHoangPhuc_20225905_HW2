#include "resolver.h"
#include "validation.h"
#include "dns_lookup.h"

/**
 * @function: main
 * @param: argc - Number of command line arguments
 * @param: argv - Array of command line argument strings
 * @return: 0 on success, 1 on usage error
 */
int main(int argc, char *argv[]) {
    char *parameter;
    
    if (argc != 2) {
        printf("Usage: %s <hostname|ip_address>\n", argv[0]);
        return 1;
    }
    
    parameter = argv[1];
    
    if (is_valid_ipv4(parameter)) {
        reverse_lookup(parameter);
    } else {
        forward_lookup(parameter);
    }
    
    return 0;
}

