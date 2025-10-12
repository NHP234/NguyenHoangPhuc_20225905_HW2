#include "utils.h"

/**
 * @function: print_ip_address
 * @param: addr_info - Pointer to addrinfo structure containing IPv4 address data
 * @return: void
 */
void print_ip_address(struct addrinfo *addr_info) {
    char ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr_info->ai_addr;
    void *addr = &(ipv4->sin_addr);
    
    inet_ntop(AF_INET, addr, ipstr, sizeof(ipstr));
    printf("%s\n", ipstr);
}

/**
 * @function: is_hostname_in_list
 * @param: hostname - Hostname to check
 * @param: list - Array of hostnames
 * @param: count - Number of hostnames in the list
 * @return: 1 if hostname exists in list, 0 otherwise
 */
int is_hostname_in_list(const char *hostname, char list[][MAX_HOSTNAME_LEN], int count) {
    for (int i = 0; i < count; i++) {
        if (strcmp(hostname, list[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

