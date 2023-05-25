#include "CPUMatrixMathTests.h"
#include "TestUtils.h"

#include <cml/matrix/Matrix.h>
#include <cml/matrix/MatrixMath.h>

#include <string.h>

bool cml_test_cpuMatrixAddition() {
    ANNOUNCE_TEST("cml_test_cpuMatrixAddition");
    size_t dimA = 3;
    size_t dimB = 2;
    cml_Matrix a = cml_createMatrix(dimA, dimB);
    cml_Matrix b = cml_createMatrix(dimA, dimB);

    for(size_t i = 0; i < dimA*dimB; i++) {
        a.data[i] = i;
        b.data[i] = i;
    }

    cml_Matrix c = cml_createMatrix(dimA, dimB);
    cml_matrixAdd(&a, &b, &c);

    bool matchesExpected = true;
    for(size_t i = 0; i < dimA*dimB; i++) {
        if(!cml_withinMarginOfError(c.data[i], i*2, 0.05f)) {
            matchesExpected = false;
            break;
        }
    }

    cml_deleteMatrix(a);
    cml_deleteMatrix(b);
    cml_deleteMatrix(c);

    return matchesExpected;
}

bool cml_test_cpuMatrixMultiply() {
    ANNOUNCE_TEST("cml_test_cpuMatrixMultiply");
    size_t dimA = 3;
    size_t dimB = 2;
    cml_Matrix a = cml_createMatrix(dimA, dimB);
    cml_Matrix b = cml_createMatrix(dimB, dimA);

    for(size_t i = 0; i < dimA*dimB; i++) {
        a.data[i] = i;
        b.data[i] = i;
    }

    cml_Matrix c = cml_createMatrix(dimA, dimA);
    float expectedValues[] = {3,4,5,9,14,19,15,24,33};

    cml_matrixMultiply(a, b, &c);

    bool matchesExpected = true;
    for(size_t i = 0; i < dimA*dimA; i++) {
        if(c.data[i] != expectedValues[i]) {
            matchesExpected = false;
            break;
        }
    }

    return matchesExpected;
}

bool cml_test_cpuMatrixLinear() {
    ANNOUNCE_TEST("cml_test_cpuMatrixLinear");
    size_t dimA = 3; // row
    size_t dimB = 1; // col
    cml_Matrix x = cml_createMatrix(dimA, dimB);

    float input[] = {-1, 0.1f, 5};
    memcpy(x.data, input, dimA * sizeof(float));

    cml_Matrix y = cml_createMatrix(dimA, dimB);
    cml_matrixLinear(&x, &y);

    bool matchesExpected = true;
    for(size_t i = 0; i < dimA*dimB; i++) {
        if(x.data[i] != y.data[i]) {
            matchesExpected = false;
            break;
        }
    }

    cml_deleteMatrix(x);
    cml_deleteMatrix(y);

    return matchesExpected;
}
bool cml_test_cpuMatrixRelu() {
    ANNOUNCE_TEST("cml_test_cpuMatrixRelu");
    size_t dimA = 3; // row
    size_t dimB = 1; // col
    cml_Matrix x = cml_createMatrix(dimA, dimB);

    float input[] = {-1, 0, 0.1f};
    memcpy(x.data, input, dimA * sizeof(float));

    cml_Matrix y = cml_createMatrix(dimA, dimB);
    cml_matrixRelu(&x, &y);

    bool matchesExpected = true;
    for(size_t i = 0; i < dimA*dimB; i++) {
        float expectedValue = (x.data[i] > 0 ? x.data[i] : 0);
        if(y.data[i] != expectedValue) {
            matchesExpected = false;
            break;
        }
    }

    cml_deleteMatrix(x);
    cml_deleteMatrix(y);

    return matchesExpected;
}