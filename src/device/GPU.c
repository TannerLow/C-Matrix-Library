#include <cml/device/GPU.h>
#include <cml/Logger.h>
#include <cml/ErrorCodes.h>
#include <cml/Debug.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

cml_DeviceArray cml_getGPUsWithCUDASupport(const uint8_t maxPlatforms, const uint8_t maxDevices) {
    cml_DeviceArray gpus;
    gpus.deviceIds = NULL;
    gpus.count = 0;

    // Devices (GPUs) are categorized under platforms (ex. CUDA)

    // Get platforms and search for CUDA
    cl_platform_id* platformIds = (cl_platform_id*)malloc(sizeof(cl_platform_id) * maxPlatforms);
    cl_uint numberOfPlatforms;
    cl_int clCode = clGetPlatformIDs(maxPlatforms, platformIds, &numberOfPlatforms);

    if(clCode != CL_SUCCESS) {
        cml_crash(CML_CL_ERROR);
    }

    if(numberOfPlatforms == 0) {
        cml_crash(CML_NO_PLATFORMS_FOUND);
    }

    cml_DEBUG(fprintf(cml_logStream, "clCode from clGetPlatformIDs: %d\n", clCode));
    cml_DEBUG(fprintf(cml_logStream, "# of platforms found: %d\n", (int)numberOfPlatforms));
    cml_DEBUG(fprintf(cml_logStream, "Platforms:\n"));

    int cudaPlatformIndex = -1;
    for(int i = 0; i < (int)numberOfPlatforms; i++) {
        char platformName[128];
        memset(platformName, 0, 128);
        clCode = clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, 128, platformName, NULL);
        
        if(clCode != CL_SUCCESS) {
            cml_DEBUG(fprintf(cml_logStream, "clGetPlatformInfo failed with code %d\n", clCode));
            free(platformIds);
            cml_crash(CML_CL_ERROR);
        }
        cml_DEBUG(fprintf(cml_logStream, "%s\n", platformName));

        if(strcmp(platformName, "NVIDIA CUDA") == 0) {
            cudaPlatformIndex = i;
        }
    }
    cml_DEBUG(fprintf(cml_logStream, "\n"));

    if(cudaPlatformIndex < 0) {
        cml_DEBUG(fprintf(cml_logStream, "No platforms with name NVIDIA CUDA were found\n"));
        return gpus; // CUDA not found, return empty DeviceArray
    }


    cl_device_id* deviceIds = (cl_device_id*)malloc(sizeof(cl_device_id) * maxDevices);  
    cl_uint numberOfDevices;
    clCode = clGetDeviceIDs(platformIds[cudaPlatformIndex], CL_DEVICE_TYPE_GPU, maxDevices, deviceIds, &numberOfDevices);
    free(platformIds);

    cml_DEBUG(fprintf(cml_logStream, "clCode from clGetDeviceIDs: %d\n", clCode));
    cml_DEBUG(fprintf(cml_logStream, "# of devices found: %d\n", (int)numberOfDevices));
    cml_DEBUG(fprintf(cml_logStream, "Devices:\n"));

    if(clCode != CL_SUCCESS) {
        cml_crash(CML_CL_ERROR);
    }

    if(numberOfDevices == 0) {
        cml_crash(CML_NO_DEVICES_FOUND);
    }

    gpus.deviceIds = (cl_device_id*)malloc(sizeof(cl_device_id) * numberOfDevices);
    gpus.count = numberOfDevices;
    for(int i = 0; i < (int)numberOfDevices; i++) {
        char deviceName[128];
        memset(deviceName, 0, 128);
        clCode = clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME, 128, deviceName, NULL);

        if(clCode != CL_SUCCESS) {
            cml_DEBUG(fprintf(cml_logStream, "clGetPlatformInfo failed with code %d\n", clCode));
            free(deviceIds);
            free(gpus.deviceIds);
            cml_crash(CML_CL_ERROR);
        }
        cml_DEBUG(fprintf(cml_logStream, "%s\n", deviceName));

        memcpy(gpus.deviceIds + i, deviceIds + i, sizeof(cl_device_id));
    }
    cml_DEBUG(fprintf(cml_logStream, "\n"));
    
    free(deviceIds);

    return gpus;
}

void cml_deleteDeviceArray(cml_DeviceArray* deviceArray) {
    assert(deviceArray != NULL);
    assert(deviceArray->deviceIds != NULL);

    free(deviceArray->deviceIds);
    deviceArray->deviceIds = NULL;
    deviceArray->count = 0;
}

