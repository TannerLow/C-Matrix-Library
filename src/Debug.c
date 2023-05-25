#include <cml/Debug.h>
#include <stdlib.h>

int _cml_crash(int errorCode, const char* file, unsigned int line) {
    cml_log("[ERROR] Encountered error with code %d (%s, %d)\n", errorCode, file, line);
    exit(errorCode);
}