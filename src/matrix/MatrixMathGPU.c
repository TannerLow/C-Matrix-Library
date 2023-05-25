// how to get max number of work groups in openCL
// cl_ulong max_work_group_size;
// clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_work_group_size), &max_work_group_size, NULL);

// size_t max_total_work_group_size;
// clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(max_total_work_group_size), &max_total_work_group_size, NULL);

// cl_uint max_dims;
// clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(max_dims), &max_dims, NULL);

// size_t *max_work_group_size_per_dim = new size_t[max_dims];
// clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * max_dims, max_work_group_size_per_dim, NULL);

// //Output the maximum number of work groups
// std::cout << "Max number of work groups: " << (max_total_work_group_size / max_work_group_size_per_dim[0]) << std::endl;


#include <cml/matrix/MatrixMathGPU.h>

#include <CL/cl.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>

void cml_matrixMultiplyGPU(cml_GPU* gpu, const cml_Matrix* a, const cml_Matrix* b, cml_Matrix* out) {
    assert(gpu != NULL);
    assert(a   != NULL);
    assert(b   != NULL);
    assert(out != NULL);
    //printf("1\n");

    // find the desired kernel
    cl_kernel kernel = NULL;
    for (size_t i = 0; i < gpu->kernelMap.size; i++) {
        cml_KernelMapEntry mapEntry = *(cml_KernelMapEntry*)cml_dynamicArrayGet(&gpu->kernelMap, i);
        const char* kernelName = mapEntry.kernalName;

        if (strcmp(kernelName, "matrixMultiply") == 0) {
            kernel = mapEntry.kernel;
        }
    }
    //printf("2\n");
    assert(kernel != NULL);

    if(out->rows != a->rows || out->cols != b->cols) {
        // setup out matrix structure
        out->rows = a->rows;
        out->cols = b->cols;
        // FAILING, cant call realloc on non-malloc'd memory
        out->data = (float*)realloc(out->data, out->rows * out->cols * sizeof(float));
    }
    assert(out->data != NULL);
    //printf("3\n");

    // set kernel args
    int widthA   = a->cols;
    int widthOut = out->cols;

    // clean up memory allocated on the device
    cl_int clCode;
    for(size_t i = 0; i < gpu->buffers.size; i++) {
        cml_GPUBuffer buffer = *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, i);
        clCode = clReleaseMemObject(buffer);
        assert(clCode == CL_SUCCESS);
    }
    gpu->buffers.size = 0;
    //printf("4\n");

    // write matrices to buffers
    const cml_Matrix* matrices[3] = {a, b, out};
    for(size_t i = gpu->buffers.size; i < 3; i++) {
        cml_allocateGPUBuffer(gpu, matrices[i]->rows * matrices[i]->cols * sizeof(float)); // , (void*)matrices[i]->data);
    }
    //printf("5\n");
 
    clCode  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)cml_dynamicArrayGet(&gpu->buffers, 2)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)cml_dynamicArrayGet(&gpu->buffers, 0)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)cml_dynamicArrayGet(&gpu->buffers, 1)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 3, sizeof(int),    (void*)&widthA);
    clCode |= clSetKernelArg(kernel, 4, sizeof(int),    (void*)&widthOut); 
    assert(clCode == CL_SUCCESS);
    //printf("6\n");

    // set work size args
    size_t localWorkSize[2], globalWorkSize[2];
    localWorkSize[0] = 1;
    localWorkSize[1] = 1;
    globalWorkSize[0] = out->rows;
    globalWorkSize[1] = out->cols;

    // transfer data to GPU
    clCode |= clEnqueueWriteBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 0),
        CL_TRUE, 0,  a->cols * a->rows * sizeof(float), a->data,  0,  NULL,  NULL);
        assert(clCode == CL_SUCCESS);
    clCode |= clEnqueueWriteBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 1),
        CL_TRUE, 0,  b->cols * b->rows * sizeof(float), b->data,  0,  NULL,  NULL);
        assert(clCode == CL_SUCCESS);
    clCode |= clEnqueueWriteBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 2),
        CL_TRUE, 0,  out->cols * out->rows * sizeof(float), out->data,  0,  NULL,  NULL);
    assert(clCode == CL_SUCCESS);
    //printf("7\n");

    // run job
    clCode = clEnqueueNDRangeKernel(gpu->commands, kernel, 2, NULL, globalWorkSize, localWorkSize, 
    0, NULL, NULL);
    assert(clCode == CL_SUCCESS);
    //printf("8\n");

    // read to output matrix
    clCode = clEnqueueReadBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 2), 
        CL_TRUE, 0,  out->cols * out->rows * sizeof(float), out->data, 0, NULL, NULL);
    assert(clCode == CL_SUCCESS);
    //printf("9\n");
}

