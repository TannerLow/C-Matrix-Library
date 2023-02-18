#include "Testing.h"

#include <cml/Logger.h>
#include <cml/device/GPU.h>
#include <cml/Debug.h>
#include <cml/ErrorCodes.h>
#include <cml/matrix/Matrix.h>
#include <cml/matrix/MatrixMath.h>

#include <CL/cl.h>

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define MAX_KERNEL_SOURCE_SIZE 0x10000

void fillRandom(float*,const size_t);

void copyFloatArray(float* src, float* dst, const size_t count);
bool withinRange(float, float, float);

int driver() {
    const unsigned int widthA = 1024;
    const unsigned int heightA = 1024;
    const unsigned int widthB = 1024;
    const unsigned int heightB = 1024;
    const unsigned int widthC = 1024;

    // Testing CPU matrix multiplication
    cml_Matrix matrixA = cml_createMatrix(heightA, widthA);
    cml_Matrix matrixB = cml_createMatrix(heightB, widthB);
    cml_Matrix matrixC = cml_createMatrix(heightA, widthB);

    // OpenCL device memory
    cl_mem dA;
    cl_mem dB;
    cl_mem dC;

    srand(123);

    // Host memory (non-GPU stuff)
    const unsigned int sizeA = widthA * heightA;
    const unsigned int memSizeA = sizeof(float) * sizeA;
    float* hostA = (float*)malloc(memSizeA);
    assert(hostA != NULL);

    const unsigned int sizeB = widthB * heightB;
    const unsigned int memSizeB = sizeof(float) * sizeB;
    float* hostB = (float*)malloc(memSizeB);
    assert(hostB != NULL);

    fillRandom(hostA, sizeA);
    fillRandom(hostB, sizeB);
    copyFloatArray(hostA, matrixA.data, sizeA);
    copyFloatArray(hostB, matrixB.data, sizeB);

    unsigned int sizeC = heightA * widthB;
    const unsigned int memSizeC = sizeof(float) * sizeC;
    float* hostC = (float*)malloc(memSizeC);
    assert(hostC != NULL);

    // GPU setup
    DeviceArray gpus = cml_getGPUsWithCUDASupport(5, 5);
    assert(gpus.count > 0);

    cl_device_id gpu = gpus.deviceIds[0];

    cl_int clCode;
    cl_context context = clCreateContext(NULL, 1, &gpu, NULL, NULL, &clCode);
    assert(clCode == CL_SUCCESS);
    cl_command_queue commands = clCreateCommandQueue(context, gpu, 0, &clCode);
    assert(clCode == CL_SUCCESS);

    // Load kernel into GPU
    FILE *fp;
    char *kernelSource;
    size_t kernelSourceSize;
 
    fp = fopen("matrix_multiply_kernel.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        abort();
    }
    kernelSource = (char*)malloc(MAX_KERNEL_SOURCE_SIZE);
    kernelSourceSize = fread(kernelSource, 1, MAX_KERNEL_SOURCE_SIZE, fp);
    fclose(fp);

    cl_program program = clCreateProgramWithSource(context, 1, (const char**) &kernelSource, (const size_t*) &kernelSourceSize, &clCode);
    assert(clCode == CL_SUCCESS);
    
    clCode = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if(clCode != CL_SUCCESS) {
        size_t len;
        char buffer[2048];
        fprintf(cml_logStream, "[ERROR] Failed to build kernel executable!\n");
        clGetProgramBuildInfo(program, gpu, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        fprintf(cml_logStream, "%s\n", buffer);
        cml_crash(CML_CL_ERROR);
    }

    cl_kernel kernel = clCreateKernel(program, "matrixMul", &clCode);
    assert(kernel && clCode == CL_SUCCESS);

    // Transfer data to GPU
    dA = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, memSizeA, hostA, &clCode);
    dB = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, memSizeB, hostB, &clCode);
    dC = clCreateBuffer(context, CL_MEM_READ_WRITE, memSizeA, NULL, &clCode);
    assert(dA && dB && dC);

    fprintf(cml_logStream, "Running matrix multiplication for matrices A (%dx%d) and B (%dx%d) ...\n", widthA, heightA, widthB, heightB); 

    int wA  = widthA;
    int wC  = widthC;
    clCode  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&dC);
    clCode |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&dA);
    clCode |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&dB);
    clCode |= clSetKernelArg(kernel, 3, sizeof(int),    (void*)&wA);
    clCode |= clSetKernelArg(kernel, 4, sizeof(int),    (void*)&wC);

    assert(clCode == CL_SUCCESS);

    // Run the kernel on the GPU
    size_t localWorkSize[2], globalWorkSize[2];
    localWorkSize[0] = 16;
    localWorkSize[1] = 16;
    globalWorkSize[0] = 1024;
    globalWorkSize[1] = 1024;

    clCode = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, globalWorkSize, localWorkSize, 
    0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    // Copy back result to host memory
    clCode = clEnqueueReadBuffer(commands, dC, CL_TRUE, 0, memSizeC, hostC, 0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    fprintf(cml_logStream, "Start CPU multiplication\n");
    cml_matrixMultiply(&matrixA, &matrixB, &matrixC);
    fprintf(cml_logStream, "done\n");
    for(size_t i = 0; i < (size_t)sizeA; i++) {
        // fprintf(cml_logStream, "%.10f %.10f\n", matrixC.data[i], hostC[i]);
        assert(withinRange(matrixC.data[i], hostC[i], 0.0001f));
    }

    // Clean up
    clCode = clFlush(commands);
    clCode = clFinish(commands);
    clCode = clReleaseKernel(kernel);
    clCode = clReleaseProgram(program);
    clCode = clReleaseMemObject(dA);
    clCode = clReleaseMemObject(dB);
    clCode = clReleaseMemObject(dC);
    clCode = clReleaseCommandQueue(commands);
    clCode = clReleaseContext(context);
    free(hostA);
    free(hostB);
    free(hostC);
    cml_deleteMatrix(matrixA);
    cml_deleteMatrix(matrixB);
    cml_deleteMatrix(matrixC);

    cml_deleteDeviceArray(&gpus);
    return 0;
}

void fillRandom(float* data,const size_t count) {
    assert(data != NULL);
    for(size_t i = 0; i < count; i++) {
        data[i] = 2 * ((float) rand() / RAND_MAX) - 1;
    }
}

void copyFloatArray(float* src, float* dst, const size_t count) {
    for(size_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

bool withinRange(float a, float b, float range) {
    return fabs(a - b) <= range;
}
