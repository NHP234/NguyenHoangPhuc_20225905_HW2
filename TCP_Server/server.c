#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include "logger.h"

#define BUFF_SIZE 65536  // 64KB buffer 
#define BACKLOG 10

char storage_dir[256];

/**
* @function create_directory: Create directory if it doesn't exist
* @param path: Path to the directory to create
* @return: 0 on success, -1 on failure
**/
int create_directory(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0755) == -1) {
            perror("Failed to create directory");
            return -1;
        }
    }
    return 0;
}

/**
* @function get_client_addr_str: Get client address as string in format IP:Port
* @param client_addr: Pointer to client address structure
* @param addr_str: Buffer to store the address string
* @param len: Length of the buffer
**/
void get_client_addr_str(struct sockaddr_in *client_addr, char *addr_str, size_t len) {
    snprintf(addr_str, len, "%s:%d", inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port));
}

/**
* @function send_all: Send all data through socket (handle partial sends)
* @param sockfd: Socket file descriptor
* @param buf: Buffer containing data to send
* @param len: Length of data to send
* @return: 0 on success, -1 on failure
**/
int send_all(int sockfd, const char *buf, int len) {
    int total = 0;
    int bytesleft = len;
    int n;

    while (total < len) {
        n = send(sockfd, buf + total, bytesleft, 0);
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }

    return (n == -1) ? -1 : 0;
}

/**
* @function tcp_recv: Receive a line until \r\n (handle TCP stream properly)
* @param sockfd: Socket file descriptor
* @param buffer: Buffer to store received line
* @param max_len: Maximum buffer size
* @return: Length of received line (excluding \r\n), -1 on error, 0 on disconnect
**/
int tcp_recv(int sockfd, char *buffer, int max_len) {
    char recv_buf[BUFF_SIZE];
    int total_len = 0;
    
    buffer[0] = '\0';
    
    while (total_len < max_len - 1) {
        char *newline = strstr(buffer, "\r\n");
        if (newline != NULL) {
            *newline = '\0';
            return strlen(buffer);
        }
        
        int n = recv(sockfd, recv_buf, BUFF_SIZE - 1, 0);
        
        if (n <= 0) {
            if (n == 0 && total_len > 0) {
                return total_len;
            }
            return n;
        }
        
        recv_buf[n] = '\0';
        strcat(buffer, recv_buf);
        total_len += n;
    }
    
    return total_len;
}