cml_GPU cml_createGPU(cl_device_id device) {
    assert(device != NULL);

    cml_GPU gpu;

    gpu.kernelMap = cml_createDynamicArray(5, sizeof(cml_KernelMapEntry));
    gpu.buffers = cml_createDynamicArray(10, sizeof(cml_GPUBuffer));

    cl_int clCode;
    gpu.gpu = device;
    gpu.context = clCreateContext(NULL, 1, &gpu.gpu, NULL, NULL, &clCode);
    assert(clCode == CL_SUCCESS);
    gpu.commands = clCreateCommandQueue(gpu.context, gpu.gpu, 0, &clCode);
    assert(clCode == CL_SUCCESS);
    gpu.programLoaded = false;

    return gpu;
}

void cml_deleteGPU(cml_GPU* gpu) {
    assert(gpu != NULL);

    cl_int clCode;

    // clean the command queue
    clCode = clFlush(gpu->commands);
    assert(clCode == CL_SUCCESS);
    clCode = clFinish(gpu->commands);
    assert(clCode == CL_SUCCESS);

    // clean up program and kernels
    if(gpu->programLoaded) {
        for(size_t i = 0; i < gpu->kernelMap.size; i++) {
            cl_kernel kernel = ((cml_KernelMapEntry*)cml_dynamicArrayGet(&gpu->kernelMap, i))->kernel;
            clCode = clReleaseKernel(kernel);
            assert(clCode == CL_SUCCESS);
        }
        clCode = clReleaseProgram(gpu->program);
        assert(clCode == CL_SUCCESS);
        gpu->programLoaded = false;
    }
    
    // clean up memory allocated on the device
    for(size_t i = 0; i < gpu->buffers.size; i++) {
        cml_GPUBuffer buffer = *(cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, i);
        clCode = clReleaseMemObject(buffer);
        assert(clCode == CL_SUCCESS);
    }
    gpu->buffers.size = 0;

    // finish the clean up
    clCode = clReleaseCommandQueue(gpu->commands);
    assert(clCode == CL_SUCCESS);
    clCode = clReleaseContext(gpu->context);
    assert(clCode == CL_SUCCESS);

    if(clCode != CL_SUCCESS) {
        cml_crash(clCode);
    }
}

void cml_loadGPUProgram(cml_GPU* gpu, const cml_Program program) {
    assert(gpu != NULL);
    
    cl_int clCode;
    cl_program clProgram = clCreateProgramWithSource(gpu->context, 1, (const char**) &program.code, (const size_t*) &program.size, &clCode);
    assert(clCode == CL_SUCCESS);
    
    clCode = clBuildProgram(clProgram, 0, NULL, NULL, NULL, NULL);
    if(clCode != CL_SUCCESS) {
        size_t len;
        char buffer[2048];
        fprintf(cml_logStream, "[ERROR] Failed to build kernel executable!\n");
        clGetProgramBuildInfo(clProgram, gpu->gpu, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        fprintf(cml_logStream, "%s\n", buffer);
        cml_crash(CML_CL_ERROR);
    }

    gpu->program = clProgram;
    gpu->programLoaded = true;
}

void cml_createGPUKernel(cml_GPU* gpu, const cml_Kernel kernel) {
    assert(gpu != NULL);

    cl_int clCode;
    cml_KernelMapEntry mapEntry;
    mapEntry.kernalName = kernel.kernelName;
    mapEntry.kernel = clCreateKernel(gpu->program, kernel.kernelName, &clCode);
    assert(mapEntry.kernel && clCode == CL_SUCCESS);

    cml_dynamicArrayPush(&gpu->kernelMap, (void*)&mapEntry);
}

cml_GPUBuffer* cml_allocateGPUBuffer(cml_GPU* gpu, size_t size) {
    assert(gpu != NULL);
    // assert(hostPtr != NULL);

    cl_int clCode;
    cml_GPUBuffer buffer;
    buffer = clCreateBuffer(gpu->context, CL_MEM_READ_WRITE, size, NULL, &clCode);
    assert(clCode == CL_SUCCESS);

    size_t index = cml_dynamicArrayPush(&gpu->buffers, (void*)&buffer);
    return (cml_GPUBuffer*)cml_dynamicArrayGet(&gpu->buffers, index);
}

cml_GPU cml_simpleSetupGPU() {
    cml_DeviceArray deviceArray = cml_getGPUsWithCUDASupport(1, 1);
    assert(deviceArray.count > 0);
    cml_GPU gpu = cml_createGPU(deviceArray.deviceIds[0]);
    free(deviceArray.deviceIds);

    FILE* programSource = fopen("matrix_multiply.cl", "r");
    cml_Program program = cml_createProgram(programSource);
    fclose(programSource);
    cml_loadGPUProgram(&gpu, program);

    cml_Kernel kernel = cml_createKernel("matrixMultiply", &program);
    cml_createGPUKernel(&gpu, kernel);

    return gpu;
}