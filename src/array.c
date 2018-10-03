#include "array.h"

#define SIZE_SIZE sizeof(size_t)
#define SIZE_UINT sizeof(unsigned int)
#define SIZE_ADDR sizeof(void *)

/* Array head: <size_t size of element><unsigned int length><void * next address> */
/* Array item: <void * next address><<size of elt>B elt> */


static void setArrayLength(array arr, unsigned int newLength);
static void *getArrayItem(array arr, unsigned int index);
static void _setArrayItemValue(void *item, void *elt, size_t elt_size);
static void *makeNewArrayItem(size_t elt_size);


array makeArray(size_t elt_size) {
    array arr = (array) malloc(SIZE_SIZE + SIZE_UINT + SIZE_ADDR);
    if (arr == NULL) {
        return NULL;
    }
    memcpy(arr, &elt_size, SIZE_SIZE);
    memset(arr + SIZE_SIZE, 0, SIZE_UINT);
    memset(arr + SIZE_SIZE + SIZE_UINT, 0, SIZE_ADDR);
    return arr;
}

array convertedArray(void *carray, unsigned int len, size_t elt_size) {
    array arr = makeArray(elt_size);
    unsigned int i;
    for (i = 0; i < len; i++) {
        arrayAppend(arr, carray + (elt_size * i));
    }
    return arr;
}

array arraySlice(array arr, unsigned int stt, unsigned int end) {
    array res = makeArray(getArrayElementSize(arr));
    setArrayLength(res, end - stt);
    void *item = getArrayItem(arr, stt);
    memcpy(res + SIZE_SIZE + SIZE_UINT, &item, SIZE_ADDR);
    return res;
}

array arrayDetached(array arr) {
    unsigned int idx = 0;
    unsigned int len = getArrayLength(arr);
    size_t esz = getArrayElementSize(arr);
    void *previtm;
    for (void *itm = getArrayItemValue(arr, 0);
            idx < len;
            itm = getArrayNextItem(itm)) {
        void *newitm = makeNewArrayItem(esz);
        _setArrayItemValue(newitm, itm, esz);
        if (idx) {
            memcpy(previtm, &newitm, SIZE_ADDR);
        } else {  // addr of this elt should be in the header
            memcpy(arr + SIZE_SIZE + SIZE_UINT, &newitm, SIZE_ADDR);
        }
        previtm = newitm;
        idx += 1;
    }
    return arr;
}

array arrayCopy(array original) {
    array res = makeArray(getArrayElementSize(original));
    unsigned int idx = 0;
    unsigned int len = getArrayLength(original);
    for (void *item = getArrayItemValue(original, 0);
            idx++ < len;
            item = getArrayNextItem(item))
        arrayAppend(res, item);
    return res;
}

array arrayReversed(array arr) {
    if (getArrayLength(arr) < 2)
        return arr;
    unsigned int len = getArrayLength(arr);
    void *item0;
    void *this_item = getArrayItemValue(arr, 0);        // item in idx=0
    void *next_item = getArrayNextItem(this_item);      // item in idx=1
    for (unsigned int idx = 0; idx < len - 2; idx++) {  // idx is item0 index
        item0 = this_item;                              // idx + 0
        this_item = next_item;                          // idx + 1
        next_item = getArrayNextItem(this_item);        // idx + 2
        if (idx >= 1)
            *(void **)(this_item - SIZE_ADDR) = item0 - SIZE_ADDR;
    }  // next_item should point to the last elt
    item0 = getArrayItemValue(arr, 0);
    *(void **)(getArrayNextItem(item0) - SIZE_ADDR) = next_item - SIZE_ADDR;
    *(void **)(item0 - SIZE_ADDR) = this_item - SIZE_ADDR;  // sets the next item for item0
    size_t elt_size = getArrayElementSize(arr);
    void *mem = malloc(elt_size);
    memcpy(mem, item0, elt_size);
    _setArrayItemValue(item0 - SIZE_ADDR, next_item, elt_size);
    _setArrayItemValue(next_item - SIZE_ADDR, mem, elt_size);
    free(mem);
    return arr;
}

array arrayPart(array arr, unsigned int from, unsigned int to) {
    if (from >= to)
        return arrayClear(arr);
    for (unsigned int idx = 0; idx < from; idx++)
        arrayRemove(arr, 0);
    void *last_item_next = *(void **)getArrayItem(arr, getArrayLength(arr) - 1);
    destroyArray(arraySlice(arr, to, getArrayLength(arr)));
    unsigned int len = to - from;
    setArrayLength(arr, len);
    *(void **)getArrayItem(arr, len - 1) = last_item_next;
    return arr;
}

void destroyArray(array arr) {
    arrayClear(arr);
    free(arr);
}

void destroyArrayHeader(array slc) {
    free(slc);
}

size_t getArrayElementSize(array arr) {
    return *(size_t *)arr;
}

unsigned int getArrayLength(array arr) {
    void *plc = arr + SIZE_SIZE;
    return *((unsigned int *)plc);
}

static void setArrayLength(array arr, unsigned int newLength) {
    memcpy(arr + SIZE_SIZE, &newLength, sizeof(unsigned int));
}

static void *getArrayItem(array arr, unsigned int index) {
    if (index > getArrayLength(arr)) {
        return NULL;
    }
    unsigned int i;
    for (i = 0; i < index + 1; i++) {
        if (i == 0) {
            arr = *(void **)(arr + SIZE_SIZE + SIZE_UINT);
        } else {
            arr = *(void **)arr;
        }
    }
    return arr;
}

