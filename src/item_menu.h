#ifndef ITEM_MENU_INCLUDED
#define ITEM_MENU_INCLUDED

#include <pebble.h>

struct ItemMenuLayer;
typedef struct ItemMenuLayer ItemMenuLayer;

typedef unsigned int (*imItemCountCallback)(ItemMenuLayer *im, void *cb_context);

typedef const char *(*imItemTitleCallback)(ItemMenuLayer *im, unsigned int index, void *cb_context);

typedef void (*imItemCallback)(ItemMenuLayer *im, unsigned int index, void *cb_context);

typedef void (*imBackClickCallback)(ItemMenuLayer *im, void *cb_context);

struct ItemMenuLayer {
    ScrollLayer *sl;
    Layer *l;
    GFont font;
    unsigned int padding;
    unsigned int margin;
    unsigned int cindex;
    void *cb_ctx;
    imItemCallback sel_cb;
    imBackClickCallback bck_cb;
    imItemCallback cur_changed_cb;
};

ItemMenuLayer *item_menu_create();

void item_menu_destroy(ItemMenuLayer *im);

void item_menu_set_font(ItemMenuLayer *im, GFont font);

GFont item_menu_get_font(ItemMenuLayer *im);

void item_menu_set_padding(ItemMenuLayer *im, unsigned int padding);

unsigned int item_menu_get_padding(ItemMenuLayer *im);

void item_menu_set_margin(ItemMenuLayer *im, unsigned int margin);

unsigned int item_menu_get_margin(ItemMenuLayer *im);

void item_menu_set_current_index(ItemMenuLayer *im, unsigned int current);

unsigned int item_menu_get_current_index(ItemMenuLayer *im);

void item_menu_next_item(ItemMenuLayer *im, bool forward);

void item_menu_set_callbacks(ItemMenuLayer *im, imItemCountCallback countCb, imItemTitleCallback titleCb,
        imItemCallback selCb, imBackClickCallback bckCb, imItemCallback curChangedCb);

void item_menu_set_callback_context(ItemMenuLayer *im, void *context);

void item_menu_set_click_config_onto_window(ItemMenuLayer *im, struct Window *window);

Layer *item_menu_get_layer(ItemMenuLayer *im);

ScrollLayer *item_menu_get_scroll_layer(ItemMenuLayer *im);

#endif

