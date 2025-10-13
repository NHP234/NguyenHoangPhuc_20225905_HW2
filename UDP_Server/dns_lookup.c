#include "dns_lookup.h"
#include "utils.h"

/**
 * @function: lookup_additional_hostnames
 * @param: primary_host - Primary hostname found from reverse lookup
 * @param: result - Buffer to store additional hostnames
 * @return: void
 */
void lookup_additional_hostnames(const char *primary_host, char *result) {
    struct addrinfo hints, *res, *p;
    char hostname_buf[MAX_HOSTNAME_LEN];
    char unique_hostnames[MAX_UNIQUE_HOSTNAMES][MAX_HOSTNAME_LEN];
    int unique_count = 0;
    
    result[0] = '\0';  // Initialize empty result
    
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
                        strcat(result, hostname_buf);
                        strcat(result, "\n");
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
 * @param: result - Buffer to store result
 * @return: void
 */
void forward_lookup(const char *hostname, char *result) {
    struct addrinfo hints, *res, *p;
    int status;
    int found = 0;
    char ipstr[INET_ADDRSTRLEN];
    
    result[0] = '\0';  // Initialize empty result
    
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
    if (only_digits_and_dots && dot_count != 3) {
        strcpy(result, "Not found information");
        return;
    }
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    status = getaddrinfo(hostname, NULL, &hints, &res);
    if (status != 0) {
        strcpy(result, "Not found information");
        return;
    }
    
    for (p = res; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            get_ip_address(p, ipstr, sizeof(ipstr));
            strcat(result, ipstr);
            strcat(result, "\n");
            found = 1;
        }
    }
    
    if (!found) {
        strcpy(result, "Not found information");
    }
    
    freeaddrinfo(res);
}

/**
 * @function: reverse_lookup
 * @param: ip - IPv4 address to resolve
 * @param: result - Buffer to store result
 * @return: void
 */
void reverse_lookup(const char *ip, char *result) {
    struct sockaddr_in sa;
    char host[MAX_HOSTNAME_LEN];
    char additional[BUFFER_SIZE];
    int status;
    
    result[0] = '\0';  // Initialize empty result
    
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &(sa.sin_addr));
    
    status = getnameinfo((struct sockaddr *)&sa, sizeof(sa),
                        host, sizeof(host), NULL, 0, NI_NAMEREQD);
    
    if (status != 0) {
        strcpy(result, "Not found information");
        return;
    }
    
    strcpy(result, host);
    strcat(result, "\n");
    
    lookup_additional_hostnames(host, additional);
    strcat(result, additional);
}