/**
* @function handle_client: Handle client connection and file upload requests
* @param client_sock: Socket file descriptor for the client connection
* @param client_addr: Client address structure
**/
void handle_client(int client_sock, struct sockaddr_in client_addr) {
    char addr_str[64];
    char buffer[BUFF_SIZE];
    char welcome_msg[] = "+OK Welcome to file server\r\n";
    char ok_send_msg[] = "+OK Please send file\r\n";
    char ok_success_msg[] = "+OK Successful upload\r\n";
    char err_msg[] = "-ERR Failed to receive file\r\n";
    
    get_client_addr_str(&client_addr, addr_str, sizeof(addr_str));
    
    printf("New client connected: %s\n", addr_str);
    
    // Send welcome message
    if (send_all(client_sock, welcome_msg, strlen(welcome_msg)) == -1) {
        perror("Failed to send welcome message");
        log_activity(addr_str, "", "-ERR Failed to send welcome");
        close(client_sock);
        return;
    }
    log_activity(addr_str, "", welcome_msg);
    
    // Main loop to handle multiple file uploads from same client
    while (1) {
        // Receive UPLD command using tcp_recv (handles stream properly)
        memset(buffer, 0, BUFF_SIZE);
        int bytes_received = tcp_recv(client_sock, buffer, BUFF_SIZE);
        
        if (bytes_received < 0) {
            perror("Failed to receive command");
            break;
        }
        
        if (bytes_received == 0) {
            printf("Client %s disconnected\n", addr_str);
            break;
        }
        
        // Check if client wants to disconnect (empty message)
        if (strlen(buffer) == 0) {
            printf("Client %s requested disconnect\n", addr_str);
            break;
        }
        
        // Parse UPLD command
        char cmd[10], filename[256];
        uint32_t filesize;
        char upld_command[BUFF_SIZE];
        
        int parsed = sscanf(buffer, "%s %s %u", cmd, filename, &filesize);
        
        if (parsed != 3 || strcmp(cmd, "UPLD") != 0) {
            log_activity(addr_str, buffer, "-ERR Invalid command");
            send_all(client_sock, "-ERR Invalid command\n", 21);
            continue;
        }
        
        // Save UPLD command for logging
        strncpy(upld_command, buffer, sizeof(upld_command) - 1);
        upld_command[sizeof(upld_command) - 1] = '\0';
        
        printf("Received: %s\n", upld_command);
        
        // Send acknowledgment
        if (send_all(client_sock, ok_send_msg, strlen(ok_send_msg)) == -1) {
            perror("Failed to send OK message");
            log_activity(addr_str, upld_command, "-ERR Failed to send OK");
            break;
        }
        log_activity(addr_str, upld_command, ok_send_msg);
        
        // Prepare file path
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", storage_dir, filename);
        
        // Open file for writing
        FILE *fp = fopen(filepath, "wb");
        if (fp == NULL) {
            perror("Failed to open file for writing");
            log_activity(addr_str, upld_command, "-ERR Failed to open file");
            send_all(client_sock, err_msg, strlen(err_msg));
            continue;
        }
        
        // Receive file data
        uint32_t bytes_left = filesize;
        uint32_t total_received = 0;
        int success = 1;
        
        while (bytes_left > 0) {
            int to_receive = (bytes_left < BUFF_SIZE) ? bytes_left : BUFF_SIZE;
            int received = recv(client_sock, buffer, to_receive, 0);
            
            if (received <= 0) {
                if (received == 0) {
                    printf("Connection closed during file transfer\n");
                } else {
                    perror("recv failed during file transfer");
                }
                success = 0;
                break;
            }
            
            // Write to file
            size_t written = fwrite(buffer, 1, received, fp);
            if (written != received) {
                printf("Failed to write to file\n");
                success = 0;
                break;
            }
            
            total_received += received;
            bytes_left -= received;
        }
        
        fclose(fp);
        
        if (success && total_received == filesize) {
            printf("File %s (%u bytes) received successfully from %s\n", 
                   filename, filesize, addr_str);
            log_activity(addr_str, upld_command, ok_success_msg);
            send_all(client_sock, ok_success_msg, strlen(ok_success_msg));
        } else {
            printf("File transfer failed. Expected %u bytes, got %u bytes\n", 
                   filesize, total_received);
            log_activity(addr_str, upld_command, err_msg);
            send_all(client_sock, err_msg, strlen(err_msg));
            // Delete incomplete file
            remove(filepath);
        }
    }
    
    close(client_sock);
    printf("Connection with %s closed\n", addr_str);
}

/**
* @function main: Server main function to setup socket and accept connections
* @param argc: Number of command line arguments
* @param argv: Array of command line arguments
* @return: 0 on success, 1 on failure
**/
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s Port_Number Directory_name\n", argv[0]);
        printf("Example: %s 5500 storage\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    strncpy(storage_dir, argv[2], sizeof(storage_dir) - 1);
    storage_dir[sizeof(storage_dir) - 1] = '\0';
    
    if (port <= 0 || port > 65535) {
        printf("Invalid port number\n");
        return 1;
    }
    
    
    if (create_directory(storage_dir) == -1) {
        return 1;
    }
    
    
    if (logger_init() == -1) {
        return 1;
    }
    
    
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Failed to create socket");
        logger_close();
        return 1;
    }
    
    //Can restart server immediately
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(server_sock);
        logger_close();
        return 1;
    }
    
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        close(server_sock);
        logger_close();
        return 1;
    }
    
    
    if (listen(server_sock, BACKLOG) == -1) {
        perror("Listen failed");
        close(server_sock);
        logger_close();
        return 1;
    }
    
    printf("Server started on port %d\n", port);
    printf("Storage directory: %s\n", storage_dir);
    printf("Waiting for connections...\n");
    
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_sock == -1) {
            perror("Accept failed");
            continue;  
        }
        
        handle_client(client_sock, client_addr);
    }
    
    close(server_sock);
    logger_close();
    
    return 0;
}

