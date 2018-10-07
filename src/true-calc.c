#include <pebble.h>
#include "item_menu.h"
#include "number_layer.h"
#include "eval.h"
#include "list.h"
#include "calc_state.h"
#include "calc_lib.h"
#include "ftoa.h"

#define EXPR_DISPLAY_HEIGHT 30
#define MENU_TOP (STATUS_BAR_LAYER_HEIGHT + EXPR_DISPLAY_HEIGHT)
#define MENU_HEIGHT(total) ((total) - MENU_TOP)
#define SAVED_CONST_NAME_LEN_MAX 16
#define EXPR_LINE_TEXT_LEN_MAX 14

/* ---===TODOS===--- */

// Look for bugs, they're everywhere!

/* ---===GUI-ELEMENTS-DECLARATIONS===--- */

static Window           *main_window;
static StatusBarLayer   *statusbar_l;
static TextLayer        *expr_l;

static ItemMenuLayer    *tokenmenu_l;
static MenuLayer        *menu_l;
static NumberLayer      *number_l;

static CalcState        *calc_state;
static char             expr_str[EXPR_LINE_TEXT_LEN_MAX + 1];

void mlc_config(void *ctx);

/* ---===CONST-SAVE-LOAD===--- */

void save_const(unsigned int cid) {
    int32_t idx = 0;
    if (!persist_exists(0))
        persist_write_int(0, 1);
    else {
        idx = persist_read_int(0);
        persist_write_int(0, ++idx);
    }
    double val = get_const(cid);
    persist_write_data(2 * idx - 1, &val, sizeof(double));
    char buf[SAVED_CONST_NAME_LEN_MAX];
    strncpy(buf, get_const_name(cid), sizeof(buf));
    persist_write_string(2 * idx, buf);
}

unsigned int load_const() {
    if (!persist_exists(0))
        return 0;
    int32_t c = persist_read_int(0);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "%ld", c);
    for (int32_t i = 1; i <= c; i++) {
        if (!(persist_exists(i * 2 - 1) && persist_exists(i * 2))) {
            persist_write_int(0, 1 - i);
            return i - 1;
        }
        double val;
        persist_read_data(i * 2 - 1, &val, sizeof(double));
        char buf[SAVED_CONST_NAME_LEN_MAX];
        int nsz = persist_read_string(i * 2, buf, sizeof(buf));
        char *n = malloc(nsz);
        strcpy(n, buf);
        cs_add_symbol(calc_state, n);
        add_const(val, n);
    }
    return c;
}

void clear_const() {
    if (!persist_exists(0))
        return;
    int32_t c = persist_read_int(0);
    for (int32_t i = 0; i < c; i++) {
        persist_delete(i * 2 + 1);
        persist_delete((i + 1) * 2);
    }
    persist_write_int(0, 0);
}

static double predef_savec(struct list_head *args) {
    if (!args->length)
        return -1.;
    Token *tokp = GET_PTOKEN(LIST_REF(struct list_head *, args, 0), 0);
    if (tokp->type != Const)
        return -1.;
    save_const(tokp->value.id);
    return 0;
}

static double predef_loadc(struct list_head *args) {
    return (double)load_const();
}

static double predef_clearc(struct list_head *args) {
    clear_const();
    return 0;
}

void init_constlib() {
    FUNCDEF_ARGS("save_cst", 1, { "cst" }, predef_savec);
    FUNCDEF("load_cst", makeList(sizeof(char *)), predef_loadc);
    FUNCDEF("clear_cst", makeList(sizeof(char *)), predef_clearc);
}

/* ---===VARIOUS-FUNCS===--- */

char *alloc_string(char *str) {
    char *s1 = malloc(strlen(str));
    strcpy(s1, str);
    return s1;
}

