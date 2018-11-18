#ifndef CALC_STATE_INCLUDED
#define CALC_STATE_INCLUDED

#include "list.h"
#include "eval.h"

#define CALC_STR_BLOCK_SIZE 4
#define CALC_STR_MAX_TAKEN_LEN 32

struct calc_state;

typedef void (*calc_state_changed_cb)(struct calc_state *cs);
typedef void (*calc_eval_cb)(struct calc_state *cs, double result);

/* user-visible buttons */
enum token_item_id {
    TIIBackspace, TIIPlus, TIIMinus, TIIMultiply, TIITrueDiv, TIIFloorDiv, TIIModulo, TIIPower,
    TIILParen, TIIRParen, TIIAssign, TIILFuncall, TIIComma, TIIRFuncall, TIIAddArg, TIINumber,
    TIIVar, TIIConst, TIIFunction, TIIArg, TIIUPlus, TIIUMinus, TIIReturn, TIISaveToConst,
    TIISaveToNamedConst
};

/* token (or action) groups that we expect */
enum token_exp {
    TENone=0, TEValue=1, TEBinaryOperator=2, TEFuncall=4, TEFArgs=8,
    TEFComma=16, TEFRCall=32, TECloseParen=64, TEAssign=128
};

/*
 * a function is being called there,
 * for nested function calls
 */
struct funcall_mark {
    unsigned int nesting_level;
    unsigned int fid;
    unsigned int arg_idx;
};

/* input type (for menus I guess) */
enum calc_input_type {
    ITNone, ITNC, ITNumber, ITVar, ITConst, ITFunc, ITArg
};

/* a new name is being defined or what? */
struct new_name_mark {
    unsigned int offset;
    enum calc_input_type type;
    unsigned int id;
};

/* not sure how that is possible */
struct defun_mark {
    unsigned int nesting_level;
    unsigned int offset;
    unsigned int fid;
};

// new-style definition
struct calc_state {
    // token list
    struct list_head *expr;
    // expectations
    enum token_exp exp;
    unsigned int nesting; 
    char *str;
    // string buffer size
    // TODO: remove
    size_t str_sz;
    size_t str_len;
    calc_state_changed_cb callback;
    calc_eval_cb eval_cb;
    // temporarily disable the callback
    // TODO: should the eval_cb be disabled as well?
    int no_cb;
    // funcall_mark list
    struct list_head *fcalls;
    unsigned int prev_fid;
    // which menu to open and type for new ids (via c9_input_newid)
    enum calc_input_type cit;
    // new_name_mark list
    struct list_head *names;
    // char pointer (string) list
    // symbols to free() later
    // NOTE: head of this list is used by store function parameter
    // name strings
    struct list_head *symbols;
    // arg names: "p%d". shows how many argnames have already been generated
    unsigned int argnames;
    // inside the defun. offset can not be 0
    struct defun_mark scope;
    // number of saved consts
    unsigned int history_size;
};


/*
 * the following should be used carefully because
 * return garbage when not applicable
 */
#define TII_TO_OPTYPE(tii) ((tii) - TIIPlus)
#define OPTYPE_TO_TII(op) ((op) + TIIPlus)
#define TII_TO_UOPTYPE(tii) ((tii) - TIIUPlus)
#define UOPTYPE_TO_TII(uop) ((uop) + TIIUPlus)

/* doesn’t handle TIISaveToNamedConst */
#define TII_TO_CIT(tii) ((tii) - TIINumber + ITNumber)

// undefined (not applicable) for ITNone and ITNC
static inline enum token_type
CIT_TO_TOKEN_TYPE(enum calc_input_type cit)
{
    switch (cit) {
    case ITNumber:
        return Number;
    case ITVar:
        return Var;
    case ITArg:
        return Arg;
    case ITConst:
        return Const;
    case ITFunc:
        return Func;
    default:
        return -1;
    }
}


void cs_set_cb(struct calc_state *cs,
               calc_state_changed_cb cb,
               calc_eval_cb ecb);
void cs_set_nocb(struct calc_state *cs,
                 int nocb);

const char *getButtonText(enum token_item_id tii);

/* result may be statically allocated and mustn’t be mutated, accessed
 * after subsequent call to this function or free()’d
 */
const char *cs_get_token_text(struct calc_state *cs, struct token *token);

struct calc_state *cs_create();
double cs_eval(struct calc_state *cs);
void cs_clear(struct calc_state *cs);
void cs_destroy(struct calc_state *cs);

/* What kind of token/action to expect next. Buggy */
void cs_update_expect(struct calc_state *cs,
                      struct token *new_tok_p);

/* Whether to show a certain button or not. Not buggy */
int cs_show_item(struct calc_state *cs,
                 enum token_item_id tii);

void cs_add_item(struct calc_state *cs,
                 struct token new_tok);
void cs_rem_item(struct calc_state *cs);

enum calc_input_type cs_interact(struct calc_state *cs,
                                 enum token_item_id tii);

void cs_input_number(struct calc_state *cs, double num);
void cs_input_id(struct calc_state *cs, unsigned int id);
unsigned int cs_input_newid(struct calc_state *cs, char *name);
void cs_input_newarg(struct calc_state *cs);

unsigned int cs_add_const(struct calc_state *cs, double val, char *name);

char *cs_add_symbol(struct calc_state *cs, char *sym);
void cs_remove_symbol(struct calc_state *cs, char *sym);
void cs_remove_unneeded(struct calc_state *cs);

enum calc_input_type cs_get_input_type(struct calc_state *cs);
char *cs_curr_str(struct calc_state *cs);
unsigned int cs_get_func_id(struct calc_state *cs);
struct list_head *cs_get_expr(struct calc_state *cs);


#define CALC_WITH_NO_CB(cs, body)               \
    do {                                        \
        cs_set_nocb(cs, 1);                     \
        do { body } while (0);                  \
        cs_set_nocb(cs, 0);                     \
    } while (0);
#define TOP_FUNCALL(cs) ((struct funcall_mark *)(cs)->fcalls->first->data)
#define TOP_NEW_NAME(cs) ((struct new_name_mark *)(cs)->names->first->data)


#endif
