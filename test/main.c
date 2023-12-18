#include <cdh/Logger.h>
#include "Tests.h"
#include "Testing.h"
#include "CPUMatrixMathTests.h"
#include <stdio.h>

int main() {
    FILE* logFile = fopen("log.txt", "w");
    cdh_logStream = logFile;
    //cml_test_runAllTests();
    driver();
    cdh_log("cml_test_cpuMatrixAddition: %s\n", cml_test_cpuMatrixAddition() ? "SUCCESS" : "FAILURE");
    cdh_log("cml_test_cpuMatrixMultiply: %s\n", cml_test_cpuMatrixMultiply() ? "SUCCESS" : "FAILURE");
    cdh_log("cml_test_cpuMatrixLinear: %s\n", cml_test_cpuMatrixLinear() ? "SUCCESS" : "FAILURE");
    cdh_log("cml_test_cpuMatrixRelu: %s\n", cml_test_cpuMatrixRelu() ? "SUCCESS" : "FAILURE");
    
    fclose(logFile);
    return 0;
}