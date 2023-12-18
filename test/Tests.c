#include "Tests.h"
#include "TestUtils.h"

#include <cdh/Logger.h>
#include <cml/device/GPU.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// OpenCL reference pages: https://man.opencl.org/
#ifdef __APPLE__
#include <CL/opencl.h>
#else
#include <CL/cl.h>
#endif

// Should come last to override malloc family in debug mode
#include <cdh/Memory.h>

#define TESTCASE(test) (test == TEST_SUCCESS)

bool cml_test_runAllTests() {
    ANNOUNCE_TEST("cml_test_runAllTests");

    bool cml_test_getGpusSuccess = TESTCASE(cml_test_getGpus());

    cdh_log("[TEST] cml_test_GetGpus: %s\n", cml_test_getGpusSuccess ? "SUCCESS" : "FAILURE");

    return cml_test_getGpusSuccess;
}

int cml_test_getGpus() {
    ANNOUNCE_TEST("cml_test_getGpus");
    cml_DeviceArray gpus = cml_getGPUsWithCUDASupport(5, 5);

    cdh_log("Devices again:\n");
    for(int i = 0; i < (int)gpus.count; i++) {
        char deviceName[128];
        memset(deviceName, 0, 128);
        clGetDeviceInfo(gpus.deviceIds[i], CL_DEVICE_NAME, 128, deviceName, NULL);
        cdh_log("%s\n", deviceName);
    }

    free(gpus.deviceIds);

    return TEST_SUCCESS;
}