void cml_matrixAddRowGPU(cml_GPU* gpu, const cml_Matrix matrix, const cml_Matrix row, cml_Matrix* out) {
    assert(gpu != NULL);
    assert(matrix.data != NULL);
    assert(row.data != NULL);
    assert(matrix.cols == row.cols);
    assert(out->data != NULL);

    // find the desired kernel
    cl_kernel kernel = NULL;
    for (size_t i = 0; i < gpu->kernelMap.size; i++) {
        cml_KernelMapEntry mapEntry = *(cml_KernelMapEntry*)cml_dynamicArrayGet(&gpu->kernelMap, i);
        const char* kernelName = mapEntry.kernalName;

        if (strcmp(kernelName, "matrixAddRow") == 0) {
            kernel = mapEntry.kernel;
        }
    }
    assert(kernel != NULL);

    if(out->rows != matrix.rows || out->cols != matrix.cols) {
        // setup out matrix structure
        out->rows = matrix.rows;
        out->cols = matrix.cols;
        out->data = (float*)realloc(out->data, out->rows * out->cols * sizeof(float));
    }
    assert(out->data != NULL);

    // set kernel args
    int columns = matrix.cols;

    // clean up memory allocated on the device
    cl_int clCode;
    for(size_t i = 0; i < gpu->buffers.size; i++) {
        cml_GPUBuffer buffer = *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, i);
        clCode = clReleaseMemObject(buffer);
        assert(clCode == CL_SUCCESS);
    }
    gpu->buffers.size = 0;

    // write matrices to buffers
    const cml_Matrix* matrices[3] = {&matrix, &row, out};
    for(size_t i = gpu->buffers.size; i < 3; i++) {
        cml_allocateGPUBuffer(gpu, matrices[i]->rows * matrices[i]->cols * sizeof(float));
    }

    clCode  = clSetKernelArg(kernel, 0, sizeof(cl_mem), cml_dynamicArrayGet(&gpu->buffers, 0)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 1, sizeof(cl_mem), cml_dynamicArrayGet(&gpu->buffers, 1)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 2, sizeof(cl_mem), cml_dynamicArrayGet(&gpu->buffers, 2)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 3, sizeof(int),    (void*)&columns);
    assert(clCode == CL_SUCCESS);

    // set work size args
    const size_t localWorkSize = 1; // TODO change to a dynamic or something to allow chunking of columns
    const size_t globalWorkSize = out->rows;

    // transfer data to GPU
    clCode |= clEnqueueWriteBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 0),
        CL_TRUE, 0,  matrix.cols * matrix.rows * sizeof(float), matrix.data,  0,  NULL,  NULL);
        assert(clCode == CL_SUCCESS);
    clCode |= clEnqueueWriteBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 1),
        CL_TRUE, 0,  row.cols * sizeof(float), row.data,  0,  NULL,  NULL);
        assert(clCode == CL_SUCCESS);
    clCode |= clEnqueueWriteBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 2),
        CL_TRUE, 0,  out->cols * out->rows * sizeof(float), out->data,  0,  NULL,  NULL);
    assert(clCode == CL_SUCCESS);

    // run job
    clCode = clEnqueueNDRangeKernel(gpu->commands, kernel, 1, NULL, &globalWorkSize, &localWorkSize, 
    0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    // read to output matrix
    clCode = clEnqueueReadBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 2), 
        CL_TRUE, 0,  out->cols * out->rows * sizeof(float), out->data, 0, NULL, NULL);
    assert(clCode == CL_SUCCESS);
}

void cml_matrixReluGPU(cml_GPU* gpu, const cml_Matrix matrix, cml_Matrix* out) {
    assert(gpu != NULL);
    assert(matrix.data != NULL);
    assert(matrix.rows == out->rows);
    assert(matrix.cols == out->cols);
    assert(out->data != NULL);

    // find the desired kernel
    cl_kernel kernel = NULL;
    for (size_t i = 0; i < gpu->kernelMap.size; i++) {
        cml_KernelMapEntry mapEntry = *(cml_KernelMapEntry*)cml_dynamicArrayGet(&gpu->kernelMap, i);
        const char* kernelName = mapEntry.kernalName;

        if (strcmp(kernelName, "matrixRelu") == 0) {
            kernel = mapEntry.kernel;
        }
    }
    assert(kernel != NULL);

    if(out->rows != matrix.rows || out->cols != matrix.cols) {
        // setup out matrix structure
        out->rows = matrix.rows;
        out->cols = matrix.cols;
        out->data = (float*)realloc(out->data, out->rows * out->cols * sizeof(float));
    }
    assert(out->data != NULL);

    // clean up memory allocated on the device
    cl_int clCode;
    for(size_t i = 0; i < gpu->buffers.size; i++) {
        cml_GPUBuffer buffer = *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, i);
        clCode = clReleaseMemObject(buffer);
        assert(clCode == CL_SUCCESS);
    }
    gpu->buffers.size = 0;

    // write matrices to buffers
    const cml_Matrix* matrices[2] = {&matrix, out};
    for(size_t i = gpu->buffers.size; i < 2; i++) {
        cml_allocateGPUBuffer(gpu, matrices[i]->rows * matrices[i]->cols * sizeof(float));
    }

    clCode  = clSetKernelArg(kernel, 0, sizeof(cl_mem), cml_dynamicArrayGet(&gpu->buffers, 0)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 1, sizeof(cl_mem), cml_dynamicArrayGet(&gpu->buffers, 1)); assert(clCode == CL_SUCCESS);
    assert(clCode == CL_SUCCESS);

    // set work size args
    const size_t localWorkSize = 1; // TODO change to a dynamic or something to allow chunking of columns
    const size_t globalWorkSize = matrix.rows * matrix.cols;

    // transfer data to GPU
    clCode |= clEnqueueWriteBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 0),
        CL_TRUE, 0,  matrix.cols * matrix.rows * sizeof(float), matrix.data,  0,  NULL,  NULL);
        assert(clCode == CL_SUCCESS);
    clCode |= clEnqueueWriteBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 1),
        CL_TRUE, 0,  out->cols * out->rows * sizeof(float), out->data,  0,  NULL,  NULL);
    assert(clCode == CL_SUCCESS);

    // run job
    clCode = clEnqueueNDRangeKernel(gpu->commands, kernel, 1, NULL, &globalWorkSize, &localWorkSize, 
    0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    // read to output matrix
    clCode = clEnqueueReadBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 1), 
        CL_TRUE, 0,  out->cols * out->rows * sizeof(float), out->data, 0, NULL, NULL);
    assert(clCode == CL_SUCCESS);
}