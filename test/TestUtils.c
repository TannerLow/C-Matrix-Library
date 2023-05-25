#include "TestUtils.h"

#include <math.h>

bool cml_withinMarginOfError(const float actual, const float expected, const float acceptableDeviation) {
    return fabs(actual - expected) < acceptableDeviation;
}