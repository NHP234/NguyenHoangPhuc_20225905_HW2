#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define BUFF_SIZE 4096
#define MAX_ACCOUNTS 100
#define MAX_USERNAME 50
#define BACKLOG 20
#define MAX_CLIENTS FD_SETSIZE

/* Account structure */
typedef struct {
    char username[MAX_USERNAME];
    int status; /* 0: blocked, 1: active */
} account_t;

/* Connection state for each client */
typedef struct {
    char recv_buffer[BUFF_SIZE];
    int buffer_pos;
    char logged_user[MAX_USERNAME];
    int is_logged_in;
} conn_state_t;

/* Global variables */
account_t accounts[MAX_ACCOUNTS];
int account_count = 0;
int client_sockets[MAX_CLIENTS];
conn_state_t client_states[MAX_CLIENTS];

/* Function prototypes */
void load_accounts();
int tcp_send(int sockfd, char *msg);
int tcp_receive(int sockfd, conn_state_t *state, char *buffer, int max_len);
void process_command(int client_idx, char *command);
void disconnect_client(int idx);

/**
 * @function load_accounts: Load user accounts from file into memory
 * @param: None
 * @return: None
 **/
void load_accounts() {
    FILE *f = fopen("TCP_Server/account.txt", "r");
    if (f == NULL) {
        perror("Cannot open TCP_Server/account.txt");
        exit(1);
    }
    
    account_count = 0;
    while (fscanf(f, "%s %d", accounts[account_count].username, 
                  &accounts[account_count].status) == 2) {
        account_count++;
        if (account_count >= MAX_ACCOUNTS) break;
    }
    
    fclose(f);
    printf("Loaded %d accounts\n", account_count);
}

/**
 * @function tcp_send: Send message to client with \r\n delimiter
 * @param sockfd: Socket file descriptor of the client
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
 * @function tcp_receive: Receive complete message from client (delimited by \r\n)
 * @param sockfd: Socket file descriptor of the client
 * @param state: Connection state containing receive buffer
 * @param buffer: Buffer to store the received message
 * @param max_len: Maximum length of the buffer
 * @return: Length of received message on success, 0 if incomplete, -1 on error
 **/
int tcp_receive(int sockfd, conn_state_t *state, char *buffer, int max_len) {
    int i, bytes_received;
    
    /* First, check if buffer already has complete message */
    for (i = 0; i < state->buffer_pos - 1; i++) {
        if (state->recv_buffer[i] == '\r' && state->recv_buffer[i + 1] == '\n') {
            /* Found complete message */
            int msg_len = i;
            if (msg_len >= max_len) {
                msg_len = max_len - 1;
            }
            
            /* Copy message to output buffer */
            memcpy(buffer, state->recv_buffer, msg_len);
            buffer[msg_len] = '\0';
            
            /* Remove processed message from buffer */
            state->buffer_pos -= (i + 2);
            memmove(state->recv_buffer, state->recv_buffer + i + 2, state->buffer_pos);
            
            return msg_len;
        }
    }
    
    /* No complete message in buffer, try to receive more data */
    if (state->buffer_pos >= BUFF_SIZE - 1) {
        return -1; /* Buffer full */
    }
    
    bytes_received = recv(sockfd, state->recv_buffer + state->buffer_pos, 
                         BUFF_SIZE - state->buffer_pos - 1, 0);
    
    if (bytes_received <= 0) {
        return -1; /* Connection closed or error */
    }
    
    state->buffer_pos += bytes_received;
    
    /* Check again if we now have complete message */
    for (i = 0; i < state->buffer_pos - 1; i++) {
        if (state->recv_buffer[i] == '\r' && state->recv_buffer[i + 1] == '\n') {
            /* Found complete message */
            int msg_len = i;
            if (msg_len >= max_len) {
                msg_len = max_len - 1;
            }
            
            /* Copy message to output buffer */
            memcpy(buffer, state->recv_buffer, msg_len);
            buffer[msg_len] = '\0';
            
            /* Remove processed message from buffer */
            state->buffer_pos -= (i + 2);
            memmove(state->recv_buffer, state->recv_buffer + i + 2, state->buffer_pos);
            
            return msg_len;
        }
    }
    
    return 0; /* Still incomplete message */
}

/**
 * @function process_command: Process and execute client commands (USER, POST, BYE)
 * @param client_idx: Index of client in client_sockets array
 * @param command: Command string received from client
 * @return: None
 **/
