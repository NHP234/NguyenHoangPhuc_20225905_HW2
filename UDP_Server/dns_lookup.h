#ifndef DNS_LOOKUP_H
#define DNS_LOOKUP_H

#include "resolver.h"

/**
 * @function: lookup_additional_hostnames
 * @param: primary_host - Primary hostname found from reverse lookup
 * @param: result - Buffer to store additional hostnames
 * @return: void
 */
void lookup_additional_hostnames(const char *primary_host, char *result);

/**
 * @function: forward_lookup
 * @param: hostname - Domain name to resolve
 * @param: result - Buffer to store result
 * @return: void
 */
void forward_lookup(const char *hostname, char *result);

/**
 * @function: reverse_lookup
 * @param: ip - IPv4 address to resolve
 * @param: result - Buffer to store result
 * @return: void
 */
void reverse_lookup(const char *ip, char *result);

#endif /* DNS_LOOKUP_H */

