#ifndef CML_PROGRAM_H
#define CML_PROGRAM_H

#include <stdio.h>

#define CML_MAX_PROGRAM_SOURCE_SIZE 0x40000

typedef struct {
    char* code;
    size_t size;
} cml_Program;

// Takes a file of .cl format and extracts contents into a cml_Program struct.
// Closes the provided file handle if successful.
cml_Program cml_createProgram(FILE* file);

// program.code must be a heap allocated pointer
void cml_deleteProgram(cml_Program* program);

#endif // CML_PROGRAM_H