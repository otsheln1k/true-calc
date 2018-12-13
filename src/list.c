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



// CONVERTING TO/FROM C ARRAYS

struct list_head *listFromArray(void *carray,
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

void *listToArray(struct list_head *lst)
{
    size_t len = lst->length;
    void *a = malloc(lst->elt_size * len);
    size_t elt_size = lst->elt_size;
    struct list_item *item = lst->first;

    for (size_t i = 0; i < len; i++) {
        memcpy(a + i * elt_size, item->data, elt_size);
        item = item->next;
    }

    return a;
}


// DESTRUCTORS

void destroyListHeader(struct list_head *slc)
{
    free(slc);
}

struct list_item *destroyList(struct list_head *lst)
{
    struct list_item *item = listClear(lst);
    destroyListHeader(lst);
    return item;
}


// GLOBAL ACTIONS

void listAlias(struct list_head *dest,
               struct list_head *src)
{
    *dest = *src;
}

struct list_head *listSlice(struct list_head *lst,
                            size_t stt,
                            size_t end)
{
    return takeListItems(getListItem(lst, stt),
                         end - stt,
                         lst->elt_size);
}

struct list_item *listDetach(struct list_head *lst)
{
    size_t idx = 0;
    size_t len = lst->length;
    struct list_item *previtm, *newitm, *itm;

    for (itm = lst->first;
         idx < len;
         previtm = newitm, idx++, itm = itm->next) {
        newitm = makeListItem(lst->elt_size);
        memcpy(newitm->data, itm->data, lst->elt_size);
        *(idx ? &previtm->next : &lst->first) = newitm;
    }

    return itm;
}

struct list_head *listCopy(struct list_head *original)
{
    struct list_head *res = makeList(original->elt_size);

    listAlias(res, original);
    listDetach(res);

    return res;
}

struct list_item *listPart(struct list_head *lst,
                           size_t from,
                           size_t to)
{
    if (from >= to)
        return listClear(lst);

    struct list_item *after;

    // remove tail
    after = destroyList(listSlice(lst, to, lst->length));

    // remote head
    lst->first = destroyList(listSlice(lst, 0, from));
    lst->length = to - from;

    return after;
}

struct list_item *listReverse(struct list_head *lst)
{
    size_t len = lst->length;
    if (len < 2)
        return len ? lst->first->next : NULL;

    struct list_item *origin = lst->first;
    struct list_item *this;
    struct list_item *next;

    for (size_t idx = 1; idx < len; idx++) {
        this = origin->next;
        next = this->next;
        this->next = lst->first;
        lst->first = this;
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

/* Same, but swap the first and last elements' data
 * to keep the old links to the first element of
 * the list (or slice) valid.
 */
struct list_item *listReverseSaveLink(struct list_head *lst)
{
    struct list_item *item;
    struct list_head *slice;

    switch (lst->length) {
    case 0:
        return NULL;
    case 1:
        return lst->first->next;
    case 2:
        item = lst->first->next;
        break;
    default:
        slice = listSlice(lst, 1, lst->length - 1);
        item = listReverse(slice);
        lst->first->next = slice->first;
        destroyListHeader(slice);
        break;
    }

    swap_data(item, lst->first, lst->elt_size);

    return item->next;
}

struct list_item *listClear(struct list_head *lst)
{
    size_t i = lst->length;
    struct list_item *item = lst->first, *next;

    while (i--) {
        next = item->next;
        free(item);
        item = next;
    }

    lst->length = 0;
    lst->first = NULL;

    return item;
}


// ITEMS

struct list_item *getListItem(struct list_head *lst, size_t index)
{
    if (index >= lst->length)
        return NULL;

    struct list_item *item = lst->first;

    for (size_t i = 0; i < index; i++, item = item->next);

    return item;
}

struct list_item *listInsert(struct list_head *lst,
                             size_t index,
                             void *elt)
{
    size_t len = lst->length;
    if (index > len)
        return NULL;

    struct list_item *new_item = makeListItem(lst->elt_size);
    if (new_item == NULL)
        return NULL;

    struct list_item *prev = index ? getListItem(lst, index - 1) : NULL;
    struct list_item *next = index ? prev->next : lst->first;

    new_item->next = next;
    *(index ? &prev->next : &lst->first) = new_item;

    memcpy(new_item->data, elt, lst->elt_size);
    lst->length += 1;

    return new_item;
}

struct list_item *listAppend(struct list_head *lst, void *elt)
{
    return listInsert(lst, lst->length, elt);
}

// REMOVE?
struct list_item *listPush(struct list_head *lst, void *elt)
{
    return listInsert(lst, 0, elt);
}

struct list_item *listPop(struct list_head *lst, size_t index)
{
    size_t len = lst->length;
    if (index >= len) return NULL;

    struct list_item *prev_item =
        index ? getListItem(lst, index - 1) : NULL;
    struct list_item *item = index ? prev_item->next : lst->first;
    struct list_item *next_item = (index < len - 1)
        ? item->next : NULL;
    *(index ? &prev_item->next : &lst->first) = next_item;
    lst->length -= 1;
    return item;
}

void listRemove(struct list_head *lst, size_t index)
{
    free(listPop(lst, index));
}


// ITERATION AND SEARCHING

void listIter(struct list_head *lst,
              listIterFunc f,
              void *user_data)
{
    if (!lst->length) return;

    struct list_item *item = lst->first;
    struct list_item *nitem;

    for (size_t i = 0; i < lst->length; i++) {
        nitem = item->next;
        if (f(lst, i, item, user_data))
            break;
        item = nitem;
    }
}

struct list_item *findListItem(struct list_head *lst,
                               listIterFunc predicate,
                               void *user_data,
                               size_t *idx_return)
{
    struct list_item *itm;
    size_t idx;

    FOR_LIST(lst, idx, itm)
        if ((predicate != NULL)
            ? predicate(lst, idx, itm, user_data)
            : !memcmp(itm->data, user_data, lst->elt_size)) {
            if (idx_return) *idx_return = idx;
            return itm;
        }

    return NULL;
}
