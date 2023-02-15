#include <cml/matrix/Matrix.h>

#include <stdlib.h>
#include <assert.h>

Matrix cml_createMatrix(const size_t rows, const size_t cols) {
    Matrix matrix;
    matrix.data = (double*)malloc(sizeof(double) * rows * cols);
    matrix.rows = rows;
    matrix.cols = cols;
    return matrix;
}

void cml_deleteMatrix(Matrix matrix) {
    assert(matrix.data != NULL);
    free(matrix.data);
    matrix.data = NULL;
    matrix.rows = 0;
    matrix.cols = 0;
}