TokenItemId getTokenItemId(unsigned int tm_selection) {
    tm_selection += 1;
    for (TokenItemId t = TIIBackspace;; t++) {
        tm_selection -= cs_show_item(calc_state, t);
        if (!tm_selection)
            return t;
    }
}

char *getOperatorName(unsigned int idx) {
    return getButtonText(getTokenItemId(idx));  // getButtonText from cs
}

void cs_callback(CalcState *cs) {
    char *s = cs_curr_str(cs);
    size_t slen = strlen(s);
    if (slen <= EXPR_LINE_TEXT_LEN_MAX)
        text_layer_set_text(expr_l, cs_curr_str(cs));
    else {
        expr_str[0] = '<';
        strncpy(expr_str + 1, s + slen - EXPR_LINE_TEXT_LEN_MAX, EXPR_LINE_TEXT_LEN_MAX);
        text_layer_set_text(expr_l, expr_str);
    }
    // text_layer_set_text(expr_l, s);  // TODO truncate
}

void cs_eval_cb(CalcState *cs, double result) {
    strcpy(expr_str, "->");
    ftoa(result, expr_str + 2, sizeof(expr_str) - 2);
    text_layer_set_text(expr_l, expr_str);
    cs_clear(cs);
}

/* ---===TOKEN-MENU-CALLBACKS===--- */

unsigned int tm_count_cb(ItemMenuLayer *tm, void *ctx) {
    /*
     * BS
     * + - * / // % **
     * =
     * arg+
     * func num var const +x -x (
     * )
     * arg
     * f,
     * f)
     * f(
     * RET SAV
     */
    return cs_show_item(calc_state, TIIBackspace)
         + cs_show_item(calc_state, TIIPlus) * 7
         + cs_show_item(calc_state, TIIAssign)
         + cs_show_item(calc_state, TIIAddArg)
         + cs_show_item(calc_state, TIINumber) * 6
         + cs_show_item(calc_state, TIIConst)
         + cs_show_item(calc_state, TIIRFuncall)
         + cs_show_item(calc_state, TIIArg)
         + cs_show_item(calc_state, TIIComma)
         + cs_show_item(calc_state, TIILFuncall)
         + cs_show_item(calc_state, TIIRParen)
         + cs_show_item(calc_state, TIIReturn) * 3;
}

char *tm_title_cb(ItemMenuLayer *tm, unsigned int idx, void *ctx) {
    return getOperatorName(idx);
}

void tm_select_cb(ItemMenuLayer *tm, unsigned int idx, void *ctx) {
    // check whether the items can be added immediately;
    // if a Menu is needed, show it, its callback will write
    // everything to the CS.
    // if a NumberChooser is needed, idem
    item_menu_set_current_index(tokenmenu_l, 0);
    switch (cs_interact(calc_state, getTokenItemId(idx))) {
        case ITNumber:
            number_layer_set_identifier(number_l, false);
            number_layer_set_click_config_onto_window(number_l, main_window);
            layer_set_hidden(number_layer_get_layer(number_l), false);
            layer_set_hidden(item_menu_get_layer(tokenmenu_l), true);
            break;
        case ITVar: case ITConst: case ITFunc: case ITArg:
            window_set_click_config_provider(main_window, mlc_config);
            layer_set_hidden(item_menu_get_layer(tokenmenu_l), true);
            menu_layer_reload_data(menu_l);
            menu_layer_set_selected_index(menu_l,
                    MenuIndex(0, 0), MenuRowAlignCenter, false);
            layer_set_hidden(menu_layer_get_layer(menu_l), false);
            break;
        case ITNC:
            number_layer_set_click_config_onto_window(number_l, main_window);
            number_layer_set_identifier(number_l, true);
            layer_set_hidden(number_layer_get_layer(number_l), false);
            layer_set_hidden(item_menu_get_layer(tokenmenu_l), true);
        case ITNone:
            break;
    }
}

/* ---===NUMBER-LAYER-CLICKS===--- */

