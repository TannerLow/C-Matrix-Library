#include <cml/device/GPU.h>
#include <cdh/Logger.h>
#include <cml/ErrorCodes.h>
#include <cdh/Debug.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Should come last to override malloc family in debug mode
#include <cdh/Memory.h>

cml_DeviceArray cml_getGPUsWithCUDASupport(const uint8_t maxPlatforms, const uint8_t maxDevices) {
    cml_DeviceArray gpus;
    gpus.deviceIds = NULL;
    gpus.count = 0;

    // Devices (GPUs) are categorized under platforms (ex. CUDA)

    // Get platforms and search for CUDA
    printf("malloc space for platformIds\n");
    cl_platform_id* platformIds = (cl_platform_id*)malloc(sizeof(cl_platform_id) * maxPlatforms);
    printf("successfully malloc'd space for platformIds\n");
    cl_uint numberOfPlatforms;
    cdh_DEBUG(cdh_log("fetching GPU platform IDs\n"));
    cl_int clCode = clGetPlatformIDs(maxPlatforms, platformIds, &numberOfPlatforms);

    if(clCode != CL_SUCCESS) {
        cdh_crash(CML_CL_ERROR);
    }

    if(numberOfPlatforms == 0) {
        cdh_crash(CML_NO_PLATFORMS_FOUND);
    }

    cdh_DEBUG(cdh_log("clCode from clGetPlatformIDs: %d\n", clCode));
    cdh_DEBUG(cdh_log("# of platforms found: %d\n", (int)numberOfPlatforms));
    cdh_DEBUG(cdh_log("Platforms:\n"));

    int cudaPlatformIndex = -1;
    for(int i = 0; i < (int)numberOfPlatforms; i++) {
        char platformName[128];
        memset(platformName, 0, 128);
        clCode = clGetPlatformInfo(platformIds[i], CL_PLATFORM_NAME, 128, platformName, NULL);

        char platform_version[256];
        clGetPlatformInfo(platformIds[i], CL_PLATFORM_VERSION, sizeof(platform_version), platform_version, NULL);

        if(clCode != CL_SUCCESS) {
            cdh_DEBUG(cdh_log("clGetPlatformInfo failed with code %d\n", clCode));
            free(platformIds);
            cdh_crash(CML_CL_ERROR);
        }
        cdh_DEBUG(cdh_log("%s (OpenCL Version: %s)\n", platformName, platform_version));

        if(strcmp(platformName, "NVIDIA CUDA") == 0) {
            cudaPlatformIndex = i;
        }
    }
    cdh_DEBUG(cdh_log("\n"));

    if(cudaPlatformIndex < 0) {
        cdh_DEBUG(cdh_log("No platforms with name NVIDIA CUDA were found\n"));
        return gpus; // CUDA not found, return empty DeviceArray
    }


    cl_device_id* deviceIds = (cl_device_id*)malloc(sizeof(cl_device_id) * maxDevices);  
    cl_uint numberOfDevices;
    clCode = clGetDeviceIDs(platformIds[cudaPlatformIndex], CL_DEVICE_TYPE_GPU, maxDevices, deviceIds, &numberOfDevices);
    free(platformIds);

    cdh_DEBUG(cdh_log("clCode from clGetDeviceIDs: %d\n", clCode));
    cdh_DEBUG(cdh_log("# of devices found: %d\n", (int)numberOfDevices));
    cdh_DEBUG(cdh_log("Devices:\n"));

    if(clCode != CL_SUCCESS) {
        cdh_crash(CML_CL_ERROR);
    }

    if(numberOfDevices == 0) {
        cdh_crash(CML_NO_DEVICES_FOUND);
    }

    gpus.deviceIds = (cl_device_id*)malloc(sizeof(cl_device_id) * numberOfDevices);
    gpus.count = numberOfDevices;
    for(int i = 0; i < (int)numberOfDevices; i++) {
        char deviceName[128];
        memset(deviceName, 0, 128);
        clCode = clGetDeviceInfo(deviceIds[i], CL_DEVICE_NAME, 128, deviceName, NULL);

        if(clCode != CL_SUCCESS) {
            cdh_DEBUG(cdh_log("clGetPlatformInfo failed with code %d\n", clCode));
            free(deviceIds);
            free(gpus.deviceIds);
            cdh_crash(CML_CL_ERROR);
        }
        cdh_DEBUG(cdh_log("%s\n", deviceName));

        memcpy(gpus.deviceIds + i, deviceIds + i, sizeof(cl_device_id));
    }
    cdh_DEBUG(cdh_log("\n"));
    
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

    gpu.programMap = cml_createDynamicArray(5, sizeof(cl_program));
    gpu.kernelMap = cml_createDynamicArray(5, sizeof(cml_KernelMapEntry));
    gpu.buffers = cml_createDynamicArray(10, sizeof(cml_GPUBuffer));

    printf("cml_createGPU: creating context and command queue\n");

    cl_int clCode;
    gpu.gpu = device;
    gpu.context = clCreateContext(NULL, 1, &gpu.gpu, NULL, NULL, &clCode);
    assert(clCode == CL_SUCCESS);
    gpu.commands = clCreateCommandQueue(gpu.context, gpu.gpu, 0, &clCode);
    assert(clCode == CL_SUCCESS);

    printf("cml_createGPU: success\n");

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
    for(size_t i = 0; i < gpu->kernelMap.size; i++) {
        cl_kernel kernel = ((cml_KernelMapEntry*)cml_dynamicArrayGet(&gpu->kernelMap, i))->kernel;
        clCode = clReleaseKernel(kernel);
        assert(clCode == CL_SUCCESS);
    }
    cml_deleteDynamicArray(&gpu->kernelMap);

    for(size_t i = 0; i < gpu->programMap.size; i++) {
        cl_program program = *((cl_program*)cml_dynamicArrayGet(&gpu->programMap, i));
        clCode = clReleaseProgram(program);
        assert(clCode == CL_SUCCESS);
    }
    cml_deleteDynamicArray(&gpu->programMap);
    
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
        cdh_crash(clCode);
    }
}

