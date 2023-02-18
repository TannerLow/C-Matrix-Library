#include <cml/device/GPU.h>
#include <cml/Logger.h>
#include <cml/ErrorCodes.h>
#include <cml/Debug.h>

#include <stdlib.h>
#include <string.h>
#include <assert.h>

DeviceArray cml_getGPUsWithCUDASupport(const uint8_t maxPlatforms, const uint8_t maxDevices) {
    DeviceArray gpus;
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

void cml_deleteDeviceArray(DeviceArray* deviceArray) {
    assert(deviceArray != NULL);
    assert(deviceArray->deviceIds != NULL);

    free(deviceArray->deviceIds);
    deviceArray->deviceIds = NULL;
    deviceArray->count = 0;
}