void nlc_select(NumberLayer *nl, void *ctx) {
    CalcInputType cit = cs_get_input_type(calc_state);
    switch (cit) {
        case ITNumber:  // value chooser
            cs_input_number(calc_state, number_layer_get_number(nl));
            break;
        case ITVar:  // name chooser for the new var/func
        case ITFunc:
            {
                char *name = alloc_string(number_layer_get_string(nl));
                // cs_add_symbol(calc_state, name);
                cs_input_newid(calc_state, (cit == ITVar) ? add_var(0, name) : add_func(name));
            }
            break;
        case ITNC:
            cs_input_newid(calc_state, add_const(
                        cs_eval(calc_state),
                        alloc_string(number_layer_get_string(nl))));
            break;
        default:
            return;
    }
    // switch back
    item_menu_set_click_config_onto_window(tokenmenu_l, main_window);
    layer_set_hidden(item_menu_get_layer(tokenmenu_l), false);
    layer_set_hidden(number_layer_get_layer(nl), true);
}

void nlc_back(NumberLayer *nl, void *ctx) {
    CalcInputType cit = cs_get_input_type(calc_state);
    if (cit == ITNumber) {
        item_menu_set_click_config_onto_window(tokenmenu_l, main_window);
        layer_set_hidden(item_menu_get_layer(tokenmenu_l), false);
    } else {
        window_set_click_config_provider(main_window, mlc_config);
        layer_set_hidden(menu_layer_get_layer(menu_l), false);
    }
    layer_set_hidden(number_layer_get_layer(nl), true);
}

/* ---===MENU-LAYER-CALLBACKS===--- */

uint16_t ml_get_num_rows(MenuLayer *ml, uint16_t idx, void *ctx) {
    CalcInputType cit = cs_get_input_type(calc_state);
    return (cit == ITVar || cit == ITFunc) + 
        ((cit == ITVar) ? count_var() :
        (cit == ITConst) ? count_const() :
        (cit == ITFunc) ? count_func() :
        get_func_argc(cs_get_func_id(calc_state)));
}

void ml_draw_row(GContext *gctx, const Layer *cell_layer, MenuIndex *cell_index, void *ctx) {
    const CalcInputType cit = cs_get_input_type(calc_state);
    uint16_t row = cell_index->row;
    menu_cell_basic_draw(gctx, cell_layer,
            (((cit == ITVar || cit == ITFunc) && !row) ? "+" :
             (cit == ITVar) ? get_var_name(row - 1) :
             (cit == ITFunc) ? get_func_name(row - 1) :
             (cit == ITConst) ? get_const_name(row) :
             get_func_arg_name(cs_get_func_id(calc_state), row)),
             NULL, NULL);
}

/* ---===MENU-LAYER-CLICKS===--- */

void mlc_down(ClickRecognizerRef rec, void *ctx) {
    menu_layer_set_selected_next(menu_l, false, MenuRowAlignCenter, true);
}

void mlc_up(ClickRecognizerRef rec, void *ctx) {
    menu_layer_set_selected_next(menu_l, true, MenuRowAlignCenter, true);
}

void mlc_back(ClickRecognizerRef rec, void *ctx) {
    item_menu_set_click_config_onto_window(tokenmenu_l, main_window);
    layer_set_hidden(item_menu_get_layer(tokenmenu_l), false);
    layer_set_hidden(menu_layer_get_layer(menu_l), true);
}

void mlc_select(ClickRecognizerRef rec, void *ctx) {
    // check whether the [+] item is selected, if so, do smth else
    CalcInputType cit = cs_get_input_type(calc_state);
    if (!menu_layer_get_selected_index(menu_l).row && (cit == ITVar || cit == ITFunc)) {
        number_layer_set_identifier(number_l, true);
        number_layer_set_click_config_onto_window(number_l, main_window);
        layer_set_hidden(number_layer_get_layer(number_l), false);
    } else {
        cs_input_id(calc_state, menu_layer_get_selected_index(menu_l).row - (cit == ITVar || cit == ITFunc));
        item_menu_set_click_config_onto_window(tokenmenu_l, main_window);
        layer_set_hidden(item_menu_get_layer(tokenmenu_l), false);
    }
    layer_set_hidden(menu_layer_get_layer(menu_l), true);
}

