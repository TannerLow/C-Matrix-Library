#include <cml/Logger.h>
#include "Tests.h"
#include "Testing.h"


int main() {
    cml_logStream = stdout;
    //cml_test_runAllTests();
    driver();
    return 0;
}