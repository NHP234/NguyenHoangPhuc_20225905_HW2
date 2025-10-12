#include "dns_lookup.h"
#include "utils.h"

/**
 * @function: lookup_additional_hostnames
 * @param: primary_host - Primary hostname found from reverse lookup
 * @return: void
 */
void lookup_additional_hostnames(const char *primary_host) {
    struct addrinfo hints, *res, *p;
    char hostname_buf[MAX_HOSTNAME_LEN];
    char unique_hostnames[MAX_UNIQUE_HOSTNAMES][MAX_HOSTNAME_LEN];
    int unique_count = 0;
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(primary_host, NULL, &hints, &res) != 0) {
        return;
    }
    
    /* Query multiple times */
    for (int attempt = 0; attempt < MAX_QUERY_ATTEMPTS; attempt++) {
        for (p = res; p != NULL; p = p->ai_next) {
            if (getnameinfo(p->ai_addr, p->ai_addrlen,
                           hostname_buf, sizeof(hostname_buf),
                           NULL, 0, NI_NAMEREQD) == 0) {
                
                
                if (strcmp(hostname_buf, primary_host) != 0 && 
                    !is_hostname_in_list(hostname_buf, unique_hostnames, unique_count)) {
                    
                    if (unique_count < MAX_UNIQUE_HOSTNAMES) {
                        strcpy(unique_hostnames[unique_count], hostname_buf);
                        unique_count++;
                        printf("%s\n", hostname_buf);
                    }
                }
            }
        }
    }
    
    freeaddrinfo(res);
}


/**
 * @function: forward_lookup
 * @param: hostname - Domain name to resolve
 * @return: void
 */
void forward_lookup(const char *hostname) {
    struct addrinfo hints, *res, *p;
    int status;
    int found = 0;
    
    /* Prevent getaddrinfo from parsing partial IPs like "1", "1.2", "1.2.3" */
    int dot_count = 0;
    int only_digits_and_dots = 1;
    const char *ptr = hostname;
    
    while (*ptr != '\0') {
        if (*ptr == '.') {
            dot_count++;
        } else if (*ptr >= '0' && *ptr <= '9') {
            /* Valid digit, continue */
        } else {
            /* Found a non-digit, non-dot character (letter) */
            only_digits_and_dots = 0;
            break;
        }
        ptr++;
    }
    
    /* If input is numeric-only but not a full IPv4 (needs 3 dots), reject it */
    /* This catches: "1", "1.2", "1.2.3" but allows "1.2.3.4" and "google.com" */
    if (only_digits_and_dots && dot_count != 3) {
        printf("Not found information\n");
        return;
    }
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    status = getaddrinfo(hostname, NULL, &hints, &res);
    if (status != 0) {
        printf("Not found information\n");
        return;
    }
    
    for (p = res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
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
 * @param: ip - IPv4 address to resolve
 * @return: void
 */
void reverse_lookup(const char *ip) {
    struct sockaddr_in sa;
    char host[MAX_HOSTNAME_LEN];
    int status;
    
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    
    if (inet_pton(AF_INET, ip, &(sa.sin_addr)) != 1) {
        printf("Invalid IP address format\n");
        return;
    }
    
    status = getnameinfo((struct sockaddr *)&sa, sizeof(sa),
                        host, sizeof(host), NULL, 0, NI_NAMEREQD);
    
    if (status != 0) {
        printf("Not found information\n");
        return;
    }
    
    printf("%s\n", host);
    
    lookup_additional_hostnames(host);
}

