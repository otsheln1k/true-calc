/* -*- mode: c -*- */

#ifndef EVAL_INCLUDED
#define EVAL_INCLUDED

#include <stdbool.h>
#include "list.h"


/* TYPES (with COMPATIBILITY TYPEDEFS) */

typedef double (*PredefFunc)(struct list_head *args);

enum token_type {
    Var, Const, Func, Number, Operator, UOperator, Arg
};  // Arg is only used in a function body

typedef enum operator_type {
    OAdd, OSub,
    OMul, OTDiv, OFDiv, OMod,
    OPow,
    OLp, ORp,
    OAssign,
    OLCall, OComma, ORCall,
    O_Count
} OperatorType;

struct operator_props {
    int order, nesting;
};

typedef enum uoperator_type {
    UPlus, UMinus
} UOperatorType;

typedef union token_value {
    double number;
    unsigned int id;
    enum operator_type op;
    enum uoperator_type uop;
} TokenValue;

typedef struct token {
    enum token_type type;
    union token_value value;
} Token;

typedef union function_body {
    struct list_head *tokens;  // list of Tokens. may contain ones with .type == Arg
    PredefFunc f;  // function pointer. args - list of list of Token
} FunctionBody;

typedef struct function {
    char *name;
    struct list_head *args;     // list of char* - arg names
    bool predef;    // whether the funcs body is an list of
                    // Tokens(false, 0) or a c-function(true, 1)
    union function_body body;
} DefinedFunction;  // callable function

typedef struct named_value {
    char *name;
    double value;
} NamedValue;

typedef struct lvalue {
    unsigned int id;
    bool is_func;
} LValue;

struct eval_state {
    struct list_head *vars;
    struct list_head *consts;
    struct list_head *funcs;
};


/* NEW INTERFACE (some symbols will be renamed) */

void init_eval_state(struct eval_state *e);
void destroy_eval_state(struct eval_state *e);

double eval_atom_es(struct eval_state *e, struct token *atom);
struct number_value *add_number_value_es(struct list_head *l,
                                         double init_val,
                                         char *name,
                                         int head_p);
struct function *add_func_es(struct eval_state *e,
                             char *name,
                             int head_p);
void remove_func_es(struct eval_state *e, unsigned int fid);
struct list_head *eval_all_args_es(struct eval_state *e,
                                   struct list_head *args);
struct list_head *eval_arglist_es(struct eval_state *e,
                                  struct list_head *tokens);
double call_func_es(struct eval_state *e,
                    unsigned int fid,
                    struct list_head *argv);
double eval_operation(enum operator_type operation,
                      double op1, double op2);
double eval_uoperation(enum uoperator_type uoperation,
                       double op);
void eval_lvalue_es(struct list_head *tokens,
                    struct lvalue *ret_lvalue);
const struct operator_props *token_props(struct token *tok);
double eval_assignment_es(struct eval_state *e,
                          struct lvalue *lvalue,
                          struct list_head *rvalue);
double eval_expr_es(struct eval_state *e,
                    struct list_head *tokens);


/* OLD INTERFACE */

// MACROS

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GET_TOKEN(arr,idx) LIST_REF(Token, arr, idx)

#define GET_PTOKEN(arr,idx) LIST_DATA(Token, arr, idx)

#define GET_DOUBLE(arr, idx) LIST_REF(double, arr, idx)

#define PREDEF_ONELINE_FUNC(name, args_name, argv_name, expr) \
    static double name(struct list_head *args_name) { \
        struct list_head *argv_name = eval_all_args(args_name); \
        double res = expr; \
        destroyList(argv_name); \
        return res; \
    }

#define FUNCDEF(fname, fargs, fpredef) \
        set_func_func(set_func_args(add_func(fname), fargs), fpredef)

#define FUNCDEF_ARGS(fname, fargc, fargs, fpredef) \
        FUNCDEF(fname, LIST_CONV(char *, fargc, fargs), fpredef)

// FUNCTIONS

void init_calc(void);
void destroy_calc(void);

double eval_atom(struct token t);

unsigned count_var(void);
unsigned add_var(double val, char *name);
double get_var(unsigned id);
char *get_var_name(unsigned id);
void set_var(unsigned id, double val);
void remove_var(unsigned id);

unsigned count_const(void);
unsigned add_const(double val, char *name);
double get_const(unsigned id);
char *get_const_name(unsigned id);
void set_const(unsigned id, double val);
void remove_const(unsigned id);

unsigned count_func(void);
unsigned add_func(char *name);
unsigned set_func_args(unsigned id, struct list_head *args);
unsigned set_func_body(unsigned id, struct list_head *body);
unsigned set_func_func(unsigned id, PredefFunc primitive);
void remove_func(unsigned id);

struct list_head *get_func_args(unsigned id);
unsigned get_func_argc(unsigned id);
char *get_func_arg_name(unsigned id, unsigned arg);
char *get_func_name(unsigned id);

struct list_head *eval_all_args(struct list_head *args);
double call_func(unsigned fid, struct list_head *args);
struct lvalue eval_lvalue(struct list_head *tokens);
struct lvalue eval_assign(struct lvalue lvalue,
                          struct list_head *tokens);
double eval_expr(struct list_head *tokens, int op_order);

#endif
