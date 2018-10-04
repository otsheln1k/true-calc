#include <stdlib.h>
#include <string.h>

#define NEW_FOR_ARRAY
#include "array.h"


// CONSTRUCTORS

struct array_head *makeArray(size_t elt_size)
{
    struct array_head *arr = malloc(sizeof(*arr));
    if (arr == NULL)
        return NULL;
    *arr = (struct array_head){
        .elt_size = elt_size,
        .length = 0,
        .first = NULL,
    };

    return arr;
}

static struct array_item *makeNewArrayItem(size_t elt_size) {
    struct array_item *item = malloc(sizeof(*item) + elt_size);
    item->next = NULL;
    return item;
}


// CONVERTING TO/FROM C ARRAYS

struct array_head *convertedArray(void *carray,
                                  size_t len,
                                  size_t elt_size)
{
    struct array_head *arr = makeArray(elt_size);

    for (size_t i = 0; i < len; i++)
        arrayAppend(arr, carray + i * elt_size);

    return arr;
}

void *arrayJoint(struct array_head * arr)
{
    size_t len = arr->length;
    void *a = calloc(len, arr->elt_size);
    size_t elt_size = arr->elt_size;
    struct array_item *item = arr->first;

    for (size_t i = 0; i < len; i++) {
        memcpy(a + i * elt_size, item->data, elt_size);
        item = item->next;
    }

    return a;
}


// DESTRUCTORS

struct array_item *destroyArray(struct array_head *arr)
{
    struct array_item *item = arrayClear(arr);
    free(arr);
    return item;
}

void destroyArrayHeader(struct array_head *slc)
{
    free(slc);
}


// GLOBAL ACTIONS

struct array_head *arraySlice(struct array_head *arr,
                               size_t stt,
                               size_t end)
{
    struct array_head *res = makeArray(arr->elt_size);

    res->length = end - stt;
    res->first = getArrayItem(arr, stt);

    return res;
}

struct array_item *arrayDetach(struct array_head *arr)
{
    size_t idx = 0;
    size_t len = arr->length;
    struct array_item *previtm, *newitm, *itm;

    for (itm = arr->first;
         idx < len;
         previtm = newitm, idx++, itm = itm->next) {
        newitm = makeNewArrayItem(arr->elt_size);
        memcpy(newitm->data, itm->data, arr->elt_size);
        *(idx ? &previtm->next : &arr->first) = newitm;
    }

    return itm;
}

struct array_head *arrayCopy(struct array_head *original)
{
    struct array_head *res = makeArray(original->elt_size);

    res->first = original->first;
    res->length = original->length;
    arrayDetach(res);

    return res;
}

struct array_item *arrayPart(struct array_head *arr,
                             size_t from,
                             size_t to)
{
    if (from >= to)
        return arrayClear(arr);

    size_t len = to - from;
    struct array_head *slice;
    struct array_item *itm, *after;

    // remove tail
    slice = arraySlice(arr, to, arr->length);
    after = destroyArray(slice);
    // remote head
    slice = arraySlice(arr, 0, from);
    itm = destroyArray(slice);

    arr->first = itm;
    arr->length = len;

    return after;
}

struct array_item *arrayReverse(struct array_head *arr)
{
    size_t len = arr->length;
    if (len < 2)
        return len ? arr->first->next : NULL;

    struct array_item *this = arr->first->next;
    struct array_item *next;

    for (size_t idx = 1; idx < len; idx++) {
        next = this->next;
        arr->first->next = this->next;
        this->next = arr->first;
        arr->first = this;
        this = next;
    }

    return this;
}

struct array_item *arrayReverseSaveLink(struct array_head *arr)
{
    struct array_item *item;
    struct array_head *slice;

    switch (arr->length) {
    case 0:
        return NULL;
    case 1:
        return arr->first->next;
    case 2:
        item = arr->first->next;
        break;
    default:
        slice = arraySlice(arr, 1, arr->length - 1);
        item = arrayReverse(slice);
        arr->first->next = slice->first;
        destroyArrayHeader(slice);
        break;
    }

    unsigned char tmp[arr->elt_size];

    memcpy(tmp, item->data, arr->elt_size);
    memcpy(item->data, arr->first->data, arr->elt_size);
    memcpy(arr->first->data, tmp, arr->elt_size);

    return item->next;
}

struct array_item *arrayClear(struct array_head *arr)
{
    size_t i = arr->length;
    struct array_item *item = arr->first, *next;

    while (i--) {
        next = item->next;
        free(item);
        item = next;
    }

    arr->length = 0;
    arr->first = NULL;

    return item;
}


// ITEMS

struct array_item *getArrayItem(struct array_head *arr, size_t index)
{
    if (index >= arr->length)
        return NULL;

    struct array_item *item = arr->first;

    for (size_t i = 0; i < index; i++, item = item->next);

    return item;
}

struct array_item *arrayAppend(struct array_head * arr, void *elt)
{
    size_t len = arr->length;
    struct array_item *item = makeNewArrayItem(arr->elt_size);

    if (item == NULL) return NULL;

    arr->length += 1;

    *(len ? &getArrayItem(arr, len - 1)->next : &arr->first) = item;
    memcpy(item->data, elt, arr->elt_size);

    return item;
}

struct array_item *arrayInsert(struct array_head *arr,
                               size_t index,
                               void *elt)
{
    size_t len = arr->length;
    if (index > len) return NULL;

    struct array_item *new_item = makeNewArrayItem(arr->elt_size);
    if (new_item == NULL) return NULL;

    struct array_item *prev = index
        ? getArrayItem(arr, index - 1) : NULL;
    struct array_item *next = (index == len)
        ? NULL : (index ? prev->next : arr->first);

    new_item->next = next;
    *(index ? &prev->next : &arr->first) = new_item;

    memcpy(new_item->data, elt, arr->elt_size);
    arr->length += 1;

    return new_item;
}

struct array_item *arrayPush(struct array_head *arr, void *elt)
{
    return arrayInsert(arr, 0, elt);
}

void arrayRemove(struct array_head *arr, size_t index)
{
    size_t len = arr->length;
    if (index >= len || !len) return;

    struct array_item *prev_item =
        index ? getArrayItem(arr, index - 1) : NULL;
    struct array_item *item = index ? prev_item->next : arr->first;
    struct array_item *next_item = (index < len - 1)
        ? item->next : NULL;
    *(index ? &prev_item->next : &arr->first) = next_item;
    free(item);
    arr->length -= 1;
}


// ITERATION AND SEARCHING

void arrayIter(struct array_head *arr,
               arrayIterFunc f,
               void *user_data)
{
    if (!arr->length) return;

    struct array_item *item = arr->first;
    struct array_item *nitem;

    for (size_t i = 0; i < arr->length; i++) {
        nitem = item->next;
        if (f(arr, i, item, user_data))
            break;
        item = nitem;
    }
}

struct array_item *findArrayItem(struct array_head *arr,
                                 arrayIterFunc predicate,
                                 void *user_data,
                                 size_t *idx_return)
{
    struct array_item *itm;
    size_t idx;

    FOR_ARRAY(arr, idx, itm)
        if ((predicate != NULL)
            ? predicate(arr, idx, itm, user_data)
            : !memcmp(itm->data, user_data, arr->elt_size)) {
            if (idx_return) *idx_return = idx;
            return itm;
        }

    return NULL;
}
