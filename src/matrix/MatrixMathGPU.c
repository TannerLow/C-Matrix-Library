// #include <cml/matrix/MatrixMathGPU.h>
// #include <cml/Logger.h>
// #include <cml/ErrorCodes.h>
// #include <cml/util/KernelArg.h>

// #include <assert.h>
// #include <stdio.h>


// static enum cml_State cml_state = CML_UNINITIALIZED;

// static cl_context cml_context;
// static cl_kernel cml_clKernel;
// static cl_command_queue cml_commands;
// static cl_mem* cml_gpuBuffers;

// void cml_setupGPU(cl_device_id gpu, const cml_Kernel kernel) {
//     assert(cml_state == CML_UNINITIALIZED);

//     cl_int clCode;
//     cml_context = clCreateContext(NULL, 1, &gpu, NULL, NULL, &clCode);
//     assert(clCode == CL_SUCCESS);
//     cml_commands = clCreateCommandQueue(cml_context, gpu, 0, &clCode);
//     assert(clCode == CL_SUCCESS);


//     cl_program program = clCreateProgramWithSource(cml_context, 1,  &kernel.code, (const size_t*) &kernel.size, &clCode);
//     assert(clCode == CL_SUCCESS);
    
//     clCode = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
//     if(clCode != CL_SUCCESS) {
//         size_t len;
//         char buffer[2048];
//         fprintf(cml_logStream, "[ERROR] Failed to build kernel executable!\n");
//         clGetProgramBuildInfo(program, gpu, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
//         fprintf(cml_logStream, "%s\n", buffer);
//         cml_crash(CML_CL_ERROR);
//     }

//     //fprintf(cml_logStream, "Running matrix multiplication for matrices A (%dx%d) and B (%dx%d) ...\n", widthA, heightA, widthB, heightB);

//     cml_clKernel = clCreateKernel(program, kernel.name, &clCode);
//     assert(cml_clKernel && clCode == CL_SUCCESS);
    
//     cml_state = CML_PRE_BUFFERS;
// }

// void cml_setupGPUBuffers(const cml_MatrixArray bufferData) {
//     assert(cml_state == CML_PRE_BUFFERS);

//     // Transfer data to GPU
//     cl_int clCode;
//     cml_gpuBuffers = (cl_mem*)malloc(sizeof(cl_mem) * bufferData.count);

//     for(size_t i = 0; i < bufferData.count; i++) {
//         // size_t memSize = sizeof(float) * bufferData.matrices[i]->cols * bufferData.matrices[i]->rows;
//         cml_gpuBuffers[i] = clCreateBuffer(cml_context, CL_MEM_READ_WRITE, 0/*memSize*/, NULL, &clCode);
//         assert(clCode == CL_SUCCESS);
//     }

//     cml_state = CML_PRE_KERNEL_ARGS;
// }

// void cml_setKernelArgs(cml_KernelArg* kernelArgs, const size_t count) {
//     assert(cml_state == CML_PRE_KERNEL_ARGS);

//     cl_int clCode = CL_SUCCESS;
//     for(size_t i = 0; i < count; i++) {
//         clCode |= clSetKernelArg(cml_clKernel, i, kernelArgs[i].size, kernelArgs[i].data);
//     }
//     assert(clCode == CL_SUCCESS);

//     cml_state = CML_READY;
// }

// void cml_matrixMultiplyGPU(const cml_Matrix* a, const cml_Matrix* b, cml_Matrix* c) {
//     assert(cml_state == CML_READY);
//     assert(a && b && c);

//     // Run the kernel on the GPU
//     size_t localWorkSize[2], globalWorkSize[2];
//     localWorkSize[0] = 16;
//     localWorkSize[1] = 16;
//     globalWorkSize[0] = a->cols;
//     globalWorkSize[1] = b->cols;

//     cl_int clCode;
//     clCode = clEnqueueWriteBuffer(cml_commands, cml_gpuBuffers[0], CL_TRUE, 0, sizeof(float) * a->cols * a->rows, a->data, 0, NULL, NULL);
//     clCode = clEnqueueWriteBuffer(cml_commands, cml_gpuBuffers[1], CL_TRUE, 0, sizeof(float) * b->cols * b->rows, b->data, 0, NULL, NULL);
//     free(c->data);
//     c->data = clEnqueueMapBuffer(cml_commands, cml_gpuBuffers[2], CL_TRUE, CL_MAP_READ, 0, sizeof(float) * c->cols * c->rows, 0, NULL, NULL, &clCode);
//     assert(clCode == CL_SUCCESS);

//     clCode = clEnqueueNDRangeKernel(cml_commands, cml_clKernel, 2, NULL, globalWorkSize, localWorkSize, 
//     0, NULL, NULL);
//     assert(clCode == CL_SUCCESS);

//     // Copy back result to host memory
//     clCode = clEnqueueReadBuffer(cml_commands, cml_gpuBuffers[2], CL_TRUE, 0, sizeof(float) * c->cols * c->rows, c->data, 0, NULL, NULL);
//     assert(clCode == CL_SUCCESS);
// }

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

    // find the desired kernel
    cl_kernel kernel = NULL;
    for (size_t i = 0; i < gpu->kernelMap.size; i++) {
        cml_KernelMapEntry mapEntry = *(cml_KernelMapEntry*)cml_dynamicArrayGet(&gpu->kernelMap, i);
        const char* kernelName = mapEntry.kernalName;

        if (strcmp(kernelName, "matrixMultiply") == 0) {
            kernel = mapEntry.kernel;
        }
    }
    assert(kernel != NULL);

    // setup out matrix structure
    out->rows = a->rows;
    out->cols = b->cols;
    out->data = (float*)realloc(out->data, out->rows * out->cols * sizeof(float));
    assert(out->data != NULL);

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

    // write matrices to buffers
    const cml_Matrix* matrices[3] = {a, b, out};
    for(size_t i = gpu->buffers.size; i < 3; i++) {
        cml_allocateGPUBuffer(gpu, matrices[i]->rows * matrices[i]->cols * sizeof(float)); // , (void*)matrices[i]->data);
    }
 
    clCode  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)cml_dynamicArrayGet(&gpu->buffers, 2)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)cml_dynamicArrayGet(&gpu->buffers, 0)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)cml_dynamicArrayGet(&gpu->buffers, 1)); assert(clCode == CL_SUCCESS);
    clCode |= clSetKernelArg(kernel, 3, sizeof(int),    (void*)&widthA);
    clCode |= clSetKernelArg(kernel, 4, sizeof(int),    (void*)&widthOut); 
    assert(clCode == CL_SUCCESS);

    // set work size args
    size_t localWorkSize[2], globalWorkSize[2];
    localWorkSize[0] = 2;
    localWorkSize[1] = 2;
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

    // run job
    clCode = clEnqueueNDRangeKernel(gpu->commands, kernel, 2, NULL, globalWorkSize, localWorkSize, 
    0, NULL, NULL);
    assert(clCode == CL_SUCCESS);

    // read to output matrix
    clCode = clEnqueueReadBuffer(gpu->commands, *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, 2), 
        CL_TRUE, 0,  out->cols * out->rows * sizeof(float), out->data, 0, NULL, NULL);
    assert(clCode == CL_SUCCESS);
}