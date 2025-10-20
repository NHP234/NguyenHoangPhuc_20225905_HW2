#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static FILE *log_fp = NULL;

/**
* @function logger_init: Initialize logger and open log file
* @return: 0 on success, -1 on failure
**/
int logger_init() {
    log_fp = fopen(LOG_FILE, "a");
    if (log_fp == NULL) {
        perror("Failed to open log file");
        return -1;
    }
    return 0;
}

/**
* @function log_activity: Log an activity to the log file
* @param client_addr: Client address in format "IP:Port"
* @param request: Request received from client
* @param result: Result of the operation
**/
void log_activity(const char *client_addr, const char *request, const char *result) {
    if (log_fp == NULL) {
        return;
    }

    time_t now;
    struct tm *timeinfo;
    char timestamp[64];

    time(&now);
    timeinfo = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", timeinfo);

    if (strlen(request) == 0) {
        fprintf(log_fp, "[%s]$%s$%s\n", timestamp, client_addr, result);
    } else {
        fprintf(log_fp, "[%s]$%s$%s$%s\n", timestamp, client_addr, request, result);
    }
    fflush(log_fp);
}

/**
* @function logger_close: Close logger and release resources
**/
void logger_close() {
    if (log_fp != NULL) {
        fclose(log_fp);
        log_fp = NULL;
    }
}

