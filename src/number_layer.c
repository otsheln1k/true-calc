#include "number_layer.h"

static char *nlGetString(NumberLayerItem nli);
static void nlUpdateProc(Layer *layer, GContext *ctx);
static bool nlShowDecimalPoint(NumberLayer *nl);
static bool nlShowUnderscore(NumberLayer *nl);
static bool nlShowDigits(NumberLayer *nl);
static bool nlShowDone(NumberLayer *nl);
static char *nlGetString(NumberLayerItem nli);

static void nlClickConfig(void *context);
static void nlSelectLClick(ClickRecognizerRef recognizer, void *context);
static void nlUpClick(ClickRecognizerRef recognizer, void *context);
static void nlDownClick(ClickRecognizerRef recognizer, void *context);
static void nlSelectClick(ClickRecognizerRef recognizer, void *context);
static void nlBackClick(ClickRecognizerRef recognizer, void *context);

double pow_di(double b, int p) {
    if (!p)
        return 1;
    else if (p > 0)
        return b * pow_di(b, p - 1);
    else
        return 1 / (b * pow_di(b, -p - 1));
}

static void nlUpdateProc(Layer *layer, GContext *ctx) {
    /*
     * determine count
     * determine bounds
     * draw rects with text, one by one
     * draw the selected rect (different color)
     */
    static const int16_t margin = 4;
    static const int16_t padding = 4;
    static NumberLayerItem minus = NLMinus;
    GRect bounds = layer_get_bounds(layer);
    const GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    NumberLayer *nl = *(NumberLayer **)layer_get_data(layer);
    bool neg = number_layer_get_negative(nl);
    // so, da plan: draw the items in reversed order :)
    // get the max size here
    unsigned int count = neg + nl->items->length + 1;
    const GSize text_size = graphics_text_layout_get_content_size(
            "__",
            font,
            bounds,
            GTextOverflowModeWordWrap,
            GTextAlignmentLeft);
    // get the bounds here
    GRect rect = GRect(0, 0, (text_size.w + 2 * padding) * count + (count - 1) * margin, text_size.h + padding * 2);
    bounds = grect_inset(bounds, GEdgeInsets(0, -margin));
    grect_align(&rect, &bounds, GAlignCenter, true);
    GSize blck_size = GSize(text_size.w + 2 * padding, text_size.h + 2 * padding);
    /* need margin, padding and the default/max text block size */
    // draw current item here
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_text_color(ctx, GColorBlack);
    GRect cur_rect = GRect(0, 0, blck_size.w, blck_size.h);
    grect_align(&cur_rect, &rect, GAlignRight, true);
    graphics_fill_rect(ctx, cur_rect, 0, 0);
    graphics_draw_rect(ctx, cur_rect);
    if (nl->current != NLDone)
        graphics_draw_text(ctx,
                nlGetString(nl->current),
                font,
                grect_inset(cur_rect, GEdgeInsets(padding)),
                GTextOverflowModeWordWrap,
                GTextAlignmentCenter,
                NULL);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_text_color(ctx, GColorWhite);
    // iterate through the items
    unsigned int index = 0;
    for (NumberLayerItem *item = nl->items->length ? getListItemValue(nl->items, 0) : &minus;
            index < (count - 1);
            item = (index < count - 1 - neg) ? getListNextItem(item) : &minus) {
        // draw items here
        // if index is out of item-list's bounds, draw minus
        cur_rect.origin.x -= blck_size.w + margin;
        graphics_fill_rect(ctx, cur_rect, 0, 0);
        graphics_draw_text(ctx,
                nlGetString(*item),
                font,
                grect_inset(cur_rect, GEdgeInsets(padding)),
                GTextOverflowModeWordWrap,
                GTextAlignmentCenter,
                NULL);
        index += 1;
    }
}

static bool nlShowDecimalPoint(NumberLayer *nl) {
    if (nl->identifier)
        return false;
    static NumberLayerItem point = NLPoint;
    return getListItemByVal(nl->items, &point) == NULL;
}

static bool nlShowUnderscore(NumberLayer *nl) {
    return nl->identifier;
}

static bool nlShowDigits(NumberLayer *nl) {
    return !nl->identifier || nl->items->length;
}

static bool nlShowDone(NumberLayer *nl) {
    return nl->items->length > 0;
}

static char *nlGetString(NumberLayerItem nli) {
    static char buf[2] = { 0, 0 };
    switch (nli) {
        case NLMinus:
            return "-";
        case NLPoint:
            return ".";
        case NLUnderscore:
            return "_";
        case NLDone:
            return "";  // just in case
        default:        // a digit
            buf[0] = ((nli >= NLA) ? 'a' - NLA : '0' - NLZero) + (unsigned int)nli;
            return buf;
    }
}

NumberLayer *number_layer_create(GRect frame) {
    NumberLayer *nl = (NumberLayer *)malloc(sizeof(NumberLayer));
    nl->l = layer_create_with_data(frame, sizeof(NumberLayer *));
    *(NumberLayer **)layer_get_data(nl->l) = nl;
    layer_set_update_proc(nl->l, nlUpdateProc);
    nl->identifier = false;
    nl->negative = false;
    nl->base = 10;
    nl->items = makeList(sizeof(NumberLayerItem));
    nl->done_cb = NULL;
    nl->cncl_cb = NULL;
    nl->current = NLZero;
    nl->cb_ctx = NULL;
    return nl;
}

void number_layer_destroy(NumberLayer *nl) {
    destroyList(nl->items);
    layer_destroy(nl->l);
    free(nl);
}

