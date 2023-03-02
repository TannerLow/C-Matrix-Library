#include <cml/matrix/MatrixMath.h>

#include <assert.h>

void cml_matrixMultiply(const cml_Matrix* a, const cml_Matrix* b, cml_Matrix* c) {
    assert(a != NULL);
    assert(b != NULL);
    assert(c != NULL);

    for(size_t row = 0; row < a->rows; row++) {
        for(size_t col = 0; col < b->cols; col++) {
            float sum = 0;
            for(size_t i = 0; i < a->cols; i++) {
                sum += a->data[row * a->cols + i] * b->data[i * b->cols + col]; 
            }
            c->data[row * c->cols + col] = sum;
        }
    }
}