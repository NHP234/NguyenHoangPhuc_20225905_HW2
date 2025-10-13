#ifndef LOGGER_H
#define LOGGER_H

#include "resolver.h"

/**
 * @function: write_log
 * @param: request - The request received from client
 * @param: result - The result of DNS lookup
 * @return: void
 */
void write_log(const char *request, const char *result);

#endif /* LOGGER_H */