double number_layer_get_number(NumberLayer *nl) {
    struct list_head *items = nl->items;
    static NumberLayerItem point = NLPoint;
    unsigned int point_idx = getListItemIndex(items, &point);
    const char base = nl->base;
    if (point_idx == items->length)     // not present
        point_idx = 0;
    else if (!point_idx)                        // first item
        *(NumberLayerItem *)getListItemValue(items, point_idx) = NLZero;
    else
        listRemove(items, point_idx);
    double res = 0.;
    FOR_LIST_COUNTER(items, index, NumberLayerItem, item) {
        res += (unsigned int)(*item - NLZero) * pow_di(base, index - point_idx);
    }
    return res;
}

char *number_layer_get_string(NumberLayer *nl) {
    unsigned int len = nl->items->length;
    char *str = malloc(len + 1);
    FOR_LIST_COUNTER(nl->items, iidx, NumberLayerItem, item)
        str[len - 1 - iidx] = nlGetString(*item)[0];
    str[len] = 0;
    return str;
}

void number_layer_next(NumberLayer *nl) {
    if (nl->current == NLDone)
        return number_layer_finish(nl);
    listInsert(nl->items, 0, &(nl->current));
    if (!nl->identifier)
        nl->current = NLDone;
    layer_mark_dirty(nl->l);
}

void number_layer_prev(NumberLayer *nl) {
    if (!nl->items->length) {
        if (nl->cncl_cb != NULL)
            nl->cncl_cb(nl, nl->cb_ctx);
        else
            window_stack_remove(window_stack_get_top_window(), true);
        return;
    }
    nl->current = *(NumberLayerItem *)getListItemValue(nl->items, 0);
    listRemove(nl->items, 0);
    layer_mark_dirty(nl->l);
}

void number_layer_finish(NumberLayer *nl) {
    if (nl->done_cb != NULL)
        nl->done_cb(nl, nl->cb_ctx);
    listClear(nl->items);
    nl->current = nl->identifier ? NLA : NLZero;
    layer_mark_dirty(nl->l);
}

void number_layer_select(NumberLayer *nl, bool up) {
    /*  ordering:
     * <...>
     * One
     * Zero
     * DONE
     * point (bottom)
     * <base> - 1 (top)
     * <...>
     */
    bool s_done = nlShowDone(nl);
    bool s_point = nlShowDecimalPoint(nl);
    bool s_unsc = nlShowUnderscore(nl);
    bool s_digs = nlShowDigits(nl);
    char base = nl->base;
    NumberLayerItem max = (nl->identifier) ? NLZ : (NLZero + base - 1);
    NumberLayerItem cur = nl->current;
    if ((cur == NLZero || (cur == NLA && !s_digs)) && !up) {
        if (cur == NLA)
            nl->current = NLZero;
        if (s_done || s_point || s_unsc)
            nl->current -= 1 + !s_done + !(s_done || s_unsc);
        else
            nl->current = max;
    } else if (cur == max && up) {  // check done & point, if none, cycle
        if (s_done || s_point || s_unsc)
            nl->current = NLPoint + !s_point + !(s_unsc || s_point);
        else
            nl->current = s_digs ? NLZero : NLA;
    } else if (cur == NLPoint) {  // see max
        if (up)
            nl->current += 1 + !s_unsc + !(s_done || s_unsc);
        else
            nl->current = max;
    } else if (cur == NLDone && !up) {
        if (s_point || s_unsc)
            nl->current -= 1 + !s_unsc;
        else
            nl->current = max;
    } else if (up) {
        nl->current += 1;
    } else {
        nl->current -= 1;
    }
    layer_mark_dirty(nl->l);
}

void number_layer_set_negative(NumberLayer *nl, bool neg) { 
    nl->negative = neg;
    layer_mark_dirty(nl->l);
}

bool number_layer_get_negative(NumberLayer *nl) {
    return nl->negative;
}

void number_layer_set_base(NumberLayer *nl, char base) {
    if (base >= 2 && base < 37)
        nl->base = base;
}

bool number_layer_get_base(NumberLayer *nl) {
    return nl->base;
}

void number_layer_set_identifier(NumberLayer *nl, bool is_ident) {
    nl->identifier = is_ident;
    nl->current = is_ident ? NLA : NLZero;
    layer_mark_dirty(nl->l);
}

bool number_layer_get_identifier(NumberLayer *nl) {
    return nl->identifier;
}

Layer *number_layer_get_layer(NumberLayer *nl) {
    return nl->l;
}

void number_layer_set_callbacks(NumberLayer *nl, nlCallback doneCb, nlCallback cancelCb) {
    nl->done_cb = doneCb;
    nl->cncl_cb = cancelCb;
}

void number_layer_set_callback_context(NumberLayer *nl, void *ctx) {
    nl->cb_ctx = ctx;
}

static void nlClickConfig(void *context) {
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, nlUpClick);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, nlDownClick);
    window_single_click_subscribe(BUTTON_ID_SELECT, nlSelectClick);
    window_long_click_subscribe(BUTTON_ID_SELECT, 500, nlSelectLClick, NULL);
    window_single_click_subscribe(BUTTON_ID_BACK, nlBackClick);
}

static void nlSelectLClick(ClickRecognizerRef recognizer, void *context) {
    number_layer_finish(context);
}

static void nlUpClick(ClickRecognizerRef recognizer, void *context) {
    number_layer_select(context, true);
}

static void nlDownClick(ClickRecognizerRef recognizer, void *context) {
    number_layer_select(context, false);
}

static void nlSelectClick(ClickRecognizerRef recognizer, void *context) {
    number_layer_next(context);
}

static void nlBackClick(ClickRecognizerRef recognizer, void *context) {
    number_layer_prev(context);
}

void number_layer_set_click_config_onto_window(NumberLayer *nl, struct Window *window) {
    window_set_click_config_provider_with_context(window, nlClickConfig, nl);
}

