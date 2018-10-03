#ifndef ARRAY_INCLUDED
#define ARRAY_INCLUDED

#include <stdlib.h>
#include <string.h>

typedef void* array;
typedef int (*arrayIterFunc)(array, unsigned int, void *);

#define FOR_ARRAY(arr,counter,itemtype,itemptr) \
    for (itemtype *itemptr = getArrayItemValue(arr, counter--); \
            (++counter) < getArrayLength(arr); \
            itemptr = getArrayNextItem(itemptr))

#define FOR_ARRAY_COUNTER(arr,cntrname,itemtype,itemptr) \
    unsigned int cntrname = 0; \
    FOR_ARRAY(arr, cntrname, itemtype, itemptr)

#define ARRAY_TAIL(arr,stt) arraySlice(arr, stt, getArrayLength(arr))

#define ARRAY_CONV(type,count,items) convertedArray((type[count])items, count, sizeof(type))

#define ARG(...) __VA_ARGS__

/* INTERFACE */

array makeArray(size_t);

array convertedArray(void *, unsigned int, size_t);

array arraySlice(array, unsigned int, unsigned int);  // depends on the original array

array arrayDetached(array);  // returns its arg, but modified

array arrayCopy(array);

array arrayReversed(array);  // see arrayDetached. also, may break headers & pointers

array arrayPart(array, unsigned int, unsigned int);  // returns its arg-1, sliced

void destroyArray(array);

void destroyArrayHeader(array);

size_t getArrayElementSize(array);

unsigned int getArrayLength(array);

void *getArrayItemValue(array, unsigned int);

void *getArrayNextItem(void *);

void setArrayItemValue(array, unsigned int, void *);

void arrayAppend(array arr, void *elt);

void arrayInsert(array, unsigned int, void *);

void arrayPush(array, void *);

void arrayRemove(array, unsigned int);

array arrayClear(array);

void *arrayJoint(array);  // make a c-array of array

void arrayIter(array, arrayIterFunc);

unsigned int getArrayItemIndex(array, void *);

void *getArrayItemByVal(array, void *);

#endif

