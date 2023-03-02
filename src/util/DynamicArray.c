#include <cml/util/DynamicArray.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

size_t cml_dynamicArrayGrowAmount = 10;

cml_DynamicArray cml_createDynamicArray(size_t initialCapacity, size_t itemSize) {
    cml_DynamicArray array;

    array.data = calloc(initialCapacity, itemSize);
    assert(array.data != NULL);

    array.capacity = initialCapacity;
    array.size = 0;
    array.itemSize = itemSize;

    return array;
}

void cml_deleteDynamicArray(cml_DynamicArray* array) {
    assert(array != NULL);

    free(array->data);
    array->capacity = 0;
    array->size = 0;
    array->itemSize = 0;
}


static void cml_growDynamicArray(cml_DynamicArray* array) {
    assert(array != NULL);

    array->data = realloc(array->data, array->capacity + cml_dynamicArrayGrowAmount);
}

size_t cml_dynamicArrayPush(cml_DynamicArray* array, void* element) {
    assert(array != NULL);
    assert(element != NULL);

    if(array->size >= array->capacity) {
        cml_growDynamicArray(array);
    }

    char* memLocation = (char*)array->data + array->size * array->itemSize;
    memcpy((void*)memLocation, element, array->itemSize);

    return array->size++;
}

void* cml_dynamicArrayGet(cml_DynamicArray* array, size_t index) {
    assert(array != NULL);

    return (void*)((char*)array->data + index * array->itemSize);
}