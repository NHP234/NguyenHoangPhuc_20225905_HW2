#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#define BUFF_SIZE 4096
#define MAX_USERNAME 1024

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

/* Check if account exists and get status */
int check_account(char *username, int *status) {
    FILE *f = fopen("TCP_Server/account.txt", "r");
    if (f == NULL) {
        perror("Cannot open TCP_Server/account.txt");
        return 0;
    }
    
    char line[BUFF_SIZE];
    char user[MAX_USERNAME];
    int stat;
    
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%s %d", user, &stat) == 2) {
            if (strcmp(user, username) == 0) {
                *status = stat;
                fclose(f);
                return 1; /* Account exists */
            }
        }
    }
    
    fclose(f);
    return 0; /* Account not found */
}

/* Handle client connection */
void handle_client(int client_sock) {
    char buff[BUFF_SIZE];
    int bytes_received;
    char username[MAX_USERNAME] = "";
    int logged_in = 0; /* 0: not logged in, 1: logged in */
    
    /* Send welcome message */
    strcpy(buff, "100");
    tcp_send(client_sock, buff);
    
    while (1) {
        bytes_received = tcp_receive(client_sock, buff, BUFF_SIZE);
        if (bytes_received <= 0) {
            break;
        }
        
        printf("Received: %s\n", buff);
        
        /* Parse command */
        if (strncmp(buff, "USER ", 5) == 0) {
            /* Login request */
            if (logged_in) {
                strcpy(buff, "213"); /* Already logged in */
            } else {
                char user[MAX_USERNAME];
                sscanf(buff + 5, "%s", user);
                
                int status;
                if (check_account(user, &status)) {
                    if (status == 1) {
                        strcpy(buff, "110"); /* Login successful */
                        strcpy(username, user);
                        logged_in = 1;
                        printf("User %s logged in\n", username);
                    } else {
                        strcpy(buff, "211"); /* Account locked */
                    }
                } else {
                    strcpy(buff, "212"); /* Account not exist */
                }
            }
        } else if (strncmp(buff, "POST ", 5) == 0) {
            /* Post article */
            if (!logged_in) {
                strcpy(buff, "221"); /* Not logged in */
            } else {
                char *article = buff + 5;
                printf("User %s posted: %s\n", username, article);
                strcpy(buff, "120"); /* Post successful */
            }
        } else if (strcmp(buff, "BYE") == 0) {
            /* Logout */
            if (!logged_in) {
                strcpy(buff, "221"); /* Not logged in */
            } else {
                printf("User %s logged out\n", username);
                strcpy(buff, "130"); /* Logout successful */
                logged_in = 0;
                username[0] = '\0';
            }
        } else {
            strcpy(buff, "300"); /* Unknown command */
        }
        
        if (tcp_send(client_sock, buff) <= 0) {
            break;
        }
        
        /* If logout, close connection */
        if (strcmp(buff, "130") == 0) {
            break;
        }
    }
    
    close(client_sock);
}

/* Signal handler for child process */
void sig_child(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("Child %d terminated\n", pid);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s Port_Number\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    int listen_sock, conn_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    pid_t pid;
    
    /* Create socket */
    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket error");
        return 1;
    }
    
    /* Bind */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind error");
        return 1;
    }
    
    /* Listen */
    if (listen(listen_sock, 20) == -1) {
        perror("Listen error");
        return 1;
    }
    
    /* Setup signal handler */
    signal(SIGCHLD, sig_child);
    
    printf("Server started at port %d\n", port);
    
    /* Accept loop */
    while (1) {
        sin_size = sizeof(client_addr);
        if ((conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &sin_size)) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("Accept error");
            continue;
        }
        
        printf("Connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        /* Fork child process */
        pid = fork();
        if (pid == 0) {
            /* Child process */
            close(listen_sock);
            handle_client(conn_sock);
            exit(0);
        } else if (pid > 0) {
            /* Parent process */
            close(conn_sock);
        } else {
            perror("Fork error");
        }
    }
    
    close(listen_sock);
    return 0;
}