void *getArrayItemValue(array arr, unsigned int index) {
    return getArrayItem(arr, index) + SIZE_ADDR;
}

void *getArrayNextItem(void *arritem) {
    return *(void **)(arritem - SIZE_ADDR) + SIZE_ADDR;
}

static void _setArrayItemValue(void *item, void *elt, size_t elt_size) {
    memcpy(item + SIZE_ADDR, elt, elt_size);
}

void setArrayItemValue(array arr, unsigned int index, void *elt) {
    size_t elt_size = getArrayElementSize(arr);
    void *item = getArrayItem(arr, index);
    _setArrayItemValue(item, elt, elt_size);
}

static void *makeNewArrayItem(size_t elt_size) {
    void *item = malloc(SIZE_ADDR + elt_size);
    void *addr = NULL;
    memcpy(item, &addr, SIZE_ADDR);
    return item;
}

void arrayAppend(array arr, void *elt) {
    unsigned int len = getArrayLength(arr);
    size_t elt_size = getArrayElementSize(arr);
    if (~len == 0) {
        return;
    }
    void *item = makeNewArrayItem(elt_size);
    if (item == NULL) {
        return;
    }
    setArrayLength(arr, len + 1);
    void *prev_item;
    if (len > 0) {
        prev_item = getArrayItem(arr, len - 1);
    } else {
        prev_item = arr + SIZE_SIZE + SIZE_UINT;
    }

    memset(item, 0, SIZE_ADDR);
    memcpy(prev_item, &item, SIZE_ADDR);
    _setArrayItemValue(item, elt, elt_size);
}

void arrayInsert(array arr, unsigned int index, void *elt) {
    unsigned int len = getArrayLength(arr);
    size_t elt_size = getArrayElementSize(arr);
    if (~len == 0 || index > len) {
        return;
    }
    void *new_item = makeNewArrayItem(elt_size);
    if (new_item == NULL) {
        return;
    }
    if (index < len) {  // has next elt
        void *next_item = getArrayItem(arr, index);
        memcpy(new_item, &next_item, SIZE_ADDR);
    }
    if (index > 0) {  // prev is elt
        void *prev_item = getArrayItem(arr, index - 1);
        memcpy(prev_item, &new_item, SIZE_ADDR);
    } else {  // prev is head
        memcpy(arr + SIZE_SIZE + SIZE_UINT, &new_item, SIZE_ADDR);
    }
    _setArrayItemValue(new_item, elt, elt_size);
    setArrayLength(arr, len + 1);
}

void arrayPush(array arr, void *elt) {
    arrayInsert(arr, 0, elt);
}

void arrayRemove(array arr, unsigned int index) {
    unsigned int len = getArrayLength(arr);
    if (index >= len || !len) { return; }
    void *item = getArrayItem(arr, index);
    void *next_item = index < len - 1 ? getArrayItem(arr, index + 1) : NULL;
    if (index > 0) {  // prev is elt
        void *prev_item = getArrayItem(arr, index - 1);
        memcpy(prev_item, &next_item, SIZE_ADDR);
    } else {
        memcpy(arr + SIZE_SIZE + SIZE_UINT, &next_item, SIZE_ADDR);
    }
    free(item);
    setArrayLength(arr, len - 1);
}

array arrayClear(array arr) {
    unsigned int len = getArrayLength(arr);
    if (!len) { return arr; }
    unsigned int i;
    for (i = len - 1;; i--) {
        free(getArrayItem(arr, i));
        if (!i) { break; }  // i is an unsigned int, so it can't be < 0.
    }
    setArrayLength(arr, 0);
    memset(arr + SIZE_SIZE + SIZE_UINT, 0, SIZE_ADDR);
    return arr;
}

void *arrayJoint(array arr) {
    // optimized
    unsigned int len = getArrayLength(arr);
    size_t elt_size = getArrayElementSize(arr);
    void *a = calloc(len, elt_size);
    unsigned int i;
    void *item = getArrayItem(arr, 0);
    for (i = 0; i < len; i++) {
        memcpy(a + (i * elt_size), item + SIZE_ADDR, elt_size);
        item = *(void **)item;
    }
    return a;
}

void arrayIter(array arr, arrayIterFunc f) {
    // optimized; can be used as reduce and filter
    unsigned int len = getArrayLength(arr);
    if (!len) {
        return;
    }
    unsigned int i;
    void *item = getArrayItem(arr, 0);
    void *nitem;
    for (i = 0; i < len; i++) {
        nitem = *(void **)item;
        if (f(arr, i, item + SIZE_ADDR)) {
            break;
        }
        item = nitem;
    }
}

unsigned int getArrayItemIndex(array arr, void *elt) {
    size_t elt_size = getArrayElementSize(arr);
    FOR_ARRAY_COUNTER(arr, idx, void, itm)
        if (itm == elt || !memcmp(itm, elt, elt_size))
            return idx;
    return getArrayLength(arr);
}

void *getArrayItemByVal(array arr, void *elt) {
    size_t elt_size = getArrayElementSize(arr);
    FOR_ARRAY_COUNTER(arr, idx, void, itm)
        if (itm == elt || !memcmp(itm, elt, elt_size))
            return itm;
    return NULL;
}