cl_program cml_loadGPUProgram(cml_GPU* gpu, const cml_Program program) {
    assert(gpu != NULL);
    
    cl_int clCode;
    cl_program clProgram = clCreateProgramWithSource(gpu->context, 1, (const char**) &program.code, (const size_t*) &program.size, &clCode);
    assert(clCode == CL_SUCCESS);
    
    clCode = clBuildProgram(clProgram, 0, NULL, NULL, NULL, NULL);
    if(clCode != CL_SUCCESS) {
        size_t len;
        char buffer[2048];
        cdh_log("[ERROR] Failed to build kernel executable!\n");
        clGetProgramBuildInfo(clProgram, gpu->gpu, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        cdh_log("%s\n", buffer);
        cdh_crash(CML_CL_ERROR);
    }

    cml_dynamicArrayPush(&gpu->programMap, (void*)&clProgram);
    return clProgram;
}

void cml_createGPUKernel(cml_GPU* gpu, const cml_Kernel kernel) {
    assert(gpu != NULL);

    cl_int clCode;
    cml_KernelMapEntry mapEntry;
    mapEntry.kernalName = kernel.kernelName;
    mapEntry.kernel = clCreateKernel(kernel.program, kernel.kernelName, &clCode);
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
    printf("cml_simpleSetupGPU: fetching GPU\n");

    cml_DeviceArray deviceArray = cml_getGPUsWithCUDASupport(1, 1);
    assert(deviceArray.count > 0);
    cml_GPU gpu = cml_createGPU(deviceArray.deviceIds[0]);
    free(deviceArray.deviceIds);

    printf("cml_simpleSetupGPU: GPU found\n");

    // setup matrixMultiply gpu kernel
    FILE* programSource = fopen("matrix_multiply.cl", "r");
    cml_Program program = cml_createProgram(programSource);
    fclose(programSource);
    cl_program clProgram = cml_loadGPUProgram(&gpu, program);

    printf("cml_simpleSetupGPU: matrix_multiply.cl read\n");

    cml_Kernel kernel = cml_createKernel("matrixMultiply", clProgram);
    cml_createGPUKernel(&gpu, kernel);

    printf("cml_simpleSetupGPU: matrixMultiply loaded\n");

    //setup matrixAddRow gpu kernel
    programSource = fopen("matrix_add_row.cl", "r");
    program = cml_createProgram(programSource);
    fclose(programSource);
    clProgram = cml_loadGPUProgram(&gpu, program);

    printf("cml_simpleSetupGPU: matrix_add_row.cl read\n");

    kernel = cml_createKernel("matrixAddRow", clProgram);
    cml_createGPUKernel(&gpu, kernel);

    printf("cml_simpleSetupGPU: matrixAddRow loaded\n");

    //setup matrixRelu gpu kernel
    programSource = fopen("matrix_relu.cl", "r");
    program = cml_createProgram(programSource);
    fclose(programSource);
    clProgram = cml_loadGPUProgram(&gpu, program);

    printf("cml_simpleSetupGPU: matrix_relu.cl read\n");

    kernel = cml_createKernel("matrixRelu", clProgram);
    cml_createGPUKernel(&gpu, kernel);

    printf("cml_simpleSetupGPU: matrixRelu loaded\n");

    return gpu;
}
