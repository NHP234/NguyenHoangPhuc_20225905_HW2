#include "resolver.h"
#include "validation.h"
#include "dns_lookup.h"
#include "utils.h"
#include "logger.h"

/**
 * @function: process_request
 * @param: request - The request string from client
 * @param: result - Buffer to store result
 * @return: void
 */
void process_request(const char *request, char *result) {
    if (is_valid_ipv4(request)) {
        reverse_lookup(request, result);
    } else {
        forward_lookup(request, result);
    }
    
    // Remove trailing newline if exists
    trim_newline(result);
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
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    int recv_len;
    
    
    if (argc != 2) {
        printf("Usage: %s PortNumber\n", argv[0]);
        return 1;
    }
    
    port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        printf("Error: Invalid port number\n");
        return 1;
    }
    
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error: Could not create socket");
        return 1;
    }
    
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: Bind failed");
        close(sockfd);
        return 1;
    }
    
    printf("Server is running on port %d\n", port);
    printf("Waiting for requests...\n");
    
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        memset(result, 0, sizeof(result));
        client_len = sizeof(client_addr);
        
        // Receive request from client
        recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr *)&client_addr, &client_len);
        
        if (recv_len < 0) {
            perror("Error: Receive failed");
            continue;  
        }
        
        buffer[recv_len] = '\0';  
        
        // Get client IP and port for logging
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        int client_port = ntohs(client_addr.sin_port);
        
        printf("Received request from %s:%d - Query: %s\n", 
               client_ip, client_port, buffer);
        
        // Process the request
        process_request(buffer, result);
        
        printf("Result: %s\n\n", result);
        
        // Write to log file
        write_log(buffer, result);
        
        // Send result back to client
        if (sendto(sockfd, result, strlen(result), 0,
                  (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("Error: Send failed");
            continue;  // Don't exit, just continue to next iteration
        }
    }
    
    // This code will never be reached, but included for completeness
    close(sockfd);
    return 0;
}

