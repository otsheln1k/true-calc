#include <string.h>

#include <array.h>
#include <test_ut.h>

#define GET_INT(arr,idx) *(int *)getArrayItemValue(arr, idx)

void arrayAppendInt(array arr, int i) {
    arrayAppend(arr, &i);
}

void arrayInsertInt(array arr, unsigned int idx, int i) {
    arrayInsert(arr, idx, &i);
}

void arraySetInt(array arr, unsigned int idx, int i) {
    setArrayItemValue(arr, idx, &i);
}

bool test_array_AppendGet() {
    array arr = makeArray(sizeof(int));
    arrayAppendInt(arr, 42);
    arrayAppendInt(arr, 100);
    arrayAppendInt(arr, 12);
    bool res = \
           ASSERT(GET_INT(arr, 1) == 100)
        && ASSERT(GET_INT(arr, 0) == 42)
        && ASSERT(GET_INT(arr, 2) == 12);
    destroyArray(arr);
    return res;
}

bool test_array_SetInsert() {
    array arr = makeArray(sizeof(int));
    arrayAppendInt(arr, 42);
    arrayAppendInt(arr, 0);
    arrayInsertInt(arr, 1, 100500);
    if (!ASSERT(GET_INT(arr, 1) == 100500)) {
        bool res = false;
        goto t_a_SI_ret;
    }
    arraySetInt(arr, 2, 15);
    bool res = \
           ASSERT(GET_INT(arr, 2) == 15)
        && ASSERT(GET_INT(arr, 1) == 100500)
        && ASSERT(GET_INT(arr, 0) == 42);
t_a_SI_ret:
    destroyArray(arr);
    return res;
}

bool test_array_ConvSlice() {
    array arr = convertedArray((int[6]){15, 32, 2, 28, 90, 42}, 6, sizeof(int));
    if (!ASSERT(getArrayLength(arr) == 6)
     || !ASSERT(GET_INT(arr, 3) == 28)) {
        bool res = false;
        goto t_a_CS_ret;
    }
    array slc = arraySlice(arr, 1, 4);
    bool res = \
           ASSERT(getArrayLength(slc) == 3)
        && ASSERT(GET_INT(slc, 0) == 32)
        && ASSERT(GET_INT(slc, 1) == 2)
        && ASSERT(GET_INT(slc, 2) == 28);
    destroyArrayHeader(slc);
t_a_CS_ret:
    destroyArray(arr);
    return res;
}

typedef struct {
    int i;
    double f;
} SType;

bool test_array_ConvEx() {
    array stringarr = convertedArray((char*[3]){ "abc", "def", "ghi" }, 3, sizeof(char *));
    if (!ASSERT(strcmp(*(char **)getArrayItemValue(stringarr, 1), "def") == 0)) {
        printf("%s\n", *(char **)getArrayItemValue(stringarr, 1));
        bool res = false;
        goto t_a_CE_ret_1;
    }
    array structarr = convertedArray((SType[4]){ { 1, 1.2 }, { 2, 2.2 }, { 3, 3.2 }, { 4, 4.2 } }, 4, sizeof(SType));
    if (!(ASSERT(((SType *)getArrayItemValue(structarr, 2))->f == 3.2)
        &&ASSERT((*(SType *)getArrayItemValue(structarr, 1)).i == 2))) {
        bool res = false;
        goto t_a_CE_ret_2;
    }
    bool res = \
           ASSERT(getArrayLength(stringarr) == 3)
        && ASSERT(getArrayLength(structarr) == 4);
t_a_CE_ret_2:
    destroyArray(structarr);
t_a_CE_ret_1:
    destroyArray(stringarr);
    return res;
}

#define GET_ARRAY(arr,idx) *(array *)getArrayItemValue(arr, idx)

bool test_array_2dim() {
    array arr = makeArray(sizeof(array));
    array a01 = convertedArray((int[6]){0, 1, 2, 3, 4, 5}, 6, sizeof(int));
    arrayAppend(arr, &a01);
    array a02 = convertedArray((int[6]){7, 8, 9, 10, 11, 12}, 6, sizeof(int));
    arrayAppend(arr, &a02);
    bool res = \
           ASSERT(GET_INT(GET_ARRAY(arr, 0), 2) == 2)
        && ASSERT(GET_INT(GET_ARRAY(arr, 1), 2) == 9)
        && ASSERT(GET_INT(GET_ARRAY(arr, 0), 0) == 0);
    destroyArray(a01);
    destroyArray(a02);
    destroyArray(arr);
    return res;
}

bool test_array_NextItem() {
    array arr = convertedArray((int[6]){0, 1, 2, 3, 4, 5}, 6, sizeof(int));
    int *item2 = getArrayItemValue(arr, 2);
    int *item3 = getArrayNextItem(item2);
    bool res = \
           ASSERT(*item2 == 2)
        && ASSERT(*item3 == 3);
    destroyArray(arr);
    return res;
}

bool test_array_For() {
    array arr = convertedArray((int[6]){ 15, -1, 42, 146, 2, 0 }, 6, sizeof(int));
    unsigned int i = 0;
    bool res = false;
    FOR_ARRAY_COUNTER(arr, idx, int, itm) {
        if (!(ASSERT(idx == i)
           && ASSERT(*itm == ((idx == 0) ? 15 : (idx == 1)
                   ? -1 : (idx == 2) ? 42 : (idx == 3) ? 146 :
                   (idx == 4) ? 2 : 0)))) {
            goto t_a_F_ret;
        }
        i += 1;
    }
    res = ASSERT(i == 6);
t_a_F_ret:
    destroyArray(arr);
    return res;
}

