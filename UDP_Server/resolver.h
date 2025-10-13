#ifndef RESOLVER_H
#define RESOLVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

/* Global Constants */
#define MAX_HOSTNAME_LEN 256
#define MAX_QUERY_ATTEMPTS 100
#define MAX_UNIQUE_HOSTNAMES 10
#define BUFFER_SIZE 2048
#define LOG_BUFFER_SIZE 4096

#endif /* RESOLVER_H */

