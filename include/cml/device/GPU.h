#ifndef CML_GPU_H
#define CML_GPU_H

#include <CL/cl.h>

#include <stdint.h>

typedef struct {
    cl_device_id* deviceIds;
    size_t count;
} DeviceArray;

DeviceArray getGPUsWithCUDASupport(const uint8_t maxPlatforms, const uint8_t maxDevices);

#endif // CML_GPU_H