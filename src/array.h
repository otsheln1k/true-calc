#ifndef ARRAY_INCLUDED
#define ARRAY_INCLUDED

#include <stdlib.h>
#include <string.h>


struct array_head {
    size_t elt_size, length;
    struct array_item *first;
};

struct array_item {
    struct array_item *next;
    unsigned char data[0];
};

typedef int (*arrayIterFunc)(struct array_head *arr,
                             size_t idx,
                             struct array_item *item,
                             void *user_data);


#define FOR_ARRAY_ITEMS(arr, index_var, item_var)     \
    for (item_var = arr->first, index_var = 0;  \
         (++index_var) < arr->length;           \
         item_var = item_var->next)

#define ARRAY_TAIL(arr, stt)                    \
    arraySlice((arr), (stt), (arr)->length)

#define ARRAY_CONV(type, count, items)              \
    convertedArray(((type[count])items), (count), sizeof(type))

struct array_head *makeArray(size_t elt_size);
struct array_head *convertedArray(void *carray,
                                  size_t len,
                                  size_t elt_size);
void *arrayJoint(struct array_head * arr);
struct array_item *destroyArray(struct array_head *arr);
void destroyArrayHeader(struct array_head *slc);
struct array_head *arraySlice(struct array_head *arr,
                               size_t stt,
                              size_t end);
struct array_item *arrayDetach(struct array_head *arr);
struct array_head *arrayCopy(struct array_head *original);
struct array_item *arrayPart(struct array_head *arr,
                             size_t from,
                             size_t to);
struct array_item *arrayReverse(struct array_head *arr);
struct array_item *arrayReverseSaveLink(struct array_head *arr);
struct array_item *arrayClear(struct array_head *arr);
struct array_item *getArrayItem(struct array_head *arr, size_t index);
struct array_item * arrayAppend(struct array_head * arr, void *elt);
struct array_item * arrayInsert(struct array_head *arr, size_t index, void *elt);
struct array_item * arrayPush(struct array_head *arr, void *elt);
void arrayRemove(struct array_head *arr, size_t index);
void arrayIter(struct array_head *arr,
               arrayIterFunc f,
               void *user_data);
struct array_item *findArrayItem(struct array_head *arr,
                                 arrayIterFunc predicate,
                                 void *user_data,
                                 size_t *idx_return);


// COMPATIBILITY

#define FOFFSET(t, f) (((void *)&((t *)0)->f) - ((void *)0))

typedef struct array_head *array;
static inline unsigned int getArrayLength(array arr)
{ return (unsigned int)arr->length; }
static inline void *getArrayItemValue(array arr, unsigned int idx)
{ return getArrayItem(arr, idx)->data; }
static inline void *getArrayNextItem(void *item_val)
{
    // like container_of()
    return ((struct array_item *)
            (item_val - FOFFSET(struct array_item, data)))
        ->next->data;
}
static inline void setArrayItemValue(array arr,
                                     unsigned int idx,
                                     void *data)
{
    memcpy(getArrayItemValue(arr, idx), data, arr->elt_size);
}

#define FOR_ARRAY_OLD(arr, idxvar, type, itemvar)           \
    type *itemvar;                                          \
    for (itemvar = (type *)getArrayItem(arr, idxvar)->data; \
         idxvar < arr->length;                              \
         idxvar += 1,                                       \
             itemvar = (type *)                             \
             ((struct array_item *)                         \
              ((void *)itemvar                              \
               - (void *)FOFFSET(struct array_item, data))) \
             ->next->data)

#ifndef NEW_FOR_ARRAY

#define FOR_ARRAY(...) FOR_ARRAY_OLD(__VA_ARGS__)

#else

#define FOR_ARRAY(...) FOR_ARRAY_ITEMS(__VA_ARGS__)

#endif

#define FOR_ARRAY_COUNTER(arr, idxvar, type, itemvar)   \
    unsigned int idxvar = 0;                            \
    FOR_ARRAY_OLD(arr, idxvar, type, itemvar)

static inline array arrayDetached(array arr)
{
    arrayDetach(arr);
    return arr;
}

static inline array arrayReversed(array arr)
{
    arrayReverseSaveLink(arr);
    return arr;
}

static inline unsigned int getArrayItemIndex(array arr, void *data)
{
    FOR_ARRAY_COUNTER(arr, idx, void, itm)
        if (data == itm || !memcmp(data, itm, arr->elt_size))
            return idx;
    return arr->length;
}

static inline void *getArrayItemByVal(array arr, void *data)
{
    FOR_ARRAY_COUNTER(arr, idx, void, itm)
        if (data == itm || !memcmp(data, itm, arr->elt_size))
            return itm;
    return NULL;
}


// NOTES

// arraySlice shared contents with its original array
// arrayPart: slice, actually removes data
// arrayDetached: keep head, copy contents
// arrayCopy: copy both
// convertedArray: c-array -> linked-list
// arrayJoint: linked-list -> c-array

#endif
