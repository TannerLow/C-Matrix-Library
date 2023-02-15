#include <cml/Logger.h>
#include "Tests.h"


int main() {
    cml_logStream = stdout;
    cml_test_runAllTests();
    return 0;
}