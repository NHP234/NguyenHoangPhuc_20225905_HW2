#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUFFER_SIZE 2048
#define TIMEOUT_SEC 5

/**
 * @function: trim_newline
 * @param: str - String to trim
 * @return: void
 */
void trim_newline(char *str) {
    int len = strlen(str);
    if (len > 0 && str[len-1] == '\n') {
        str[len-1] = '\0';
    }
}

/**
 * @function: main
 * @param: argc - Number of command line arguments
 * @param: argv - Array of command line argument strings
 * @return: 0 on success, 1 on error
 */
int main(int argc, char *argv[]) {
    int sockfd;
    int port;
    struct sockaddr_in server_addr;
    socklen_t server_len;
    char buffer[BUFFER_SIZE];
    char query[BUFFER_SIZE];
    int send_len, recv_len;
    struct timeval tv;
    
    // Check command line arguments
    if (argc != 3) {
        printf("Usage: %s IPAddress PortNumber\n", argv[0]);
        return 1;
    }
    
    port = atoi(argv[2]);
    if (port <= 0 || port > 65535) {
        printf("Error: Invalid port number\n");
        return 1;
    }
    
    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error: Could not create socket");
        return 1;
    }
    
    // Set socket timeout for receiving
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error: Could not set socket timeout");
        close(sockfd);
        return 1;
    }
    
    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Convert IP address from string to binary form
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        printf("Error: Invalid server IP address\n");
        close(sockfd);
        return 1;
    }
    
    printf("Connected to server %s:%d\n", argv[1], port);
    printf("Enter domain name or IP address (press Enter to quit):\n");
    
    // Main client loop
    while (1) {
        memset(query, 0, sizeof(query));
        memset(buffer, 0, sizeof(buffer));
        
        printf("> ");
        fflush(stdout);
        
        // Read input from user
        if (fgets(query, sizeof(query), stdin) == NULL) {
            break;
        }
        
        trim_newline(query);
        
        // Check for empty string (exit condition)
        if (strlen(query) == 0) {
            printf("Exiting...\n");
            break;
        }
        
        server_len = sizeof(server_addr);
        
        // Send query to server
        send_len = sendto(sockfd, query, strlen(query), 0,
                         (struct sockaddr *)&server_addr, server_len);
        
        if (send_len < 0) {
            perror("Error: Send failed");
            continue;
        }
        
        // Receive response from server
        recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr *)&server_addr, &server_len);
        
        if (recv_len < 0) {
            printf("Error: No response from server (timeout)\n");
            continue;
        }
        
        buffer[recv_len] = '\0';  // Null-terminate the received string
        
        // Display result
        printf("%s\n", buffer);
    }
    
    close(sockfd);
    return 0;
}