bool test_array_Copy() {
    array arr0 = convertedArray((int [6]){ 5, 2, 41, -1, 0, 66 }, 6, sizeof(int));
    array arr1 = arrayCopy(ARRAY_TAIL(arr0, 2));
    int i = 42;
    setArrayItemValue(arr0, 2, &i);
    bool res =
           ASSERT(GET_INT(arr0, 2) == 42)
        && ASSERT(GET_INT(arr1, 0) == 41)
        && ASSERT(GET_INT(arr0, 3) == GET_INT(arr1, 1))
        && ASSERT(GET_INT(arr0, 4) == GET_INT(arr1, 2))
        && ASSERT(GET_INT(arr0, 5) == GET_INT(arr1, 3));
    destroyArray(arr0);
    destroyArray(arr1);
    return res;
}

bool test_array_Detach() {
    array arr = convertedArray((int[6]){ 1, 2, 3, 0, -1, -2 }, 6, sizeof(int));
    array slc = ARRAY_TAIL(arr, 3);
    int i = -3;
    setArrayItemValue(arr, 3, &i);
    if (!(ASSERT(GET_INT(arr, 3) == GET_INT(slc, 0))
       && ASSERT(GET_INT(arr, 4) == GET_INT(slc, 1))
       && ASSERT(GET_INT(arr, 5) == GET_INT(slc, 2)))) {
        bool res = false;
        destroyArrayHeader(slc);
        goto t_a_D_ret;
    }
    arrayDetached(slc);
    i = -4;
    setArrayItemValue(arr, 5, &i);
    bool res = \
            ASSERT(GET_INT(arr, 3)==GET_INT(slc, 0))
         && ASSERT(GET_INT(arr, 4)==GET_INT(slc, 1))
         && ASSERT(GET_INT(arr, 5) + 2 == GET_INT(slc, 2));
    destroyArray(slc);
t_a_D_ret:
    destroyArray(arr);
    return res;
}

bool test_array_Search() {
    array arr = ARRAY_CONV(int, 6, ARG({ 1, 1, 2, 3, 5, 8 }));
    int a = 5;
    int b = 6;
    bool res = ASSERT(getArrayItemIndex(arr, &a) == 4)
            && ASSERT(getArrayItemIndex(arr, &b) == 6)
            && ASSERT(getArrayItemByVal(arr, &a) == getArrayItemValue(arr, 4))
            && ASSERT(getArrayItemByVal(arr, &b) == NULL);
    destroyArray(arr);
    return res;
}

bool test_array_Reverse() {
    array arr = ARRAY_CONV(int, 8, ARG({ 0, 1, 2, 3, 4, 5, 6, 7 }));
    array slc = arraySlice(arr, 2, 6);
    arrayReversed(slc);
    bool res = \
            ASSERT(GET_INT(arr, 1) == 1)
         && ASSERT(GET_INT(arr, 2) == 5)
         && ASSERT(GET_INT(arr, 3) == 4)
         && ASSERT(GET_INT(arr, 4) == 3)
         && ASSERT(GET_INT(arr, 5) == 2)
         && ASSERT(GET_INT(arr, 6) == 6)
         && ASSERT(GET_INT(arr, 7) == 7);
    destroyArrayHeader(slc);
    destroyArray(arr);
    return res;
}

bool test_array_DoubleReverse() {
    array arr = ARRAY_CONV(int, 6, ARG({ 1, 2, 3, 4, 5, 6 }));
    array slc = arrayReversed(arrayReversed(arraySlice(arr, 1, 5)));
    bool res = \
            ASSERT(GET_INT(arr, 0) == 1)
         && ASSERT(GET_INT(arr, 1) == 2)
         && ASSERT(GET_INT(arr, 2) == 3)
         && ASSERT(GET_INT(arr, 3) == 4)
         && ASSERT(GET_INT(arr, 4) == 5)
         && ASSERT(GET_INT(arr, 5) == 6);
    destroyArrayHeader(slc);
    destroyArray(arr);
    return res;
}

bool test_array_Part() {
    array arr = ARRAY_CONV(int, 8, ARG({ 1, 2, 4, 8, 16, 32, 64, 128 }));
    array slc = arraySlice(arr, 0, 7);
    destroyArrayHeader(arr);
    struct array_item *after = arrayPart(slc, 1, 6);
    bool res = \
        ASSERT(GET_INT(slc, 0) == 2)
        && ASSERT(GET_INT(slc, 1) == 4)
        && ASSERT(GET_INT(slc, 2) == 8)
        && ASSERT(GET_INT(slc, 3) == 16)
        && ASSERT(GET_INT(slc, 4) == 32)
        && ASSERT(getArrayLength(slc) == 5)
        && ASSERT(*(int *)after->data == 128);
    destroyArray(slc);
    return res;
}

int main() {
    if (test_array_AppendGet()
     && test_array_SetInsert()
     && test_array_ConvSlice()
     && test_array_ConvEx()
     && test_array_2dim()
     && test_array_NextItem()
     && test_array_For()
     && test_array_Copy()
     && test_array_Detach()
     && test_array_Search()
     && test_array_Reverse()
     && test_array_DoubleReverse()
     && test_array_Part())
        fprintf(stderr, "All ok\n");
    else
        return 1;
}

