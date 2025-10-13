#include "logger.h"

#define MSSV "20225905"

/**
 * @function: write_log
 * @param: request - The request received from client
 * @param: result - The result of DNS lookup
 * @return: void
 */
void write_log(const char *request, const char *result) {
    FILE *log_file;
    char log_filename[50];
    char timestamp[30];
    char log_entry[LOG_BUFFER_SIZE];
    time_t now;
    struct tm *tm_info;
    
    // Create log filename
    sprintf(log_filename, "log_%s.txt", MSSV);
    
    // Get current time
    time(&now);
    tm_info = localtime(&now);
    
    // Format timestamp as dd/mm/yyyy hh:mm:ss
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", tm_info);
    
    // Remove newlines from result for logging
    char result_clean[BUFFER_SIZE];
    strcpy(result_clean, result);
    // Replace newlines with spaces for cleaner log
    for (int i = 0; result_clean[i] != '\0'; i++) {
        if (result_clean[i] == '\n') {
            result_clean[i] = ' ';
        }
    }
    // Remove trailing spaces
    int len = strlen(result_clean);
    while (len > 0 && result_clean[len-1] == ' ') {
        result_clean[--len] = '\0';
    }
    
    // Determine success (+) or failure (-)
    char status = '+';
    if (strstr(result_clean, "Not found information") != NULL) {
        status = '-';
    }
    
    // Create log entry with status indicator before result
    sprintf(log_entry, "[%s]$%s$%c%s\n", timestamp, request, status, result_clean);
    
    // Open log file in append mode
    log_file = fopen(log_filename, "a");
    if (log_file == NULL) {
        fprintf(stderr, "Error: Could not open log file\n");
        return;
    }
    
    // Write log entry
    fprintf(log_file, "%s", log_entry);
    
    fclose(log_file);
}

