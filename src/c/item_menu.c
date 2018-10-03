#include <pebble.h>

#include "item_menu.h"

#define ITEM_MENU_CB_DATA(layer) ((struct imCallbackData *)layer_get_data(layer))

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

struct imCallbackData {
    ItemMenuLayer *iml;
    imItemCountCallback countCb;
    imItemTitleCallback titleCb;
};

static void imUpdateProc(Layer *layer, GContext *ctx);
static void imClickConfig(void *context);
static void imUpClick(ClickRecognizerRef recognizer, void *context);
static void imDownClick(ClickRecognizerRef recognizer, void *context);
static void imSelectClick(ClickRecognizerRef recognizer, void *context);
static void imBackClick(ClickRecognizerRef recognizer, void *context);

static void imClickConfig(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 150, imUpClick);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 150, imDownClick);
    window_single_click_subscribe(BUTTON_ID_SELECT, imSelectClick);
    window_single_click_subscribe(BUTTON_ID_BACK, imBackClick);
}

static void imUpClick(ClickRecognizerRef recognizer, void *context) {
    item_menu_next_item((ItemMenuLayer *)context, false);
}

static void imDownClick(ClickRecognizerRef recognizer, void *context) {
    item_menu_next_item((ItemMenuLayer *)context, true);
    // scroll_layer_set_content_offset(((ItemMenuLayer *)context)->sl, GPoint(0, -40), true);
}

static void imSelectClick(ClickRecognizerRef recognizer, void *context) {
    ItemMenuLayer *im = (ItemMenuLayer *)context;
    if (im->sel_cb != NULL)
        im->sel_cb(im, item_menu_get_current_index(im), im->cb_ctx);
    layer_mark_dirty(im->l);
}

static void imBackClick(ClickRecognizerRef recognizer, void *context) {
    ItemMenuLayer *im = (ItemMenuLayer *)context;
    if (im->bck_cb != NULL)
        im->bck_cb(im, im->cb_ctx);
    else
        window_stack_remove(window_stack_get_top_window(), true);
}

static void imUpdateProc(Layer *layer, GContext *ctx) {
    const unsigned int count = (ITEM_MENU_CB_DATA(layer)->countCb)(
            ITEM_MENU_CB_DATA(layer)->iml,
            ITEM_MENU_CB_DATA(layer)->iml->cb_ctx);
    const int padding = ITEM_MENU_CB_DATA(layer)->iml->padding;
    const int margin = ITEM_MENU_CB_DATA(layer)->iml->margin;
    static int16_t off = 0;  // imaginary content offset; ignores animations
    GFont font = ITEM_MENU_CB_DATA(layer)->iml->font;  // here
    GSize layer_size = layer_get_bounds(layer).size;
    unsigned int x = margin + padding;
    unsigned int y = x;
    unsigned int sel_idx = ITEM_MENU_CB_DATA(layer)->iml->cindex;
    unsigned int h = 0;
    /* settings */
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_text_color(ctx, GColorWhite);
    for (unsigned int index = 0; index < count; index++) {
        char *str = ITEM_MENU_CB_DATA(layer)->titleCb(
                ITEM_MENU_CB_DATA(layer)->iml,
                index,
                ITEM_MENU_CB_DATA(layer)->iml->cb_ctx);
        GSize bounds = graphics_text_layout_get_content_size(
                str,
                font,
                GRect(0, 0, layer_size.w, layer_size.h),
                GTextOverflowModeWordWrap,
                GTextAlignmentLeft);
        if (!h)
            h = bounds.h;
start_draw_item:;
        GRect rect = GRect(x, y, MAX(bounds.w, bounds.h), bounds.h);
        GRect rect_wide = grect_inset(rect, GEdgeInsets(-padding));
        /*
        GRect rect_text = rect;
        grect_align(&rect_text, &rect_wide, GAlignCenter, true);
        */
        /* if we need more space in the line than
         * there is currently available, break line */
        if ((layer_size.w - (signed)x) < (rect.size.w + margin + padding)) {
            y += margin + 2 * padding + bounds.h;   // lf
            x = margin + padding;                   // cr
            goto start_draw_item;                   // count everything again
        }
        x += rect.size.w + 2 * padding + margin;
        if (index == sel_idx) {                     // scroll if needed
            ScrollLayer *sl = ITEM_MENU_CB_DATA(layer)->iml->sl;
            int16_t ht = layer_get_frame(scroll_layer_get_layer(sl)).size.h;
            if (-off < (int16_t)(y + h + padding + margin - ht)) {
                off = ht - (y + h + padding + margin);
                scroll_layer_set_content_offset(sl, GPoint(0, off), true);
            } else if ((-off) > (int16_t)(y - padding - margin)) {
                off = padding + margin - y;
                scroll_layer_set_content_offset(sl, GPoint(0, off), true);
            }
            graphics_context_set_fill_color(ctx, GColorWhite);
            graphics_context_set_text_color(ctx, GColorBlack);
        }
        /* if we need more space in the layer than
         * there is available, extend the layer */
        if ((unsigned)layer_get_frame(layer).size.h < y + h + padding + margin) {
            layer_set_frame(layer, GRect(0, 0, layer_get_frame(layer).size.w, y + h + padding + margin));
            return imUpdateProc(layer, ctx);
        }
        graphics_fill_rect(ctx, rect_wide, 0, GCornerNone);
        graphics_draw_text(
                ctx,
                str,
                font,
                rect,
                GTextOverflowModeWordWrap,
                GTextAlignmentCenter,
                NULL);
        if (index == sel_idx) {
            graphics_draw_rect(ctx, rect_wide);
            graphics_context_set_text_color(ctx, GColorWhite);
            graphics_context_set_fill_color(ctx, GColorBlack);
        }
    }
    if (h) {
        GSize cs = GSize(layer_size.w, y + h + padding + margin);
        ScrollLayer *sl = ITEM_MENU_CB_DATA(layer)->iml->sl;
        GSize current_cs = scroll_layer_get_content_size(sl);
        if (!gsize_equal(&cs, &current_cs))
            scroll_layer_set_content_size(sl, cs);
    }
}

