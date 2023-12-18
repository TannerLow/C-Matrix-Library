#ifndef CDH_DEBUG_H
#define CDH_DEBUG_H

#include <stddef.h>

#ifdef DEBUG
    #define cdh_DEBUG(statement) statement
#else
    #define cdh_DEBUG(statement) NULL
#endif

#define cdh_crash(code) _cdh_crash(code, __FILE__, __LINE__)
int _cdh_crash(int errorCode, const char* file, unsigned int line);

#endif // CDH_DEBUG_H