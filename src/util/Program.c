#include <cml/util/Program.h>
#include <cml/ErrorCodes.h>
#include <cdh/Debug.h>
#include <cdh/Asserts.h>

#include <assert.h>
#include <stdlib.h>

// Should come last to override malloc family in debug mode
#include <cdh/Memory.h>

cml_Program cml_createProgram(FILE* file) {
    assertOrExecute(file != NULL, cdh_crash(CML_NULL_POINTER_PARAMETER));
 
    cml_Program program;

    program.code = (char*)malloc(CML_MAX_PROGRAM_SOURCE_SIZE);
    program.size = fread((void*)program.code, 1, CML_MAX_PROGRAM_SOURCE_SIZE, file);

    fclose(file);

    return program;
}

void cml_deleteProgram(cml_Program* program) {
    assert(program != NULL);

    free(program->code);
    program->code = NULL;
    program->size = 0;
}
