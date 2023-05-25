#include <cml/Logger.h>
#include "Tests.h"
#include "Testing.h"
#include "CPUMatrixMathTests.h"


int main() {
    cml_logStream = stdout;
    //cml_test_runAllTests();
    driver();
    cml_log("cml_test_cpuMatrixAddition: %s\n", cml_test_cpuMatrixAddition() ? "SUCCESS" : "FAILURE");
    cml_log("cml_test_cpuMatrixMultiply: %s\n", cml_test_cpuMatrixMultiply() ? "SUCCESS" : "FAILURE");
    cml_log("cml_test_cpuMatrixLinear: %s\n", cml_test_cpuMatrixLinear() ? "SUCCESS" : "FAILURE");
    cml_log("cml_test_cpuMatrixRelu: %s\n", cml_test_cpuMatrixRelu() ? "SUCCESS" : "FAILURE");
    return 0;
}