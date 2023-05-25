#include <cml/util/Program.h>
#include <cml/Logger.h>
#include <cml/ErrorCodes.h>

#include <assert.h>
#include <stdlib.h>

cml_Program cml_createProgram(FILE* file) {
    assertOrExecute(file != NULL, cml_crash(CML_NULL_POINTER_PARAMETER));
 
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
