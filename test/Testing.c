// OpenCL error code map: https://gist.github.com/bmount/4a7144ce801e5569a0b6

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
void printMatrix(const cml_Matrix* matrix);

int unabstractedMatMul();
int abstractedMatMul();
int simpleAbstractedMatMul();
int simpleAbstractedMatAddRow();
int simpleAbstractedMatRelu();

int driver() {
    return simpleAbstractedMatMul();
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

int unabstractedMatMul() {
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
    cml_DeviceArray gpus = cml_getGPUsWithCUDASupport(5, 5);
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
 
    fp = fopen("matrix_multiply.cl", "r");
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

    fprintf(cml_logStream, "Running matrix multiplication for matrices A (%dx%d) and B (%dx%d) ...\n", widthA, heightA, widthB, heightB);

    cl_kernel kernel = clCreateKernel(program, "matrixMultiply", &clCode);
    assert(kernel && clCode == CL_SUCCESS);

    // Transfer data to GPU
    dA = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, memSizeA, hostA, &clCode);
    dB = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, memSizeB, hostB, &clCode);
    dC = clCreateBuffer(context, CL_MEM_READ_WRITE, memSizeA, NULL, &clCode);
    assert(dA && dB && dC); 

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
    globalWorkSize[0] = widthA;
    globalWorkSize[1] = widthB;

    clCode = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, globalWorkSize, localWorkSize, 
    0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    // Copy back result to host memory
    clCode = clEnqueueReadBuffer(commands, dC, CL_TRUE, 0, memSizeC, hostC, 0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    fprintf(cml_logStream, "Done running matrix multiplication\n");

    /////////////////////////////////////////////////////////

    fprintf(cml_logStream, "Start CPU multiplication\n");
    cml_matrixMultiply(matrixA, matrixB, &matrixC);
    fprintf(cml_logStream, "done\n");
    for(size_t i = 0; i < (size_t)sizeA; i++) {
        // fprintf(cml_logStream, "%.10f %.10f\n", matrixC.data[i], hostC[i]);
        assert(withinRange(matrixC.data[i], hostC[i], 0.001f));
    }

    /////////////////////////////////////////////////////////

    fprintf(cml_logStream, "Running another matrix multiplication\n");

    // Host memory (non-GPU stuff)
    const unsigned int sizeD = widthA * heightA;
    const unsigned int memSizeD = sizeof(float) * sizeD;
    float* hostD = (float*)malloc(memSizeD);
    assert(hostD != NULL);

    for(unsigned int i = 0; i < sizeD; i++) {
        hostD[i] = 0.001f;
    }

    clCode = clEnqueueWriteBuffer(commands, dB, CL_TRUE, 0, memSizeD, hostD, 0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    clCode = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, globalWorkSize, localWorkSize, 
    0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    // Copy back result to host memory
    clCode = clEnqueueReadBuffer(commands, dC, CL_TRUE, 0, memSizeC, hostC, 0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    fprintf(cml_logStream, "done\n");

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
    free(hostD);
    cml_deleteMatrix(matrixA);
    cml_deleteMatrix(matrixB);
    cml_deleteMatrix(matrixC);

    cml_deleteDeviceArray(&gpus);
    return 0;
}

#include <cml/matrix/MatrixMathGPU.h>
#include <cml/device/GPU.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

int abstractedMatMul() {
    cml_DeviceArray deviceArray = cml_getGPUsWithCUDASupport(3, 3);
    assert(deviceArray.count > 0);
    cml_GPU gpu = cml_createGPU(deviceArray.deviceIds[0]);
    free(deviceArray.deviceIds);

    FILE* programSource = fopen("matrix_multiply.cl", "r");
    cml_Program program = cml_createProgram(programSource);
    cl_program programId = cml_loadGPUProgram(&gpu, program);

    cml_Kernel kernel = cml_createKernel("matrixMultiply", programId);
    cml_createGPUKernel(&gpu, kernel);

    cml_Matrix a   = cml_createMatrix(2, 3);
    cml_Matrix b   = cml_createMatrix(3, 2);
    cml_Matrix out = cml_createMatrix(2, 2);

    float aData[] = {1,2,3,4,5,6};
    float bData[] = {0,1,0,1,0,1};
    memcpy(a.data, aData, 6 * sizeof(float));
    memcpy(b.data, bData, 6 * sizeof(float));
    printMatrix(&a);
    printMatrix(&b);

    cml_matrixMultiplyGPU(&gpu, &a, &b, &out);

    printMatrix(&out);

    // Do it again
    cml_Matrix c   = cml_createMatrix(4, 1);
    cml_Matrix d   = cml_createMatrix(1, 4);
    cml_Matrix out2 = cml_createMatrix(4, 4);

    float cData[] = {1,2,3,4};
    float dData[] = {10,9,8,7};
    memcpy(c.data, cData, 4 * sizeof(float));
    memcpy(d.data, dData, 4 * sizeof(float));
    printMatrix(&c);
    printMatrix(&d);

    cml_matrixMultiplyGPU(&gpu, &c, &d, &out2);

    printMatrix(&out2);

    cml_deleteGPU(&gpu);
    cml_deleteMatrix(a);
    cml_deleteMatrix(b);
    cml_deleteMatrix(out);
    cml_deleteMatrix(c);
    cml_deleteMatrix(d);
    cml_deleteMatrix(out2);

    printf("That's crazy!\n");
    return 0;
}

int simpleAbstractedMatMul() {
    cml_GPU gpu = cml_simpleSetupGPU();

    cml_Matrix a   = cml_createMatrix(2, 3);
    cml_Matrix b   = cml_createMatrix(3, 2);
    cml_Matrix out = cml_createMatrix(2, 2);

    float aData[] = {2,3,4,5,6,7};
    float bData[] = {1,1,1,2,1,3};
    memcpy(a.data, aData, 6 * sizeof(float));
    memcpy(b.data, bData, 6 * sizeof(float));
    printMatrix(&a);
    printMatrix(&b);

    cml_matrixMultiplyGPU(&gpu, &a, &b, &out);

    printMatrix(&out);

    // Do it again
    cml_Matrix c   = cml_createMatrix(4, 1);
    cml_Matrix d   = cml_createMatrix(1, 4);
    cml_Matrix out2 = cml_createMatrix(4, 4);

    float cData[] = {10,9,8,7};
    float dData[] = {1,2,3,4};
    memcpy(c.data, cData, 4 * sizeof(float));
    memcpy(d.data, dData, 4 * sizeof(float));
    printMatrix(&c);
    printMatrix(&d);

    cml_matrixMultiplyGPU(&gpu, &c, &d, &out2);

    printMatrix(&out2);

    // Do it again
    cml_Matrix e   = cml_createMatrix(1, 3);
    cml_Matrix f   = cml_createMatrix(3, 2);
    cml_Matrix out3 = cml_createMatrix(1, 2);

    float eData[] = {0.5f,0.2f,0.3f};
    float fData[] = {1,2,3,4,5,6};
    memcpy(e.data, eData, 3 * sizeof(float));
    memcpy(f.data, fData, 6 * sizeof(float));
    printMatrix(&e);
    printMatrix(&f);

    cml_matrixMultiplyGPU(&gpu, &e, &f, &out3);

    printMatrix(&out3);

    cml_deleteGPU(&gpu);
    cml_deleteMatrix(a);
    cml_deleteMatrix(b);
    cml_deleteMatrix(out);
    cml_deleteMatrix(c);
    cml_deleteMatrix(d);
    cml_deleteMatrix(out2);
    cml_deleteMatrix(e);
    cml_deleteMatrix(f);
    cml_deleteMatrix(out3);

    printf("That's crazy simple!\n");
    return 0;
}

int simpleAbstractedMatAddRow() {
    cml_GPU gpu = cml_simpleSetupGPU();

    cml_Matrix matrix1 = cml_createMatrix(3, 2);
    cml_Matrix row1    = cml_createMatrix(1, 2);
    cml_Matrix out1    = cml_createMatrix(3, 2);

    float matrix1Data[] = {1,2,3,4,5,6};
    float row1Data[] = {1,2};
    memcpy(matrix1.data, matrix1Data, 6 * sizeof(float));
    memcpy(row1.data, row1Data, 2 * sizeof(float));
    printMatrix(&matrix1);
    printMatrix(&row1);

    cml_matrixAddRowGPU(&gpu, matrix1, row1, &out1);

    printMatrix(&out1);

    // Do it again
    cml_Matrix matrix2 = cml_createMatrix(2, 3);
    cml_Matrix row2    = cml_createMatrix(1, 3);
    cml_Matrix out2    = cml_createMatrix(2, 3);

    float matrix2Data[] = {0,1,1,1,1,0};
    float row2Data[] = {1,2,1};
    memcpy(matrix2.data, matrix2Data, 6 * sizeof(float));
    memcpy(row2.data, row2Data, 3 * sizeof(float));
    printMatrix(&matrix2);
    printMatrix(&row2);

    cml_matrixAddRowGPU(&gpu, matrix2, row2, &out2);

    printMatrix(&out2);

    cml_deleteGPU(&gpu);
    cml_deleteMatrix(matrix1);
    cml_deleteMatrix(row1);
    cml_deleteMatrix(out1);
    cml_deleteMatrix(matrix2);
    cml_deleteMatrix(row2);
    cml_deleteMatrix(out2);

    printf("That's crazy simple!\n");
    return 0;
}

int simpleAbstractedMatRelu() {
    cml_GPU gpu = cml_simpleSetupGPU();

    cml_Matrix matrix = cml_createMatrix(1, 3);
    cml_Matrix out    = cml_createMatrix(1, 3);

    float matrixData[] = {1,0,-1};
    memcpy(matrix.data, matrixData, 3 * sizeof(float));
    printMatrix(&matrix);

    cml_matrixReluGPU(&gpu, matrix, &out);

    printMatrix(&out);

    cml_deleteGPU(&gpu);
    cml_deleteMatrix(matrix);
    cml_deleteMatrix(out);

    printf("That's crazy simple!\n");
    return 0;
}

void printMatrix(const cml_Matrix* matrix) {
    assert(matrix);
    for(size_t row = 0; row < matrix->rows; row++) {
        for(size_t col = 0; col < matrix->cols; col++) {
            fprintf(cml_logStream, "%0.4f ", matrix->data[row * matrix->cols + col]);
        }
        fprintf(cml_logStream, "\n");
    }
    fprintf(cml_logStream, "\n");
}