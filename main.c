#include "example/VectorAddExample.h"

#include <stdio.h>

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

int main(void) {
    // cml_example_runVectorAdd();
    cl_platform_id* platform_ids = (cl_platform_id*)malloc(sizeof(cl_platform_id) * 5);
    cl_uint ret_num_platforms;
    cl_int ret = clGetPlatformIDs(5, platform_ids, &ret_num_platforms);

    printf("clGetPlatformIDs: %d\n", ret);
    printf("# of platforms found: %d\n", (int)ret_num_platforms);
    printf("Platforms:\n");
    for(int i = 0; i < (int)ret_num_platforms; i++) {
        char platformName[128];
        ret = clGetPlatformInfo(platform_ids[0], CL_PLATFORM_NAME, 128, platformName, NULL);
        printf("%s\n", platformName);
    }
    printf("\n");
    return 0;
}