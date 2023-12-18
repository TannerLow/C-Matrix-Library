#include <cml/util/Kernel.h>
#include <cdh/Debug.h>
#include "cdh/Asserts.h"
#include <cml/ErrorCodes.h>

#include <stdlib.h>

// Should come last to override malloc family in debug mode
#include <cdh/Memory.h>

cml_Kernel cml_createKernel(const char* kernelName, cl_program program) {
    assertOrExecute(kernelName != NULL, cdh_crash(CML_NULL_POINTER_PARAMETER));
    assertOrExecute(program != 0, cdh_crash(CML_CL_PROGRAM_0));

    cml_Kernel kernel;
    kernel.kernelName = kernelName;
    kernel.program = program;

    return kernel;
}

void cml_deleteKernel(cml_Kernel* kernel) {
    assertOrReturn(kernel != NULL);

    // OpenCL shouldn't use 0 as an identifier as far as I know
    kernel->program = (cl_program)0;
    kernel->kernelName = NULL;
}
