#ifndef NUMBER_LAYER_INCLUDED
#define NUMBER_LAYER_INCLUDED

#include <pebble.h>
#include "list.h"

double pow_di(double b, int p);

struct NumberLayer;
typedef struct NumberLayer NumberLayer;

/*
 * not so sure why we need NLMinus
 * it’s useful, however, as the lower bound
 */
typedef enum {
    // NLZero = 0, NLNine = 9, NLPoint = 10, NLDone = 11, NLMinus = 12
    NLMinus = 0, NLPoint = 1, NLUnderscore = 2, NLDone = 3, NLZero = 4, NLNine = 13, NLA = 14, NLZ = 39
} NumberLayerItem;

typedef void (*nlCallback)(NumberLayer *nl, void *context);

struct NumberLayer {
    Layer *l;
    bool identifier;    // if true, show an underscore instead of the decimal point and don't allow first digits
    bool negative;      // if true, draw a minus block. ignored if identifier is true
    char base;          // number base. ignored if identifier is true
    struct list_head *items;        // reversed struct list_head *of NumberLayerItem
    nlCallback done_cb;
    nlCallback cncl_cb;
    NumberLayerItem current;
    void *cb_ctx;
};

NumberLayer *number_layer_create(GRect);

void number_layer_destroy(NumberLayer *);

double number_layer_get_number(NumberLayer *);

char *number_layer_get_string(NumberLayer *);

void number_layer_next(NumberLayer *);

void number_layer_prev(NumberLayer *);

void number_layer_finish(NumberLayer *);

void number_layer_select(NumberLayer *, bool);

void number_layer_set_negative(NumberLayer *, bool);

bool number_layer_get_negative(NumberLayer *);

void number_layer_set_base(NumberLayer *, char);

bool number_layer_get_base(NumberLayer *);

void number_layer_set_identifier(NumberLayer *nl, bool is_ident);

bool number_layer_get_identifier(NumberLayer *nl);

Layer *number_layer_get_layer(NumberLayer *);

void number_layer_set_callbacks(NumberLayer *, nlCallback, nlCallback);

void number_layer_set_callback_context(NumberLayer *, void *);

void number_layer_set_click_config_onto_window(NumberLayer *, struct Window *);  // here

#endif
