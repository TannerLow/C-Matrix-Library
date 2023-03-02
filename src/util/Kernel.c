#include <cml/util/Kernel.h>
#include <cml/Logger.h>

#include <stdlib.h>
#include <assert.h>

cml_Kernel cml_createKernel(const char* kernelName, cml_Program* program) {
    assert(kernelName != NULL);
    assert(program != NULL);
    assert(program->code != NULL);

    cml_Kernel kernel;
    kernel.kernelName = kernelName;
    kernel.program = program;

    return kernel;
}

void cml_deleteKernel(cml_Kernel* kernel) {
    assert(kernel != NULL);

    kernel->program = NULL;
    kernel->kernelName = NULL;
}
