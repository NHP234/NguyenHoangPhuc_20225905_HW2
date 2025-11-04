#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFF_SIZE 4096

/* Connection state */
typedef struct {
    char recv_buffer[BUFF_SIZE];
    int buffer_pos;
} conn_state_t;

/* Function prototypes */
int tcp_send(int sockfd, char *msg);
int tcp_receive(int sockfd, conn_state_t *state, char *buffer, int max_len);
void print_menu();
void print_response(char *code);

/**
 * @function tcp_send: Send message to server with \r\n delimiter
 * @param sockfd: Socket file descriptor of the server connection
 * @param msg: Message string to send (without \r\n)
 * @return: Number of bytes sent on success, -1 on error
 **/
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

/**
 * @function tcp_receive: Receive complete message from server (delimited by \r\n)
 * @param sockfd: Socket file descriptor of the server connection
 * @param state: Connection state containing receive buffer
 * @param buffer: Buffer to store the received message
 * @param max_len: Maximum length of the buffer
 * @return: Length of received message on success, -1 on error
 **/
int tcp_receive(int sockfd, conn_state_t *state, char *buffer, int max_len) {
    int bytes_received, i;
    
    while (1) {
        /* Check if we have \r\n in recv_buffer */
        for (i = 0; i < state->buffer_pos - 1; i++) {
            if (state->recv_buffer[i] == '\r' && state->recv_buffer[i + 1] == '\n') {
                /* Found complete message */
                int msg_len = i;
                if (msg_len >= max_len) {
                    msg_len = max_len - 1;
                }
                
                memcpy(buffer, state->recv_buffer, msg_len);
                buffer[msg_len] = '\0';
                
                /* Remove message from buffer */
                state->buffer_pos -= (i + 2);
                memmove(state->recv_buffer, state->recv_buffer + i + 2, state->buffer_pos);
                
                return msg_len;
            }
        }
        
        /* Receive more data */
        if (state->buffer_pos >= BUFF_SIZE - 1) {
            return -1; /* Buffer full */
        }
        
        bytes_received = recv(sockfd, state->recv_buffer + state->buffer_pos, BUFF_SIZE - state->buffer_pos - 1, 0);
        if (bytes_received <= 0) {
            return -1;
        }
        
        state->buffer_pos += bytes_received;
    }
}

/**
 * @function print_menu: Display the main menu options to user
 * @param: None
 * @return: None
 **/
void print_menu() {
    printf("\n=== ARTICLE POSTING SYSTEM ===\n");
    printf("1. Login\n");
    printf("2. Post article\n");
    printf("3. Logout\n");
    printf("4. Exit\n");
    printf("==============================\n");
    printf("Your choice: ");
}

/**
 * @function print_response: Translate and display server response code
 * @param code: Response code string from server
 * @return: None
 **/
void print_response(char *code) {
    if (strcmp(code, "100") == 0) {
        printf(">> Connected to server successfully\n");
    } else if (strcmp(code, "110") == 0) {
        printf(">> Login successful\n");
    } else if (strcmp(code, "120") == 0) {
        printf(">> Article posted successfully\n");
    } else if (strcmp(code, "130") == 0) {
        printf(">> Logout successful\n");
    } else if (strcmp(code, "211") == 0) {
        printf(">> Account is blocked\n");
    } else if (strcmp(code, "212") == 0) {
        printf(">> Account does not exist\n");
    } else if (strcmp(code, "213") == 0) {
        printf(">> Already logged in\n");
    } else if (strcmp(code, "214") == 0) {
        printf(">> Account already logged in on another client\n");
    } else if (strcmp(code, "221") == 0) {
        printf(">> Not logged in yet\n");
    } else if (strcmp(code, "300") == 0) {
        printf(">> Unknown command\n");
    } else {
        printf(">> Response: %s\n", code);
    }
}

/**
 * @function main: Main client function to connect to server and handle user interaction
 * @param argc: Number of command line arguments
 * @param argv: Array of command line arguments (argv[1] is IP, argv[2] is port)
 * @return: 0 on normal exit, 1 on error
 **/
int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char ip_addr[50];
    int port;
    conn_state_t state;
    char buffer[BUFF_SIZE];
    char username[100];
    char article[BUFF_SIZE];
    char command[BUFF_SIZE];
    int choice;
    int is_logged_in = 0;
    
    if (argc != 3) {
        printf("Usage: %s IP_Addr Port_Number\n", argv[0]);
        return 1;
    }
    
    strcpy(ip_addr, argv[1]);
    port = atoi(argv[2]);
    
    /* Create socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket() error");
        return 1;
    }
    
    /* Connect to server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip_addr);
    
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect() error");
        close(sockfd);
        return 1;
    }
    
    /* Initialize state */
    memset(&state, 0, sizeof(conn_state_t));
    
    /* Receive welcome message */
    if (tcp_receive(sockfd, &state, buffer, BUFF_SIZE) > 0) {
        print_response(buffer);
    }
    
    /* Main loop */
    while (1) {
        print_menu();
        
        if (scanf("%d", &choice) != 1) {
            /* Clear input buffer */
            while (getchar() != '\n');
            printf("Invalid choice\n");
            continue;
        }
        getchar(); 
        
        switch (choice) {
            case 1: /* Login */
                printf("Enter username: ");
                if (fgets(username, sizeof(username), stdin) == NULL) {
                    break;
                }
                username[strcspn(username, "\n")] = 0; 
                
                snprintf(command, sizeof(command), "USER %s", username);
                if (tcp_send(sockfd, command) > 0) {
                    if (tcp_receive(sockfd, &state, buffer, BUFF_SIZE) > 0) {
                        print_response(buffer);
                        if (strcmp(buffer, "110") == 0) {
                            is_logged_in = 1;
                        }
                    }
                }
                break;
                
            case 2: /* Post article */
                printf("Enter article content: ");
                if (fgets(article, sizeof(article), stdin) == NULL) {
                    break;
                }
                article[strcspn(article, "\n")] = 0; 
                
                snprintf(command, sizeof(command), "POST %s", article);
                if (tcp_send(sockfd, command) > 0) {
                    if (tcp_receive(sockfd, &state, buffer, BUFF_SIZE) > 0) {
                        print_response(buffer);
                    }
                }
                break;
                
            case 3: /* Logout */
                if (tcp_send(sockfd, "BYE") > 0) {
                    if (tcp_receive(sockfd, &state, buffer, BUFF_SIZE) > 0) {
                        print_response(buffer);
                        if (strcmp(buffer, "130") == 0) {
                            is_logged_in = 0;
                        }
                    }
                }
                break;
                
            case 4: /* Exit */
                if (is_logged_in) {
                    tcp_send(sockfd, "BYE");
                    tcp_receive(sockfd, &state, buffer, BUFF_SIZE);
                }
                printf("Goodbye!\n");
                close(sockfd);
                return 0;
                
            default:
                printf("Invalid choice\n");
        }
    }
    
    close(sockfd);
    return 0;
}