void process_command(int client_idx, char *command) {
    char cmd[20], arg[BUFF_SIZE];
    int i;
    int sockfd = client_sockets[client_idx];
    conn_state_t *state = &client_states[client_idx];
    
    /* Parse command */
    if (sscanf(command, "%s", cmd) != 1) {
        tcp_send(sockfd, "300");
        return;
    }
    
    /* Handle USER command */
    if (strcmp(cmd, "USER") == 0) {
        /* Check if already logged in */
        if (state->is_logged_in) {
            tcp_send(sockfd, "213");
            return;
        }
        
        /* Try to parse username */
        if (sscanf(command, "USER %s", arg) != 1) {
            /* USER command without username -> account not exist */
            tcp_send(sockfd, "212");
            return;
        }
        
        /* Find account */
        int found = -1;
        for (i = 0; i < account_count; i++) {
            if (strcmp(accounts[i].username, arg) == 0) {
                found = i;
                break;
            }
        }
        
        if (found == -1) {
            tcp_send(sockfd, "212");
            return;
        }
        
        if (accounts[found].status == 0) {
            tcp_send(sockfd, "211");
            return;
        }
        
        /* Login successful */
        strcpy(state->logged_user, arg);
        state->is_logged_in = 1;
        
        tcp_send(sockfd, "110");
        printf("User %s logged in\n", arg);
    }
    /* Handle POST command */
    else if (strcmp(cmd, "POST") == 0) {
        if (!state->is_logged_in) {
            tcp_send(sockfd, "221");
            return;
        }
        
        /* Extract article content (after "POST ") */
        char *article = command + 5;
        if (strlen(article) == 0) {
            tcp_send(sockfd, "300");
            return;
        }
        
        printf("User %s posted: %s\n", state->logged_user, article);
        tcp_send(sockfd, "120");
    }
    /* Handle BYE command */
    else if (strcmp(cmd, "BYE") == 0) {
        if (!state->is_logged_in) {
            tcp_send(sockfd, "221");
            return;
        }
        
        printf("User %s logged out\n", state->logged_user);
        state->is_logged_in = 0;
        tcp_send(sockfd, "130");
    }
    else {
        tcp_send(sockfd, "300");
    }
}

/**
 * @function disconnect_client: Disconnect and cleanup client connection
 * @param idx: Index of client in client_sockets array
 * @return: None
 **/
void disconnect_client(int idx) {
    if (client_states[idx].is_logged_in) {
        printf("User %s disconnected (auto logout)\n", client_states[idx].logged_user);
    }
    
    close(client_sockets[idx]);
    client_sockets[idx] = -1;
    memset(&client_states[idx], 0, sizeof(conn_state_t));
}

/**
 * @function main: Main server function using I/O multiplexing with select()
 * @param argc: Number of command line arguments
 * @param argv: Array of command line arguments (argv[1] is port number)
 * @return: 0 on normal exit, 1 on error
 **/
int main(int argc, char *argv[]) {
    int listenfd, connfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    int port;
    fd_set readfds, allset;
    int maxfd, i, nready;
    char buffer[BUFF_SIZE];
    
    if (argc != 2) {
        printf("Usage: %s Port_Number\n", argv[0]);
        return 1;
    }
    
    port = atoi(argv[1]);
    
    load_accounts();
    
    /* Initialize client arrays */
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = -1;
    }
    
    /* Create socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket() error");
        return 1;
    }
    
    /* Bind */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() error");
        close(listenfd);
        return 1;
    }
    
    /* Listen */
    if (listen(listenfd, BACKLOG) == -1) {
        perror("listen() error");
        close(listenfd);
        return 1;
    }
    
    printf("Server started at port %d\n", port);
    
    /* Initialize select sets */
    maxfd = listenfd;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    
    /* Main loop with select */
    while (1) {
        readfds = allset;
        nready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        
        if (nready < 0) {
            perror("select() error");
            continue;
        }
        
        /* Check for new connection */
        if (FD_ISSET(listenfd, &readfds)) {
            sin_size = sizeof(client_addr);
            connfd = accept(listenfd, (struct sockaddr *)&client_addr, &sin_size);
            
            if (connfd < 0) {
                perror("accept() error");
                continue;
            }
            
            /* Find empty slot for new client */
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == -1) {
                    client_sockets[i] = connfd;
                    memset(&client_states[i], 0, sizeof(conn_state_t));
                    FD_SET(connfd, &allset);
                    if (connfd > maxfd) {
                        maxfd = connfd;
                    }
                    
                    printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    
                    /* Send welcome message */
                    tcp_send(connfd, "100");
                    break;
                }
            }
            
            if (i == MAX_CLIENTS) {
                printf("Too many clients, connection rejected\n");
                close(connfd);
            }
            
            if (--nready <= 0) {
                continue; /* No more readable descriptors */
            }
        }
        
        /* Check all clients for data */
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sockfd = client_sockets[i];
            if (sockfd == -1) {
                continue;
            }
            
            if (FD_ISSET(sockfd, &readfds)) {
                int ret = tcp_receive(sockfd, &client_states[i], buffer, BUFF_SIZE);
                
                if (ret < 0) {
                    /* Connection closed or error */
                    disconnect_client(i);
                    FD_CLR(sockfd, &allset);
                } else if (ret > 0) {
                    /* Process command */
                    printf("Received: %s\n", buffer);
                    process_command(i, buffer);
                    
                    /* Check if there are more messages in buffer */
                    while (client_states[i].buffer_pos > 0) {
                        ret = tcp_receive(sockfd, &client_states[i], buffer, BUFF_SIZE);
                        if (ret > 0) {
                            printf("Received: %s\n", buffer);
                            process_command(i, buffer);
                        } else {
                            break;
                        }
                    }
                }
                
                if (--nready <= 0) {
                    break; /* No more readable descriptors */
                }
            }
        }
    }
    
    close(listenfd);
    return 0;
}
