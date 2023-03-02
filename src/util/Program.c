#include <cml/util/Program.h>
#include <cml/Logger.h>

#include <assert.h>
#include <stdlib.h>

cml_Program cml_createProgram(FILE* file) {
    assert(file != NULL);
 
    cml_Program program;

    if (!file) {
        fprintf(cml_logStream, "Failed to load kernel.\n");
        abort();
    }

    program.code = (char*)malloc(CML_MAX_PROGRAM_SOURCE_SIZE);
    program.size = fread((void*)program.code, 1, CML_MAX_PROGRAM_SOURCE_SIZE, file);

    fclose(file);

    return program;
}

void cml_deleteProgram(cml_Program* program) {
    assert(program != NULL);

    free((void*)program->code);
    program->code = NULL;
    program->size = 0;
}
