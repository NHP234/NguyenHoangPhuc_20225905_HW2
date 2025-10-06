/*
 * DNS Resolver Application
 * File: resolver.c
 * Description: Command-line tool for DNS lookups (forward and reverse)
 * 
 * Note: The "Additional domain names" feature may show inconsistent results
 * due to DNS round-robin, load balancing, and caching mechanisms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/* Constants */
#define MAX_HOSTNAME_LEN 256

/* ========================================================================== */
/*                         VALIDATION FUNCTIONS                               */
/* ========================================================================== */

/**
 * @function: is_valid_ipv4
 * @param: str - String to validate as IPv4 address
 * @return: 1 if valid IPv4 address, 0 otherwise
 */
int is_valid_ipv4(const char *str) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, str, &(sa.sin_addr)) == 1;
}

/* ========================================================================== */
/*                           HELPER FUNCTIONS                                 */
/* ========================================================================== */

/**
 * @function: print_ip_address
 * @param: addr_info - Pointer to addrinfo structure containing address data
 * @return: void
 */
void print_ip_address(struct addrinfo *addr_info) {
    char ipstr[INET6_ADDRSTRLEN];
    void *addr;
    
    if (addr_info->ai_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr_info->ai_addr;
        addr = &(ipv4->sin_addr);
    } else {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr_info->ai_addr;
        addr = &(ipv6->sin6_addr);
    }
    
    inet_ntop(addr_info->ai_family, addr, ipstr, sizeof(ipstr));
    printf("  %s\n", ipstr);
}

/**
 * @function: lookup_additional_hostnames
 * @param: primary_host - Primary hostname found from reverse lookup
 * @return: void
 * 
 * Attempts to find additional domain names for the same IP address.
 * Note: Results may be inconsistent due to DNS round-robin and caching.
 */
void lookup_additional_hostnames(const char *primary_host) {
    struct addrinfo hints, *res, *p;
    char hostname_buf[MAX_HOSTNAME_LEN];
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(primary_host, NULL, &hints, &res) != 0) {
        return;
    }
    
    printf("Additional domain names:\n");
    
    for (p = res; p != NULL; p = p->ai_next) {
        if (getnameinfo(p->ai_addr, p->ai_addrlen,
                       hostname_buf, sizeof(hostname_buf),
                       NULL, 0, NI_NAMEREQD) == 0) {
            if (strcmp(hostname_buf, primary_host) != 0) {
                printf("  %s\n", hostname_buf);
            }
        }
    }
    
    freeaddrinfo(res);
}

/**
 * @function: perform_reverse_lookup_ipv4
 * @param: ip - IPv4 address string to resolve
 * @return: 0 on success, -1 on failure
 */
int perform_reverse_lookup_ipv4(const char *ip) {
    struct sockaddr_in sa;
    char host[MAX_HOSTNAME_LEN];
    int status;
    
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    
    if (inet_pton(AF_INET, ip, &(sa.sin_addr)) != 1) {
        return -1;
    }
    
    status = getnameinfo((struct sockaddr *)&sa, sizeof(sa),
                        host, sizeof(host), NULL, 0, 0);
    
    if (status != 0) {
        printf("Not found information\n");
        return -1;
    }
    
    printf("IP address: %s\n", ip);
    printf("Official hostname: %s\n", host);
    
    lookup_additional_hostnames(host);
    
    return 0;
}

/**
 * @function: perform_reverse_lookup_ipv6
 * @param: ip - IPv6 address string to resolve
 * @return: 0 on success, -1 on failure
 */
int perform_reverse_lookup_ipv6(const char *ip) {
    struct sockaddr_in6 sa6;
    char host[MAX_HOSTNAME_LEN];
    int status;
    
    memset(&sa6, 0, sizeof(sa6));
    sa6.sin6_family = AF_INET6;
    
    if (inet_pton(AF_INET6, ip, &(sa6.sin6_addr)) != 1) {
        return -1;
    }
    
    status = getnameinfo((struct sockaddr *)&sa6, sizeof(sa6),
                        host, sizeof(host), NULL, 0, 0);
    
    if (status != 0) {
        printf("Not found information\n");
        return -1;
    }
    
    printf("IP address: %s\n", ip);
    printf("Official hostname: %s\n", host);
    
    return 0;
}

/**
 * @function: print_usage
 * @param: program_name - Name of the program (argv[0])
 * @return: void
 */
void print_usage(const char *program_name) {
    printf("Usage: %s <domain_name|IP_address>\n", program_name);
    printf("Examples:\n");
    printf("  %s google.com\n", program_name);
    printf("  %s 8.8.8.8\n", program_name);
}

/* ========================================================================== */
/*                        CORE LOOKUP FUNCTIONS                               */
/* ========================================================================== */

/**
 * @function: forward_lookup
 * @param: hostname - Domain name to resolve
 * @return: void
 */
void forward_lookup(const char *hostname) {
    struct addrinfo hints, *res, *p;
    int status;
    int found = 0;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    status = getaddrinfo(hostname, NULL, &hints, &res);
    if (status != 0) {
        printf("Not found information\n");
        return;
    }
    
    printf("Domain name: %s\n", hostname);
    printf("IP addresses:\n");
    
    for (p = res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET || p->ai_family == AF_INET6) {
            print_ip_address(p);
            found = 1;
        }
    }
    
    if (!found) {
        printf("Not found information\n");
    }
    
    freeaddrinfo(res);
}

/**
 * @function: reverse_lookup
 * @param: ip - IP address to resolve (supports both IPv4 and IPv6)
 * @return: void
 */
void reverse_lookup(const char *ip) {
    /* Try IPv4 first */
    if (perform_reverse_lookup_ipv4(ip) == 0) {
        return;
    }
    
    /* Try IPv6 if IPv4 failed */
    if (perform_reverse_lookup_ipv6(ip) == 0) {
        return;
    }
    
    /* Both failed */
    printf("Invalid IP address format\n");
}

/* ========================================================================== */
/*                             MAIN FUNCTION                                  */
/* ========================================================================== */

/**
 * @function: main
 * @param: argc - Number of command line arguments
 * @param: argv - Array of command line argument strings
 * @return: 0 on success, 1 on usage error
 */
int main(int argc, char *argv[]) {
    char *parameter;
    
    if (argc != 2) {
        print_usage(argv[0]);
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
