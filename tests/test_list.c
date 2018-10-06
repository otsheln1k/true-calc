#include <string.h>

#include <list.h>
#include <test_ut.h>

#define GET_INT(arr,idx) *(int *)getListItemValue(arr, idx)

void listAppendInt(struct list_head *arr, int i) {
    listAppend(arr, &i);
}

void listInsertInt(struct list_head *arr, unsigned int idx, int i) {
    listInsert(arr, idx, &i);
}

void listSetInt(struct list_head *arr, unsigned int idx, int i) {
    setListItemValue(arr, idx, &i);
}

bool test_array_AppendGet() {
    struct list_head *arr = makeList(sizeof(int));
    listAppendInt(arr, 42);
    listAppendInt(arr, 100);
    listAppendInt(arr, 12);
    bool res = \
           ASSERT(GET_INT(arr, 1) == 100)
        && ASSERT(GET_INT(arr, 0) == 42)
        && ASSERT(GET_INT(arr, 2) == 12);
    destroyList(arr);
    return res;
}

bool test_array_SetInsert() {
    struct list_head *arr = makeList(sizeof(int));
    listAppendInt(arr, 42);
    listAppendInt(arr, 0);
    listInsertInt(arr, 1, 100500);
    if (!ASSERT(GET_INT(arr, 1) == 100500)) {
        bool res = false;
        goto t_a_SI_ret;
    }
    listSetInt(arr, 2, 15);
    bool res = \
           ASSERT(GET_INT(arr, 2) == 15)
        && ASSERT(GET_INT(arr, 1) == 100500)
        && ASSERT(GET_INT(arr, 0) == 42);
t_a_SI_ret:
    destroyList(arr);
    return res;
}

bool test_array_ConvSlice() {
    struct list_head *arr = convertedList((int[6]){15, 32, 2, 28, 90, 42}, 6, sizeof(int));
    if (!ASSERT(arr->length == 6)
     || !ASSERT(GET_INT(arr, 3) == 28)) {
        bool res = false;
        goto t_a_CS_ret;
    }
    struct list_head *slc = listSlice(arr, 1, 4);
    bool res = \
           ASSERT(slc->length == 3)
        && ASSERT(GET_INT(slc, 0) == 32)
        && ASSERT(GET_INT(slc, 1) == 2)
        && ASSERT(GET_INT(slc, 2) == 28);
    destroyListHeader(slc);
t_a_CS_ret:
    destroyList(arr);
    return res;
}

typedef struct {
    int i;
    double f;
} SType;

bool test_array_ConvEx() {
    struct list_head *stringarr = convertedList((char*[3]){ "abc", "def", "ghi" }, 3, sizeof(char *));
    if (!ASSERT(strcmp(*(char **)getListItemValue(stringarr, 1), "def") == 0)) {
        printf("%s\n", *(char **)getListItemValue(stringarr, 1));
        bool res = false;
        goto t_a_CE_ret_1;
    }
    struct list_head *structarr = convertedList((SType[4]){ { 1, 1.2 }, { 2, 2.2 }, { 3, 3.2 }, { 4, 4.2 } }, 4, sizeof(SType));
    if (!(ASSERT(((SType *)getListItemValue(structarr, 2))->f == 3.2)
        &&ASSERT((*(SType *)getListItemValue(structarr, 1)).i == 2))) {
        bool res = false;
        goto t_a_CE_ret_2;
    }
    bool res = \
           ASSERT(stringarr->length == 3)
        && ASSERT(structarr->length == 4);
t_a_CE_ret_2:
    destroyList(structarr);
t_a_CE_ret_1:
    destroyList(stringarr);
    return res;
}

#define GET_LIST(arr,idx) *(struct list_head **)getListItemValue(arr, idx)

bool test_array_2dim() {
    struct list_head *arr = makeList(sizeof(list));
    struct list_head *a01 = convertedList((int[6]){0, 1, 2, 3, 4, 5}, 6, sizeof(int));
    listAppend(arr, &a01);
    struct list_head *a02 = convertedList((int[6]){7, 8, 9, 10, 11, 12}, 6, sizeof(int));
    listAppend(arr, &a02);
    bool res = \
           ASSERT(GET_INT(GET_LIST(arr, 0), 2) == 2)
        && ASSERT(GET_INT(GET_LIST(arr, 1), 2) == 9)
        && ASSERT(GET_INT(GET_LIST(arr, 0), 0) == 0);
    destroyList(a01);
    destroyList(a02);
    destroyList(arr);
    return res;
}

bool test_array_NextItem() {
    struct list_head *arr = convertedList((int[6]){0, 1, 2, 3, 4, 5}, 6, sizeof(int));
    int *item2 = getListItemValue(arr, 2);
    int *item3 = getListNextItem(item2);
    bool res = \
           ASSERT(*item2 == 2)
        && ASSERT(*item3 == 3);
    destroyList(arr);
    return res;
}

