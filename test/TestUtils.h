#ifndef CML_TEST_UTILS_H
#define CML_TEST_UTILS_H

#include <cml/Logger.h>

#include <stdbool.h>

#define ANNOUNCE_TEST(testName) cml_log("[TEST] Beginning test: %s\n", testName);

bool cml_withinMarginOfError(const float actual, const float expected, const float acceptableDeviation);

#endif // CML_TEST_UTILS_H