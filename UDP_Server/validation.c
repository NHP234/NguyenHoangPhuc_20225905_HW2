#include "validation.h"

/**
 * @function: is_valid_ipv4
 * @param: str - String to validate as IPv4 address
 * @return: 1 if valid IPv4 address, 0 otherwise
 */
int is_valid_ipv4(const char *str) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, str, &(sa.sin_addr)) == 1;
}

