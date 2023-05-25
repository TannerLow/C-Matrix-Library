#ifndef CML_KERNEL
#define CML_KERNEL

#include <CL/cl.h>

#include <stddef.h>
#include <stdio.h>


// Kernels are the functions within openCL programs.
// A program can have many kernels.
typedef struct {
    const char* kernelName;
    cl_program program;
} cml_Kernel;

// Doesn't manage the lifetime of the kernelName pointer
cml_Kernel cml_createKernel(const char* kernelName, cl_program program);
// Doesn't delete the kernelName pointer or release OpenCL program
void cml_deleteKernel(cml_Kernel* kernel);

#endif // CML_KERNEL