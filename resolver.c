#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_HOSTNAME_LEN 256

// Hàm kiểm tra xem chuỗi có phải là địa chỉ IPv4 hợp lệ không
int is_valid_ipv4(const char *str) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, str, &(sa.sin_addr)) == 1;
}

// Hàm thực hiện phân giải thuận (domain -> IP)
void forward_lookup(const char *hostname) {
    struct addrinfo hints, *res, *p;
    int status;
    char ipstr[INET6_ADDRSTRLEN];
    int found = 0;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // AF_INET hoặc AF_INET6
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
        printf("Not found information\n");
        return;
    }

    printf("Domain name: %s\n", hostname);
    printf("IP addresses:\n");

    for (p = res; p != NULL; p = p->ai_next) {
        void *addr;
        
        if (p->ai_family == AF_INET) { // IPv4
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
            printf("  %s\n", ipstr);
            found = 1;
        } else if (p->ai_family == AF_INET6) { // IPv6
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
            printf("  %s\n", ipstr);
            found = 1;
        }
    }

    if (!found) {
        printf("Not found information\n");
    }

    freeaddrinfo(res);
}

// Hàm thực hiện phân giải nghịch (IP -> domain)
void reverse_lookup(const char *ip) {
    struct sockaddr_in sa;
    struct sockaddr_in6 sa6;
    char host[MAX_HOSTNAME_LEN];
    int status;
    
    // Thử IPv4
    memset(&sa, 0, sizeof sa);
    sa.sin_family = AF_INET;
    
    if (inet_pton(AF_INET, ip, &(sa.sin_addr)) == 1) {
        status = getnameinfo((struct sockaddr *)&sa, sizeof sa,
                           host, sizeof host, NULL, 0, 0);
        
        if (status == 0) {
            printf("IP address: %s\n", ip);
            printf("Official hostname: %s\n", host);
            
            // Thử tìm thêm các hostname khác bằng cách phân giải thuận
            struct addrinfo hints, *res, *p;
            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            
            if (getaddrinfo(host, NULL, &hints, &res) == 0) {
                printf("Additional domain names:\n");
                
                // Lấy tất cả các canonical names
                for (p = res; p != NULL; p = p->ai_next) {
                    char hostname_buf[MAX_HOSTNAME_LEN];
                    if (getnameinfo(p->ai_addr, p->ai_addrlen,
                                  hostname_buf, sizeof hostname_buf,
                                  NULL, 0, NI_NAMEREQD) == 0) {
                        if (strcmp(hostname_buf, host) != 0) {
                            printf("  %s\n", hostname_buf);
                        }
                    }
                }
                freeaddrinfo(res);
            }
        } else {
            printf("Not found information\n");
        }
        return;
    }
    
    // Thử IPv6
    memset(&sa6, 0, sizeof sa6);
    sa6.sin6_family = AF_INET6;
    
    if (inet_pton(AF_INET6, ip, &(sa6.sin6_addr)) == 1) {
        status = getnameinfo((struct sockaddr *)&sa6, sizeof sa6,
                           host, sizeof host, NULL, 0, 0);
        
        if (status == 0) {
            printf("IP address: %s\n", ip);
            printf("Official hostname: %s\n", host);
        } else {
            printf("Not found information\n");
        }
        return;
    }
    
    printf("Invalid IP address format\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <domain_name|IP_address>\n", argv[0]);
        printf("Examples:\n");
        printf("  %s google.com\n", argv[0]);
        printf("  %s 8.8.8.8\n", argv[0]);
        return 1;
    }

    char *parameter = argv[1];

    // Kiểm tra xem parameter là địa chỉ IP hay tên miền
    if (is_valid_ipv4(parameter)) {
        // Phân giải nghịch: IP -> Domain
        reverse_lookup(parameter);
    } else {
        // Phân giải thuận: Domain -> IP
        forward_lookup(parameter);
    }

    return 0;
}

