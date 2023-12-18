#ifndef CDH_LOGGER_H
#define CDH_LOGGER_H

#include <stdio.h>

// Need to manually set this stream in order to log, else its undefined behavior
extern FILE* cdh_logStream;

// "..." refers to any valid set of printf parameters
#define cdh_log(...) fprintf(cdh_logStream, __VA_ARGS__)

#endif // LOGGER_H