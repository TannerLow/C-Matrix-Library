#ifndef CML_MATRIX_MATH_H
#define CML_MATRIX_MATH_H

#include "Matrix.h"

// By default operations are done on CPU, use GPU alternatives for hardware acceleration
void cml_matrixMultiply(const cml_Matrix* a, const cml_Matrix* b, cml_Matrix* c);

#endif // CML_MATRIX_MATH_H