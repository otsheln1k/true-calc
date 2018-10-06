#ifndef CALC_STATE_INCLUDED
#define CALC_STATE_INCLUDED

#include "eval.h"
#include "list.h"
#include <math.h>
#include "ftoa.h"
#include <stdio.h>

#define CALC_STR_BLOCK_SIZE 4
#define CALC_STR_MAX_TAKEN_LEN 32

struct CalcState;
typedef struct CalcState CalcState;

typedef void (*CalcStateChangedCb)(CalcState *cs);

typedef void (*CalcEvalCb)(CalcState *cs, double result);

typedef enum {
    TIIBackspace, TIIPlus, TIIMinus, TIIMultiply, TIITrueDiv, TIIFloorDiv, TIIModulo, TIIPower,
    TIILParen, TIIRParen, TIIAssign, TIILFuncall, TIIComma, TIIRFuncall, TIIAddArg, TIINumber,
    TIIVar, TIIConst, TIIFunction, TIIArg, TIIUPlus, TIIUMinus, TIIReturn, TIISaveToConst,
    TIISaveToNamedConst
} TokenItemId;

typedef enum {
    TENone=0, TEValue=1, TEBinaryOperator=2, TEFuncall=4, TEFArgs=8,
    TEFComma=16, TEFRCall=32, TECloseParen=64, TEAssign=128
} TokenExp;

typedef struct {
    unsigned int nesting_level;
    unsigned int fid;
    unsigned int arg_idx;
} FuncallMark;

typedef enum {
    ITNone, ITNC, ITNumber, ITVar, ITConst, ITFunc, ITArg
} CalcInputType;

typedef struct {
    unsigned int offset;
    CalcInputType type;
    unsigned int id;
} NewNameMark;

typedef struct {
    unsigned int nesting_level;
    unsigned int offset;
    unsigned int fid;
} DefunMark;

struct CalcState {
    struct list_head *expr;             // struct list_head *of Token
    TokenExp exp;           // expectations
    unsigned int nesting; 
    char *str;
    size_t block_size;
    size_t str_len;
    CalcStateChangedCb callback;
    CalcEvalCb eval_cb;
    bool no_cb;             // dt call cb. use when adding multiple Tokens at once
    struct list_head *fcalls;           // struct list_head *of FuncallMark
    CalcInputType cit;      // for interactive use
    struct list_head *names;            // new names. for interactive use
    struct list_head *symbols;          // struct list_head *of char *. dt forget to free()
    unsigned int argnames;  // arg names: "p%d". shows how many argnames have alr been generated
    DefunMark scope;        // inside the defun. offset can not be 0
    unsigned int history_size;
};

char *getButtonText(TokenItemId tii);

char *cs_get_token_text(CalcState *cs, Token token);

CalcState *cs_create();

double cs_eval(CalcState *cs);

void cs_clear(CalcState *cs);

void cs_set_cb(CalcState *cs, CalcStateChangedCb cb, CalcEvalCb ecb);

void cs_modify_expect(CalcState *cs, Token *new_tok_p);

bool cs_show_item(CalcState *cs, TokenItemId tii);

void cs_add_item(CalcState *cs, Token new_tok);

void cs_rem_item(CalcState *cs);

CalcInputType cs_interact(CalcState *cs, TokenItemId tii);

void cs_input_number(CalcState *cs, double num);

void cs_input_id(CalcState *cs, unsigned int id);

void cs_input_newid(CalcState *cs, unsigned int id);

void cs_input_newarg(CalcState *cs);

void cs_add_symbol(CalcState *cs, char *sym);

void cs_remove_symbol(CalcState *cs, char *sym);

void cs_remove_unneeded(CalcState *cs);

CalcInputType cs_get_input_type(CalcState *cs);

void cs_destroy(CalcState *cs);

char *cs_curr_str(CalcState *cs);

unsigned int cs_get_func_id(CalcState *cs);

struct list_head *cs_get_expr(CalcState *cs);

#endif