void mlc_config(void *ctx) {  // CLICK-CONFIG
    window_single_repeating_click_subscribe(BUTTON_ID_UP, 100, mlc_up);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 100, mlc_down);
    window_single_click_subscribe(BUTTON_ID_BACK, mlc_back);
    window_single_click_subscribe(BUTTON_ID_SELECT, mlc_select);
}

/* ---===MAIN-WINDOW===--- */

void mw_load(Window *wnd) {
    calc_state = cs_create();
    cs_set_cb(calc_state, cs_callback, cs_eval_cb);

    init_calc();  // initializes the data lists
    cl_lib_init();
    init_constlib();

    Layer *root_layer = window_get_root_layer(wnd);
    GSize size = layer_get_bounds(root_layer).size;

    statusbar_l = status_bar_layer_create();
    status_bar_layer_set_colors(statusbar_l, GColorBlack, GColorWhite);
    layer_add_child(root_layer, status_bar_layer_get_layer(statusbar_l));

    expr_l = text_layer_create(GRect(0, STATUS_BAR_LAYER_HEIGHT, size.w, EXPR_DISPLAY_HEIGHT));
    text_layer_set_font(expr_l, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_overflow_mode(expr_l, GTextOverflowModeFill);
    text_layer_set_text_alignment(expr_l, GTextAlignmentRight);
    text_layer_set_background_color(expr_l, GColorBlack);
    text_layer_set_text_color(expr_l, GColorWhite);
    layer_add_child(root_layer, text_layer_get_layer(expr_l));

    GRect rect = GRect(0, MENU_TOP, size.w, MENU_HEIGHT(size.h));
    tokenmenu_l = item_menu_create(rect);
    item_menu_set_font(tokenmenu_l, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    item_menu_set_callbacks(tokenmenu_l, tm_count_cb, tm_title_cb, tm_select_cb, NULL, NULL);
    layer_add_child(root_layer, item_menu_get_layer(tokenmenu_l));
    item_menu_set_click_config_onto_window(tokenmenu_l, wnd);

    menu_l = menu_layer_create(rect);
    menu_layer_set_callbacks(menu_l, NULL, (MenuLayerCallbacks){
            .get_num_rows = ml_get_num_rows,
            .draw_row = ml_draw_row });
    menu_layer_set_normal_colors(menu_l, GColorBlack, GColorWhite);
    menu_layer_set_highlight_colors(menu_l, GColorWhite, GColorBlack);
    layer_set_hidden(menu_layer_get_layer(menu_l), true);
    layer_add_child(root_layer, menu_layer_get_layer(menu_l));

    number_l = number_layer_create(rect);
    number_layer_set_callbacks(number_l, nlc_select, nlc_back);
    layer_set_hidden(number_layer_get_layer(number_l), true);
    layer_add_child(root_layer, number_layer_get_layer(number_l));
}

void mw_unload(Window *wnd) {
    status_bar_layer_destroy(statusbar_l);
    text_layer_destroy(expr_l);
    item_menu_destroy(tokenmenu_l);
    menu_layer_destroy(menu_l);
    number_layer_destroy(number_l);
    cs_destroy(calc_state);
    destroy_calc();
}

void init_app() {
    main_window = window_create();
    window_set_window_handlers(main_window, (WindowHandlers){
            .load = mw_load, .unload = mw_unload });
    window_stack_push(main_window, true);
}

void destroy_app() {
    window_destroy(main_window);
}

int main() {
    init_app();
    app_event_loop();
    destroy_app();
}

