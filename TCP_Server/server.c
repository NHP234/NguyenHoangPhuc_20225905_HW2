/* Multi-threaded TCP Server for posting articles */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define BUFF_SIZE 4096
#define MAX_ACCOUNTS 100
#define MAX_USERNAME 50
#define BACKLOG 20

/* Account structure */
typedef struct {
    char username[MAX_USERNAME];
    int status; /* 0: blocked, 1: active */
    int logged_in; /* 0: not logged in, 1: logged in */
} account_t;

/* Connection state for each client */
typedef struct {
    char recv_buffer[BUFF_SIZE];
    int buffer_pos;
    int sockfd;
    char logged_user[MAX_USERNAME];
    int is_logged_in;
} conn_state_t;

/* Global variables */
account_t accounts[MAX_ACCOUNTS];
int account_count = 0;
pthread_mutex_t account_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Function prototypes */
void load_accounts();
int tcp_send(int sockfd, char *msg);
int tcp_receive(int sockfd, conn_state_t *state, char *buffer, int max_len);
void *handle_client(void *arg);
void process_command(conn_state_t *state, char *command);

/* Load accounts from file */
void load_accounts() {
    FILE *f = fopen("TCP_Server/account.txt", "r");
    if (f == NULL) {
        perror("Cannot open TCP_Server/account.txt");
        exit(1);
    }
    
    account_count = 0;
    while (fscanf(f, "%s %d", accounts[account_count].username, 
                  &accounts[account_count].status) == 2) {
        accounts[account_count].logged_in = 0;
        account_count++;
        if (account_count >= MAX_ACCOUNTS) break;
    }
    
    fclose(f);
    printf("Loaded %d accounts\n", account_count);
}

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

/* Receive message with \r\n delimiter */
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
        
        bytes_received = recv(sockfd, state->recv_buffer + state->buffer_pos, 
                             BUFF_SIZE - state->buffer_pos - 1, 0);
        if (bytes_received <= 0) {
            return -1;
        }
        
        state->buffer_pos += bytes_received;
    }
}

/* Process client commands */
void process_command(conn_state_t *state, char *command) {
    char cmd[20], arg[BUFF_SIZE];
    int i;
    
    /* Parse command */
    if (sscanf(command, "%s", cmd) != 1) {
        tcp_send(state->sockfd, "300");
        return;
    }
    
    /* Handle USER command */
    if (strcmp(cmd, "USER") == 0) {
        if (sscanf(command, "USER %s", arg) != 1) {
            tcp_send(state->sockfd, "300");
            return;
        }
        
        /* Check if already logged in */
        if (state->is_logged_in) {
            tcp_send(state->sockfd, "213");
            return;
        }
        
        /* Find account */
        pthread_mutex_lock(&account_mutex);
        int found = -1;
        for (i = 0; i < account_count; i++) {
            if (strcmp(accounts[i].username, arg) == 0) {
                found = i;
                break;
            }
        }
        
        if (found == -1) {
            pthread_mutex_unlock(&account_mutex);
            tcp_send(state->sockfd, "212");
            return;
        }
        
        if (accounts[found].status == 0) {
            pthread_mutex_unlock(&account_mutex);
            tcp_send(state->sockfd, "211");
            return;
        }
        
        if (accounts[found].logged_in == 1) {
            pthread_mutex_unlock(&account_mutex);
            tcp_send(state->sockfd, "214");
            return;
        }
        
        /* Login successful */
        accounts[found].logged_in = 1;
        strcpy(state->logged_user, arg);
        state->is_logged_in = 1;
        pthread_mutex_unlock(&account_mutex);
        
        tcp_send(state->sockfd, "110");
        printf("User %s logged in\n", arg);
    }
    /* Handle POST command */
    else if (strcmp(cmd, "POST") == 0) {
        if (!state->is_logged_in) {
            tcp_send(state->sockfd, "221");
            return;
        }
        
        /* Extract article content (after "POST ") */
        char *article = command + 5;
        if (strlen(article) == 0) {
            tcp_send(state->sockfd, "300");
            return;
        }
        
        printf("User %s posted: %s\n", state->logged_user, article);
        tcp_send(state->sockfd, "120");
    }
    /* Handle BYE command */
    else if (strcmp(cmd, "BYE") == 0) {
        if (!state->is_logged_in) {
            tcp_send(state->sockfd, "221");
            return;
        }
        
        /* Logout */
        pthread_mutex_lock(&account_mutex);
        for (i = 0; i < account_count; i++) {
            if (strcmp(accounts[i].username, state->logged_user) == 0) {
                accounts[i].logged_in = 0;
                break;
            }
        }
        pthread_mutex_unlock(&account_mutex);
        
        printf("User %s logged out\n", state->logged_user);
        state->is_logged_in = 0;
        tcp_send(state->sockfd, "130");
    }
    else {
        tcp_send(state->sockfd, "300");
    }
}

/* Handle client connection */
void *handle_client(void *arg) {
    conn_state_t *state = (conn_state_t *)arg;
    char buffer[BUFF_SIZE];
    int ret;
    
    /* Send welcome message */
    tcp_send(state->sockfd, "100");
    
    /* Process commands */
    while (1) {
        ret = tcp_receive(state->sockfd, state, buffer, BUFF_SIZE);
        if (ret <= 0) {
            break; /* Connection closed or error */
        }
        
        printf("Received: %s\n", buffer);
        process_command(state, buffer);
    }
    
    /* Logout if logged in */
    if (state->is_logged_in) {
        pthread_mutex_lock(&account_mutex);
        for (int i = 0; i < account_count; i++) {
            if (strcmp(accounts[i].username, state->logged_user) == 0) {
                accounts[i].logged_in = 0;
                printf("User %s disconnected (auto logout)\n", state->logged_user);
                break;
            }
        }
        pthread_mutex_unlock(&account_mutex);
    }
    
    close(state->sockfd);
    free(state);
    pthread_detach(pthread_self());
    return NULL;
}

int main(int argc, char *argv[]) {
    int listenfd, connfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t sin_size;
    pthread_t tid;
    int port;
    
    if (argc != 2) {
        printf("Usage: %s Port_Number\n", argv[0]);
        return 1;
    }
    
    port = atoi(argv[1]);
    
    /* Load accounts */
    load_accounts();
    
    /* Create socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket() error");
        return 1;
    }
    
    /* Set socket options */
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
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
    
    /* Accept connections */
    while (1) {
        sin_size = sizeof(client_addr);
        connfd = accept(listenfd, (struct sockaddr *)&client_addr, &sin_size);
        if (connfd == -1) {
            perror("accept() error");
            continue;
        }
        
        printf("New connection from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        /* Create state for this connection */
        conn_state_t *state = malloc(sizeof(conn_state_t));
        memset(state, 0, sizeof(conn_state_t));
        state->sockfd = connfd;
        
        /* Create thread to handle client */
        pthread_create(&tid, NULL, handle_client, state);
    }
    
    close(listenfd);
    return 0;
}

