#include <cml/matrix/MatrixMath.h>

#include <string.h>
#include <assert.h>

void cml_matrixAdd(const cml_Matrix* a, const cml_Matrix* b, cml_Matrix* c) {
    assert(a != NULL);
    assert(b != NULL);
    assert(c != NULL);
    assert(a->rows == b->rows && b->rows == c->rows);
    assert(a->cols == b->cols && b->cols == c->cols);

    for(size_t i = 0; i < (a->rows * a->cols); i++) {
        c->data[i] = a->data[i] + b->data[i];
    }
}

void cml_matrixMultiply(const cml_Matrix a, const cml_Matrix b, cml_Matrix* c) {
    assert(a.data != NULL);
    assert(b.data != NULL);
    assert(a.cols == b.rows);
    assert(c != NULL);

    for(size_t row = 0; row < a.rows; row++) {
        for(size_t col = 0; col < b.cols; col++) {
            float sum = 0;
            for(size_t i = 0; i < a.cols; i++) {
                sum += a.data[row * a.cols + i] * b.data[i * b.cols + col]; 
            }
            c->data[row * c->cols + col] = sum;
        }
    }
}

void cml_matrixAddRow(const cml_Matrix a, const cml_Matrix row, cml_Matrix* out) {
    assert(a.cols == row.cols);
    assert(row.rows == 1);
    assert(a.data != NULL);
    assert(row.data != NULL);
    assert(out->data != NULL);
    assert(out->cols == a.cols);
    assert(out->rows == a.rows);

    for(size_t r = 0; r < a.rows; r++) {
        for(size_t c = 0; c < a.cols; c++) {
            out->data[r*a.cols + c] = a.data[r*a.cols + c] + row.data[c];
        }
    }
}

void cml_matrixLinear(const cml_Matrix* x, cml_Matrix* y) {
    assert(x != NULL);
    assert(y != NULL);
    assert(x->rows*x->cols == y->rows*y->cols);

    size_t elementCount = x->rows*x->cols;
    memcpy(y->data, x->data, elementCount * sizeof(float));
}

void cml_matrixLinearDerivative(const cml_Matrix* x, cml_Matrix* y) {
    assert(x != NULL);
    assert(y != NULL);
    assert(x->rows*x->cols == y->rows*y->cols);

    size_t elementCount = x->rows*x->cols;
    memset(y->data, 1, elementCount * sizeof(float));
}

void cml_matrixRelu(const cml_Matrix* x, cml_Matrix* y) {
    assert(x != NULL);
    assert(y != NULL);
    assert(x->rows*x->cols == y->rows*y->cols);

    size_t count = x->rows * x->cols;
    for (size_t i = 0; i < count; i++) {
        y->data[i] = x->data[i] > 0 ? x->data[i] : 0;
    }
}

void cml_matrixReluDerivative(const cml_Matrix* x, cml_Matrix* y) {
    assert(x != NULL);
    assert(y != NULL);
    assert(x->rows*x->cols == y->rows*y->cols);

    size_t count = x->rows * x->cols;
    for (size_t i = 0; i < count; i++) {
        y->data[i] = x->data[i] > 0 ? 1 : 0;
    }
}