#include "resolver.h"
#include "validation.h"
#include "dns_lookup.h"

#define MSSV "20225905"

/**
 * @function: write_log
 * @param: request - The request received from client
 * @param: result - The result of DNS lookup
 * @return: void
 */
void write_log(const char *request, const char *result) {
    FILE *log_file;
    char log_filename[50];
    char timestamp[30];
    char log_entry[LOG_BUFFER_SIZE];
    time_t now;
    struct tm *tm_info;
    
    // Create log filename
    sprintf(log_filename, "log_%s.txt", MSSV);
    
    // Get current time
    time(&now);
    tm_info = localtime(&now);
    
    // Format timestamp as dd/mm/yyyy hh:mm:ss
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", tm_info);
    
    // Remove newlines from result for logging
    char result_clean[BUFFER_SIZE];
    strcpy(result_clean, result);
    // Replace newlines with spaces for cleaner log
    for (int i = 0; result_clean[i] != '\0'; i++) {
        if (result_clean[i] == '\n') {
            result_clean[i] = ' ';
        }
    }
    // Remove trailing spaces
    int len = strlen(result_clean);
    while (len > 0 && result_clean[len-1] == ' ') {
        result_clean[--len] = '\0';
    }
    
    // Create log entry
    sprintf(log_entry, "[%s]$%s$%s\n", timestamp, request, result_clean);
    
    // Open log file in append mode
    log_file = fopen(log_filename, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Error: Could not open log file\n");
        return;
    }
    
    // Write log entry
    fprintf(log_file, "%s", log_entry);
    
    fclose(log_file);
}

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
    int len = strlen(result);
    if (len > 0 && result[len-1] == '\n') {
        result[len-1] = '\0';
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
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    char result[BUFFER_SIZE];
    int recv_len;
    
    // Check command line arguments
    if (argc != 2) {
        printf("Usage: %s PortNumber\n", argv[0]);
        return 1;
    }
    
    port = atoi(argv[1]);
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
    
    // Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind socket to address
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error: Bind failed");
        close(sockfd);
        return 1;
    }
    
    printf("Server is running on port %d\n", port);
    printf("Waiting for requests...\n");
    
    // Main server loop - never terminates
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        memset(result, 0, sizeof(result));
        client_len = sizeof(client_addr);
        
        // Receive request from client
        recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                           (struct sockaddr *)&client_addr, &client_len);
        
        if (recv_len < 0) {
            perror("Error: Receive failed");
            continue;  // Don't exit, just continue to next iteration
        }
        
        buffer[recv_len] = '\0';  // Null-terminate the received string
        
        // Remove newline if present
        int len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') {
            buffer[len-1] = '\0';
        }
        
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

