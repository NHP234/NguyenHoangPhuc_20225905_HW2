#ifndef DNS_LOOKUP_H
#define DNS_LOOKUP_H

#include "resolver.h"

/**
 * @function: lookup_additional_hostnames
 * @param: primary_host - Primary hostname found from reverse lookup
 * @return: void
 */
void lookup_additional_hostnames(const char *primary_host);

/**
 * @function: forward_lookup
 * @param: hostname - Domain name to resolve
 * @return: void
 */
void forward_lookup(const char *hostname);

/**
 * @function: reverse_lookup
 * @param: ip - IPv4 address to resolve
 * @return: void
 */
void reverse_lookup(const char *ip);

#endif /* DNS_LOOKUP_H */