ItemMenuLayer *item_menu_create(GRect frame) {
    ItemMenuLayer *im = (ItemMenuLayer *)malloc(sizeof(ItemMenuLayer));
    if (im == NULL)
        return NULL;
    im->sl = scroll_layer_create(frame);
    im->l = layer_create_with_data(GRect(0, 0, frame.size.w, frame.size.h), sizeof(struct imCallbackData));
    scroll_layer_add_child(im->sl, im->l);
    ITEM_MENU_CB_DATA(im->l)->iml = im;
    layer_set_update_proc(im->l, imUpdateProc);
    scroll_layer_set_callbacks(item_menu_get_scroll_layer(im),
            (ScrollLayerCallbacks){
            .click_config_provider = imClickConfig });
    scroll_layer_set_shadow_hidden(item_menu_get_scroll_layer(im), false);
    im->font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    im->padding = 4;
    im->margin = 2;
    im->cb_ctx = NULL;
    im->sel_cb = NULL;
    im->bck_cb = NULL;
    im->cur_changed_cb = NULL;
    item_menu_set_current_index(im, 0);
    return im;
}

ScrollLayer *item_menu_get_scroll_layer(ItemMenuLayer *im) {
    return im->sl;
}

Layer *item_menu_get_layer(ItemMenuLayer *im) {
    return scroll_layer_get_layer(item_menu_get_scroll_layer(im));
    // return im->l;
}

void item_menu_set_padding(ItemMenuLayer *im, unsigned int padding) {
    im->padding = padding;
    layer_mark_dirty(im->l);
}

unsigned int item_menu_get_padding(ItemMenuLayer *im) {
    return im->padding;
}

void item_menu_set_margin(ItemMenuLayer *im, unsigned int margin) {
    im->margin = margin;
    layer_mark_dirty(im->l);
}

unsigned int item_menu_get_margin(ItemMenuLayer *im) {
    return im->margin;
}

void item_menu_set_current_index(ItemMenuLayer *im, unsigned int current) {
    im->cindex = current;
    if (im->cur_changed_cb != NULL)
        im->cur_changed_cb(im, current, im->cb_ctx);
    layer_mark_dirty(im->l);
}

unsigned int item_menu_get_current_index(ItemMenuLayer *im) {
    return im->cindex;
}

void item_menu_next_item(ItemMenuLayer *im, bool forward) {
    unsigned int cur_idx = item_menu_get_current_index(im);
    unsigned int cnt = ITEM_MENU_CB_DATA(im->l)->countCb(im, im->cb_ctx);
    if (forward)
        item_menu_set_current_index(im, (cur_idx + 1 < cnt) ? cur_idx + 1 : 0);
    else
        item_menu_set_current_index(im, cur_idx ? cur_idx - 1 : cnt - 1);
}

void item_menu_set_font(ItemMenuLayer *im, GFont font) {
    im->font = font;
}

GFont item_menu_get_font(ItemMenuLayer *im) {
    return im->font;
}

void item_menu_set_callbacks(ItemMenuLayer *im,
        imItemCountCallback countCb,
        imItemTitleCallback titleCb,
        imItemCallback selCb,
        imBackClickCallback bckCb,
        imItemCallback curChangedCb) {
    struct imCallbackData *data = ITEM_MENU_CB_DATA(im->l);
    data->countCb = countCb;
    data->titleCb = titleCb;
    im->sel_cb = selCb;
    im->bck_cb = bckCb;
    im->cur_changed_cb = curChangedCb;
    if (curChangedCb != NULL)
        curChangedCb(im, item_menu_get_current_index(im), im->cb_ctx);
    layer_mark_dirty(im->l);
}

void item_menu_set_callback_context(ItemMenuLayer *im, void *context) {
    im->cb_ctx = context;
}

void item_menu_set_click_config_onto_window(ItemMenuLayer *im, struct Window *window) {
    window_set_click_config_provider_with_context(window, imClickConfig, (void *)im);
}

void item_menu_destroy(ItemMenuLayer *im) {
    scroll_layer_destroy(item_menu_get_scroll_layer(im));
    layer_destroy(im->l);
    free(im);
}

