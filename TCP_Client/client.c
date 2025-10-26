#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 4096

/* Send message with \r\n delimiter */
int tcp_send(int sockfd, char *msg) {
    char buffer[BUFF_SIZE + 2];
    int len, total = 0, bytes_sent;
    
    /* Add \r\n to message */
    snprintf(buffer, sizeof(buffer), "%s\r\n", msg);
    len = strlen(buffer);
    
    /* Send all data */
    while (total < len) {
        bytes_sent = send(sockfd, buffer + total, len - total, 0);
        if (bytes_sent <= 0) {
            return -1;
        }
        total += bytes_sent;
    }
    
    return total;
}

/* Receive message until \r\n */
int tcp_receive(int sockfd, char *buffer, int max_len) {
    static char recv_buffer[BUFF_SIZE];
    static int buffer_pos = 0;
    int bytes_received, i;
    
    while (1) {
        /* Check if we have \r\n in buffer */
        for (i = 0; i < buffer_pos - 1; i++) {
            if (recv_buffer[i] == '\r' && recv_buffer[i + 1] == '\n') {
                /* Found complete message */
                int msg_len = i;
                if (msg_len >= max_len) {
                    msg_len = max_len - 1;
                }
                
                memcpy(buffer, recv_buffer, msg_len);
                buffer[msg_len] = '\0';
                
                /* Remove message from buffer */
                buffer_pos -= (i + 2);
                memmove(recv_buffer, recv_buffer + i + 2, buffer_pos);
                
                return msg_len;
            }
        }
        
        /* Receive more data */
        if (buffer_pos >= BUFF_SIZE - 1) {
            return -1; /* Buffer full */
        }
        
        bytes_received = recv(sockfd, recv_buffer + buffer_pos, 
                             BUFF_SIZE - buffer_pos - 1, 0);
        if (bytes_received <= 0) {
            return -1;
        }
        
        buffer_pos += bytes_received;
    }
}

void print_menu() {
    printf("\n===== MENU =====\n");
    printf("1. Login\n");
    printf("2. Post article\n");
    printf("3. Logout\n");
    printf("4. Exit\n");
    printf("Your choice: ");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s IP_Addr Port_Number\n", argv[0]);
        return 1;
    }
    
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    
    int client_sock;
    struct sockaddr_in server_addr;
    char buff[BUFF_SIZE];
    int bytes_sent, bytes_received;
    
    /* Create socket */
    if ((client_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        return 1;
    }
    
    /* Connect to server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connect error");
        return 1;
    }
    
    /* Receive welcome message */
    bytes_received = tcp_receive(client_sock, buff, BUFF_SIZE);
    if (bytes_received > 0) {
        if (strcmp(buff, "100") == 0) {
            printf("Connected to server successfully!\n");
        }
    }
    
    /* Main loop */
    int choice;
    int logged_in = 0;
    
    while (1) {
        print_menu();
        scanf("%d", &choice);
        getchar(); /* Clear newline */
        
        if (choice == 1) {
            /* Login */
            printf("Enter username: ");
            fgets(buff, BUFF_SIZE, stdin);
            buff[strcspn(buff, "\n")] = 0; /* Remove newline */
            
            char msg[BUFF_SIZE];
            sprintf(msg, "USER %s", buff);
            
            tcp_send(client_sock, msg);
            
            bytes_received = tcp_receive(client_sock, buff, BUFF_SIZE);
            if (bytes_received > 0) {
                if (strcmp(buff, "110") == 0) {
                    printf("Login successful!\n");
                    logged_in = 1;
                } else if (strcmp(buff, "211") == 0) {
                    printf("Account is locked!\n");
                } else if (strcmp(buff, "212") == 0) {
                    printf("Account does not exist!\n");
                } else if (strcmp(buff, "213") == 0) {
                    printf("Already logged in!\n");
                } else if (strcmp(buff, "300") == 0) {
                    printf("Unknown error!\n");
                }
            }
            
        } else if (choice == 2) {
            /* Post article */
            printf("Enter article content: ");
            fgets(buff, BUFF_SIZE, stdin);
            buff[strcspn(buff, "\n")] = 0; /* Remove newline */
            
            char msg[BUFF_SIZE];
            sprintf(msg, "POST %s", buff);
            
            tcp_send(client_sock, msg);
            
            bytes_received = tcp_receive(client_sock, buff, BUFF_SIZE);
            if (bytes_received > 0) {
                if (strcmp(buff, "120") == 0) {
                    printf("Post successful!\n");
                } else if (strcmp(buff, "221") == 0) {
                    printf("Please login first!\n");
                } else if (strcmp(buff, "300") == 0) {
                    printf("Unknown error!\n");
                }
            }
            
        } else if (choice == 3) {
            /* Logout */
            strcpy(buff, "BYE");
            tcp_send(client_sock, buff);
            
            bytes_received = tcp_receive(client_sock, buff, BUFF_SIZE);
            if (bytes_received > 0) {
                if (strcmp(buff, "130") == 0) {
                    printf("Logout successful!\n");
                    logged_in = 0;
                    break; /* Close connection after logout */
                } else if (strcmp(buff, "221") == 0) {
                    printf("You are not logged in!\n");
                } else if (strcmp(buff, "300") == 0) {
                    printf("Unknown error!\n");
                }
            }
            
        } else if (choice == 4) {
            /* Exit */
            printf("Goodbye!\n");
            break;
        } else {
            printf("Invalid choice!\n");
        }
    }
    
    close(client_sock);
    return 0;
}

