#include <cml/matrix/Matrix.h>

#include <stdlib.h>
#include <assert.h>

// Should come last to override malloc family in debug mode
#include <cdh/Memory.h>

cml_Matrix cml_createMatrix(const size_t rows, const size_t cols) {
    cml_Matrix matrix;
    matrix.data = (float*)malloc(sizeof(float) * rows * cols);
    matrix.rows = rows;
    matrix.cols = cols;
    return matrix;
}

void cml_deleteMatrix(cml_Matrix matrix) {
    assert(matrix.data != NULL);
    free(matrix.data);
    matrix.data = NULL;
    matrix.rows = 0;
    matrix.cols = 0;
}