bool test_array_For() {
    struct list_head *arr = convertedList((int[6]){ 15, -1, 42, 146, 2, 0 }, 6, sizeof(int));
    unsigned int i = 0;
    bool res = false;
    FOR_LIST_COUNTER(arr, idx, int, itm) {
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
    destroyList(arr);
    return res;
}

bool test_array_Copy() {
    struct list_head *arr0 = convertedList((int [6]){ 5, 2, 41, -1, 0, 66 }, 6, sizeof(int));
    struct list_head *arr1 = listCopy(LIST_TAIL(arr0, 2));
    int i = 42;
    setListItemValue(arr0, 2, &i);
    bool res =
           ASSERT(GET_INT(arr0, 2) == 42)
        && ASSERT(GET_INT(arr1, 0) == 41)
        && ASSERT(GET_INT(arr0, 3) == GET_INT(arr1, 1))
        && ASSERT(GET_INT(arr0, 4) == GET_INT(arr1, 2))
        && ASSERT(GET_INT(arr0, 5) == GET_INT(arr1, 3));
    destroyList(arr0);
    destroyList(arr1);
    return res;
}

bool test_array_Detach() {
    struct list_head *arr = convertedList((int[6]){ 1, 2, 3, 0, -1, -2 }, 6, sizeof(int));
    struct list_head *slc = LIST_TAIL(arr, 3);
    int i = -3;
    setListItemValue(arr, 3, &i);
    if (!(ASSERT(GET_INT(arr, 3) == GET_INT(slc, 0))
       && ASSERT(GET_INT(arr, 4) == GET_INT(slc, 1))
       && ASSERT(GET_INT(arr, 5) == GET_INT(slc, 2)))) {
        bool res = false;
        destroyListHeader(slc);
        goto t_a_D_ret;
    }
    listDetached(slc);
    i = -4;
    setListItemValue(arr, 5, &i);
    bool res = \
            ASSERT(GET_INT(arr, 3)==GET_INT(slc, 0))
         && ASSERT(GET_INT(arr, 4)==GET_INT(slc, 1))
         && ASSERT(GET_INT(arr, 5) + 2 == GET_INT(slc, 2));
    destroyList(slc);
t_a_D_ret:
    destroyList(arr);
    return res;
}

bool test_array_Search() {
    struct list_head *arr = LIST_CONV(int, 6, ARG({ 1, 1, 2, 3, 5, 8 }));
    int a = 5;
    int b = 6;
    bool res = ASSERT(getListItemIndex(arr, &a) == 4)
            && ASSERT(getListItemIndex(arr, &b) == 6)
            && ASSERT(getListItemByVal(arr, &a) == getListItemValue(arr, 4))
            && ASSERT(getListItemByVal(arr, &b) == NULL);
    destroyList(arr);
    return res;
}

bool test_array_Reverse() {
    struct list_head *arr = LIST_CONV(int, 8, ARG({ 0, 1, 2, 3, 4, 5, 6, 7 }));
    struct list_head *slc = listSlice(arr, 2, 6);
    listReversed(slc);
    bool res = \
            ASSERT(GET_INT(arr, 1) == 1)
         && ASSERT(GET_INT(arr, 2) == 5)
         && ASSERT(GET_INT(arr, 3) == 4)
         && ASSERT(GET_INT(arr, 4) == 3)
         && ASSERT(GET_INT(arr, 5) == 2)
         && ASSERT(GET_INT(arr, 6) == 6)
         && ASSERT(GET_INT(arr, 7) == 7);
    destroyListHeader(slc);
    destroyList(arr);
    return res;
}

bool test_array_DoubleReverse() {
    struct list_head *arr = LIST_CONV(int, 6, ARG({ 1, 2, 3, 4, 5, 6 }));
    struct list_head *slc = listReversed(listReversed(listSlice(arr, 1, 5)));
    bool res = \
            ASSERT(GET_INT(arr, 0) == 1)
         && ASSERT(GET_INT(arr, 1) == 2)
         && ASSERT(GET_INT(arr, 2) == 3)
         && ASSERT(GET_INT(arr, 3) == 4)
         && ASSERT(GET_INT(arr, 4) == 5)
         && ASSERT(GET_INT(arr, 5) == 6);
    destroyListHeader(slc);
    destroyList(arr);
    return res;
}

bool test_array_Part() {
    struct list_head *arr = LIST_CONV(int, 8, ARG({ 1, 2, 4, 8, 16, 32, 64, 128 }));
    struct list_head *slc = listSlice(arr, 0, 7);
    destroyListHeader(arr);
    struct list_item *after = listPart(slc, 1, 6);
    bool res = \
        ASSERT(GET_INT(slc, 0) == 2)
        && ASSERT(GET_INT(slc, 1) == 4)
        && ASSERT(GET_INT(slc, 2) == 8)
        && ASSERT(GET_INT(slc, 3) == 16)
        && ASSERT(GET_INT(slc, 4) == 32)
        && ASSERT(slc->length == 5)
        && ASSERT(*(int *)after->data == 128);
    destroyList(slc);
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

