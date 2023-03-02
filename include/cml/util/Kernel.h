#ifndef CML_KERNEL
#define CML_KERNEL

#include "Program.h"

#include <stddef.h>
#include <stdio.h>

typedef struct {
    const char* kernelName;
    cml_Program* program;
} cml_Kernel;

cml_Kernel cml_createKernel(const char* kernelName, cml_Program* program);
void cml_deleteKernel(cml_Kernel* kernel);

#endif // CML_KERNEL