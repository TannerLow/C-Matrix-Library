#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

extern FILE* cml_logStream;

#define cml_log(format, ...) fprintf(cml_logStream, format, __VA_ARGS__)
#define cml_log(format) fprintf(cml_logStream, format)

#endif // LOGGER_H