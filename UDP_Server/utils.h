#ifndef UTILS_H
#define UTILS_H

#include "resolver.h"

/**
 * @function: get_ip_address
 * @param: addr_info - Pointer to addrinfo structure containing address data
 * @param: ipstr - Buffer to store IP address string
 * @param: size - Size of the buffer
 * @return: void
 */
void get_ip_address(struct addrinfo *addr_info, char *ipstr, size_t size);

/**
 * @function: is_hostname_in_list
 * @param: hostname - Hostname to check
 * @param: list - Array of hostnames
 * @param: count - Number of hostnames in the list
 * @return: 1 if hostname exists in list, 0 otherwise
 */
int is_hostname_in_list(const char *hostname, char list[][MAX_HOSTNAME_LEN], int count);

#endif /* UTILS_H */

