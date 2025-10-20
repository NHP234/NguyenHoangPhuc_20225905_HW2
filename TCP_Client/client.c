#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <stdint.h>

#define BUFF_SIZE 65536  // 64KB buffer 

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
* @function get_file_size: Get file size in bytes
* @param filename: Path to the file
* @return: File size in bytes, or -1 on error
**/
long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

/**
* @function get_filename_from_path: Extract filename from full path
* @param path: Full path to the file
* @param filename: Buffer to store the extracted filename
* @param len: Length of the buffer
**/
void get_filename_from_path(const char *path, char *filename, size_t len) {
    const char *last_slash = strrchr(path, '/');
    const char *basename = path;
    
    if (last_slash != NULL) {
        basename = last_slash + 1;
    }
    
    strncpy(filename, basename, len - 1);
    filename[len - 1] = '\0';
}

/**
* @function upload_file: Upload a file to the server
* @param sockfd: Socket file descriptor
* @param filepath: Path to the file to upload
* @return: 0 on success, -1 on failure
**/
int upload_file(int sockfd, const char *filepath) {
    char buffer[BUFF_SIZE];
    char filename[256];
    char response[BUFF_SIZE];
    
    long filesize = get_file_size(filepath);
    if (filesize < 0) {
        printf("Error: Cannot access file '%s'\n", filepath);
        return -1;
    }
    
    if (filesize > 0xFFFFFFFFUL) {
        printf("Error: File size exceeds maximum allowed size (2^32 bytes)\n");
        return -1;
    }
    
    get_filename_from_path(filepath, filename, sizeof(filename));
    
    snprintf(buffer, sizeof(buffer), "UPLD %s %lu\r\n", filename, filesize);
    printf("Sending: %s", buffer);
    
    if (send_all(sockfd, buffer, strlen(buffer)) == -1) {
        perror("Failed to send UPLD command");
        return -1;
    }
    
    memset(response, 0, sizeof(response));
    int bytes_received = tcp_recv(sockfd, response, sizeof(response));
    if (bytes_received <= 0) {
        printf("Error: Failed to receive response from server\n");
        return -1;
    }
    
    printf("Server response: %s\n", response);
    
    
    if (strncmp(response, "+OK", 3) != 0) {
        printf("Error: Server rejected the upload request\n");
        return -1;
    }
    
    
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        perror("Failed to open file");
        return -1;
    }
    
    
    printf("Uploading file...\n");
    long total_sent = 0;
    size_t bytes_read; 
    
    while ((bytes_read = fread(buffer, 1, BUFF_SIZE, fp)) > 0) {
        if (send_all(sockfd, buffer, bytes_read) == -1) {
            perror("Failed to send file data");
            fclose(fp);
            return -1;
        }
        total_sent += bytes_read;
        
        // Show progress for large files
        if (filesize > 1024 * 1024) {  
            printf("\rProgress: %ld / %ld bytes (%.1f%%)", total_sent, filesize, (total_sent * 100.0) / filesize);
            fflush(stdout);
        }
    }
    
    if (filesize > 1024 * 1024) {
        printf("\n");
    }
    
    fclose(fp);
    
    if (total_sent != filesize) {
        printf("Error: File size mismatch. Expected %ld, sent %ld\n", 
               filesize, total_sent);
        return -1;
    }
    
    printf("File sent successfully (%ld bytes)\n", total_sent);
    
    
    memset(response, 0, sizeof(response));
    bytes_received = tcp_recv(sockfd, response, sizeof(response));
    if (bytes_received <= 0) {
        printf("Error: Failed to receive final response from server\n");
        return -1;
    }
    
    printf("Server response: %s\n", response);
    
    if (strncmp(response, "+OK", 3) == 0) {
        return 0;
    } else {
        return -1;
    }
}

/**
* @function main: Client main function to connect to server and upload files
* @param argc: Number of command line arguments
* @param argv: Array of command line arguments
* @return: 0 on success, 1 on failure
**/
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s IP_Addr Port_Number\n", argv[0]);
        printf("Example: %s 127.0.0.1 5500\n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    
    if (port <= 0 || port > 65535) {
        printf("Invalid port number\n");
        return 1;
    }
    
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("Failed to create socket");
        return 1;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("Invalid IP address\n");
        close(sockfd);
        return 1;
    }
    
    // Connect to server
    printf("Connecting to server %s:%d...\n", server_ip, port);
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        close(sockfd);
        return 1;
    }
    
    printf("Connected successfully!\n");
    
    // Receive welcome message using tcp_recv
    char welcome[BUFF_SIZE];
    memset(welcome, 0, sizeof(welcome));
    int bytes_received = tcp_recv(sockfd, welcome, sizeof(welcome));
    if (bytes_received > 0) {
        printf("Server: %s\n", welcome);
    } else {
        printf("Failed to receive welcome message\n");
        close(sockfd);
        return 1;
    }
    
    // Main loop for file uploads
    char filepath[512];
    
    while (1) {
        printf("\nEnter file path (or press Enter to quit): ");
        fflush(stdout);
        
        if (fgets(filepath, sizeof(filepath), stdin) == NULL) {
            break;
        }
        
        // Remove trailing newline
        filepath[strcspn(filepath, "\r\n")] = '\0';
        
        
        if (strlen(filepath) == 0) {
            printf("Exiting...\n");
            break;
        }
        
        // Upload file
        if (upload_file(sockfd, filepath) == 0) {
            printf("Upload successful!\n");
        } else {
            printf("Upload failed!\n");
        }
    }
    
    // Close connection
    close(sockfd);
    printf("Connection closed.\n");
    
    return 0;
}

