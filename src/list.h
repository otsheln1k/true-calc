#ifndef LIST_INCLUDED
#define LIST_INCLUDED

#include <stdlib.h>
#include <string.h>


struct list_head {
    size_t elt_size, length;
    struct list_item *first;
};

struct list_item {
    struct list_item *next;
    unsigned char data[0];
};

typedef int (*listIterFunc)(struct list_head *arr,
                             size_t idx,
                             struct list_item *item,
                             void *user_data);


#define FOR_LIST_ITEMS(arr, index_var, item_var)    \
    for (item_var = arr->first, index_var = 0;      \
         index_var < arr->length;                   \
         item_var = item_var->next, index_var++)

#define LIST_TAIL(arr, stt)                     \
    listSlice((arr), (stt), (arr)->length)

#define LIST_CONV(type, count, items)                           \
    convertedList(((type[count])items), (count), sizeof(type))

struct list_head *makeList(size_t elt_size);
struct list_item *makeListItem(size_t elt_size);
struct list_head *takeListItems(struct list_item *first,
                                size_t len,
                                size_t elt_size);
struct list_head *convertedList(void *carray,
                                size_t len,
                                size_t elt_size);
void *listJoint(struct list_head * arr);
struct list_item *destroyList(struct list_head *arr);
void destroyListHeader(struct list_head *slc);
void listAlias(struct list_head *dest, struct list_head *src);
struct list_head *listSlice(struct list_head *arr,
                            size_t stt,
                            size_t end);
struct list_item *listDetach(struct list_head *arr);
struct list_head *listCopy(struct list_head *original);
struct list_item *listPart(struct list_head *arr,
                             size_t from,
                             size_t to);
struct list_item *listReverse(struct list_head *arr);
struct list_item *listReverseSaveLink(struct list_head *arr);
struct list_item *listClear(struct list_head *arr);
struct list_item *getListItem(struct list_head *arr, size_t index);
struct list_item *listAppend(struct list_head * arr, void *elt);
struct list_item *listInsert(struct list_head *arr, size_t index, void *elt);
struct list_item *listPush(struct list_head *arr, void *elt);
struct list_item *listPop(struct list_head *arr, size_t index);
void listRemove(struct list_head *arr, size_t index);
void listIter(struct list_head *arr,
               listIterFunc f,
               void *user_data);
struct list_item *findListItem(struct list_head *arr,
                                 listIterFunc predicate,
                                 void *user_data,
                                 size_t *idx_return);


#define LIST_ITEM_DATA(type, expr) ((type *)(expr)->data)
#define LIST_ITEM_REF(type, expr) (*LIST_ITEM_DATA(type, expr))
#define LIST_ITEM_SET(type, expr, new_expr)     \
    (*LIST_ITEM_DATA(type, expr) = (new_expr))

#define LIST_DATA(type, expr, idx)              \
    LIST_ITEM_DATA(type, getListItem(expr, idx))
#define LIST_REF(type, expr, idx)               \
    LIST_ITEM_REF(type, getListItem(expr, idx))
#define LIST_SET(type, expr, idx, new_expr)     \
    LIST_ITEM_SET(type, getListItem(expr, idx), new_expr)


// COMPATIBILITY

#define FOFFSET(t, f) (((void *)&((t *)0)->f) - ((void *)0))

static inline void *getListNextItem(void *item_val)
{
    // like container_of()
    return ((struct list_item *)
            (item_val - FOFFSET(struct list_item, data)))
        ->next->data;
}
static inline void setListItemValue(struct list_head *arr,
                                     unsigned int idx,
                                     void *data)
{
    memcpy(LIST_DATA(void, arr, idx), data, arr->elt_size);
}

#define FOR_LIST_OLD(arr, idxvar, type, itemvar)           \
    type *itemvar;                                          \
    for (itemvar = (type *)getListItem(arr, idxvar)->data; \
         idxvar < arr->length;                              \
         idxvar += 1,                                       \
             itemvar = (type *)                             \
             ((struct list_item *)                         \
              ((void *)itemvar                              \
               - (void *)FOFFSET(struct list_item, data))) \
             ->next->data)

#ifndef NEW_FOR_LIST

#define FOR_LIST(...) FOR_LIST_OLD(__VA_ARGS__)

#else

#define FOR_LIST(...) FOR_LIST_ITEMS(__VA_ARGS__)

#endif

#define FOR_LIST_COUNTER(arr, idxvar, type, itemvar)    \
    unsigned int idxvar = 0;                            \
    FOR_LIST_OLD(arr, idxvar, type, itemvar)

static inline struct list_head *listDetached(struct list_head *arr)
{
    listDetach(arr);
    return arr;
}

static inline struct list_head *listReversed(struct list_head *arr)
{
    listReverseSaveLink(arr);
    return arr;
}

static inline unsigned int getListItemIndex(struct list_head *arr, void *data)
{
    FOR_LIST_COUNTER(arr, idx, void, itm)
        if (data == itm || !memcmp(data, itm, arr->elt_size))
            return idx;
    return arr->length;
}

static inline void *getListItemByVal(struct list_head *arr, void *data)
{
    FOR_LIST_COUNTER(arr, idx, void, itm)
        if (data == itm || !memcmp(data, itm, arr->elt_size))
            return itm;
    return NULL;
}


// NOTES

// listSlice shared contents with its original // TODO
// listPart: slice, actually removes data
// listDetached: keep head, copy contents
// listCopy: copy both
// convertedList: c-struct list_head *-> linked-list
// listJoint: linked-list -> c-// TODO

#endif
