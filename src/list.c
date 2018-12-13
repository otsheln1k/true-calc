#include <stdlib.h>
#include <string.h>

#define NEW_FOR_LIST
#include "list.h"


// CONSTRUCTORS

struct list_head *makeList(size_t elt_size)
{
    struct list_head *arr = malloc(sizeof(*arr));
    if (arr == NULL)
        return NULL;
    *arr = (struct list_head){
        .elt_size = elt_size,
        .length = 0,
        .first = NULL,
    };

    return arr;
}

struct list_item *makeListItem(size_t elt_size) {
    struct list_item *item = malloc(sizeof(*item) + elt_size);
    item->next = NULL;
    return item;
}

struct list_head *takeListItems(struct list_item *head,
                                size_t length,
                                size_t elt_size)
{
    struct list_head *hd = makeList(elt_size);
    hd->length = length;
    hd->first = head;
    return hd;
}



// CONVERTING TO/FROM C LISTS

struct list_head *convertedList(void *carray,
                                size_t len,
                                size_t elt_size)
{
    struct list_head *arr = makeList(elt_size);
    struct list_item *last, *new;

    arr->length = len;
    arr->elt_size = elt_size;

    for (size_t i = 0; i < len; i++) {
        new = makeListItem(elt_size);
        memcpy(new->data, carray + i * elt_size, elt_size);
        *(i ? &last->next : &arr->first) = new;
        last = new;
    }

    return arr;
}

void *listJoint(struct list_head * arr)
{
    size_t len = arr->length;
    void *a = calloc(len, arr->elt_size);
    size_t elt_size = arr->elt_size;
    struct list_item *item = arr->first;

    for (size_t i = 0; i < len; i++) {
        memcpy(a + i * elt_size, item->data, elt_size);
        item = item->next;
    }

    return a;
}


// DESTRUCTORS

struct list_item *destroyList(struct list_head *arr)
{
    struct list_item *item = listClear(arr);
    free(arr);
    return item;
}

void destroyListHeader(struct list_head *slc)
{
    free(slc);
}


// GLOBAL ACTIONS

void listAlias(struct list_head *dest,
               struct list_head *src)
{
    *dest = *src;
}

struct list_head *listSlice(struct list_head *arr,
                            size_t stt,
                            size_t end)
{
    return takeListItems(getListItem(arr, stt),
                         end - stt,
                         arr->elt_size);
}

struct list_item *listDetach(struct list_head *arr)
{
    size_t idx = 0;
    size_t len = arr->length;
    struct list_item *previtm, *newitm, *itm;

    for (itm = arr->first;
         idx < len;
         previtm = newitm, idx++, itm = itm->next) {
        newitm = makeListItem(arr->elt_size);
        memcpy(newitm->data, itm->data, arr->elt_size);
        *(idx ? &previtm->next : &arr->first) = newitm;
    }

    return itm;
}

struct list_head *listCopy(struct list_head *original)
{
    struct list_head *res = makeList(original->elt_size);

    res->first = original->first;
    res->length = original->length;
    listDetach(res);

    return res;
}

struct list_item *listPart(struct list_head *arr,
                             size_t from,
                             size_t to)
{
    if (from >= to)
        return listClear(arr);

    size_t len = to - from;
    struct list_head *slice;
    struct list_item *itm, *after;

    // remove tail
    slice = listSlice(arr, to, arr->length);
    after = destroyList(slice);
    // remote head
    slice = listSlice(arr, 0, from);
    itm = destroyList(slice);

    arr->first = itm;
    arr->length = len;

    return after;
}

struct list_item *listReverse(struct list_head *arr)
{
    size_t len = arr->length;
    if (len < 2)
        return len ? arr->first->next : NULL;

    struct list_item *origin = arr->first;
    struct list_item *this;
    struct list_item *next;

    for (size_t idx = 1; idx < len; idx++) {
        this = origin->next;
        next = this->next;
        this->next = arr->first;
        arr->first = this;
        origin->next = next;
    }

