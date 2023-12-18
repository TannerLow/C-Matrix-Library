#include <cml/matrix/MatrixArray.h>
#include <cml/matrix/Matrix.h>

#include <stdlib.h>

// Should come last to override malloc family in debug mode
#include <cdh/Memory.h>

cml_MatrixArray cml_createMatrixArray(const size_t count) {
    cml_Matrix** matrices = (cml_Matrix**)malloc(sizeof(cml_Matrix*) * count);
    
    cml_MatrixArray array;
    array.matrices = matrices;
    array.count = count;

    return array;
}