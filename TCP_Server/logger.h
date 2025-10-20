#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>

#define LOG_FILE "log_20225905.txt"

/**
* @function logger_init: Initialize logger and open log file
* @return: 0 on success, -1 on failure
**/
int logger_init();

/**
* @function log_activity: Log an activity to the log file
* @param client_addr: Client address in format "IP:Port"
* @param request: Request received from client
* @param result: Result of the operation
**/
void log_activity(const char *client_addr, const char *request, const char *result);

/**
* @function logger_close: Close logger and release resources
**/
void logger_close();

#endif // LOGGER_H

