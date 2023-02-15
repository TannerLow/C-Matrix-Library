#ifndef CML_DEBUG_H
#define CML_DEBUG_H

#include "logger.h"

#ifdef DEBUG
    #define cml_DEBUG(statement) statement
#else
    #define cml_DEBUG(statement) NULL
#endif

#define cml_crash(code) _cml_crash(code, __FILE__, __LINE__)
int _cml_crash(int errorCode, const char* file, unsigned int line) {
    fprintf(cml_logStream, "[ERROR] Encountered error with code %d (%s, %d)\n", errorCode, file, line);
    exit(errorCode);
}

#endif // CML_DEBUG_H