    return origin->next;
}

static void swap_data(struct list_item *a,
                      struct list_item *b,
                      size_t elt_sz)
{
    unsigned char c;

    unsigned char *ap = a->data;
    unsigned char *bp = b->data;

    for (size_t i = 0; i < elt_sz; i++) {
        c = ap[i];
        ap[i] = bp[i];
        bp[i] = c;
    }
}

struct list_item *listReverseSaveLink(struct list_head *arr)
{
    struct list_item *item;
    struct list_head *slice;

    switch (arr->length) {
    case 0:
        return NULL;
    case 1:
        return arr->first->next;
    case 2:
        item = arr->first->next;
        break;
    default:
        slice = listSlice(arr, 1, arr->length - 1);
        item = listReverse(slice);
        arr->first->next = slice->first;
        destroyListHeader(slice);
        break;
    }

    swap_data(item, arr->first, arr->elt_size);

    return item->next;
}

struct list_item *listClear(struct list_head *arr)
{
    size_t i = arr->length;
    struct list_item *item = arr->first, *next;

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

struct list_item *getListItem(struct list_head *arr, size_t index)
{
    if (index >= arr->length)
        return NULL;

    struct list_item *item = arr->first;

    for (size_t i = 0; i < index; i++, item = item->next);

    return item;
}

struct list_item *listAppend(struct list_head * arr, void *elt)
{
    size_t len = arr->length;
    struct list_item *item = makeListItem(arr->elt_size);

    if (item == NULL) return NULL;

    arr->length += 1;

    *(len ? &getListItem(arr, len - 1)->next : &arr->first) = item;
    memcpy(item->data, elt, arr->elt_size);

    return item;
}

struct list_item *listInsert(struct list_head *arr,
                               size_t index,
                               void *elt)
{
    size_t len = arr->length;
    if (index > len) return NULL;

    struct list_item *new_item = makeListItem(arr->elt_size);
    if (new_item == NULL) return NULL;

    struct list_item *prev = index
        ? getListItem(arr, index - 1) : NULL;
    struct list_item *next = (index == len)
        ? NULL : (index ? prev->next : arr->first);

    new_item->next = next;
    *(index ? &prev->next : &arr->first) = new_item;

    memcpy(new_item->data, elt, arr->elt_size);
    arr->length += 1;

    return new_item;
}

struct list_item *listPush(struct list_head *arr, void *elt)
{
    return listInsert(arr, 0, elt);
}

struct list_item *listPop(struct list_head *arr, size_t index)
{
    size_t len = arr->length;
    if (index >= len) return NULL;

    struct list_item *prev_item =
        index ? getListItem(arr, index - 1) : NULL;
    struct list_item *item = index ? prev_item->next : arr->first;
    struct list_item *next_item = (index < len - 1)
        ? item->next : NULL;
    *(index ? &prev_item->next : &arr->first) = next_item;
    arr->length -= 1;
    return item;
}

void listRemove(struct list_head *arr, size_t index)
{
    free(listPop(arr, index));
}


// ITERATION AND SEARCHING

void listIter(struct list_head *arr,
               listIterFunc f,
               void *user_data)
{
    if (!arr->length) return;

    struct list_item *item = arr->first;
    struct list_item *nitem;

    for (size_t i = 0; i < arr->length; i++) {
        nitem = item->next;
        if (f(arr, i, item, user_data))
            break;
        item = nitem;
    }
}

struct list_item *findListItem(struct list_head *arr,
                                 listIterFunc predicate,
                                 void *user_data,
                                 size_t *idx_return)
{
    struct list_item *itm;
    size_t idx;

    FOR_LIST(arr, idx, itm)
        if ((predicate != NULL)
            ? predicate(arr, idx, itm, user_data)
            : !memcmp(itm->data, user_data, arr->elt_size)) {
            if (idx_return) *idx_return = idx;
            return itm;
        }

    return NULL;
}
