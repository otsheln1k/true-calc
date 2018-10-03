#ifndef EVAL_INCLUDED
#define EVAL_INCLUDED

#include <stdbool.h>
#include "array.h"
#ifdef __PEBBLE__
#include "SmallMaths.h"
#define __pow sm_pow
#else
#include <math.h>
#define __pow pow
#endif

/*---------------TYPES---------------*/

typedef double (*PredefFunc)(array args);

typedef enum {
    Var, Const, Func, Number, Operator, UOperator, Arg
} TokenType;  // Arg is only used in a function body

typedef enum {
    OAdd, OSub, OMul, OTDiv, OFDiv, OMod, OPow, OLp, ORp, OAssign, OLCall, OComma, ORCall
} OperatorType;

typedef enum {
    UPlus, UMinus
} UOperatorType;

typedef union {
    double number;
    unsigned int id;
    OperatorType op;
    UOperatorType uop;
} TokenValue;

typedef struct {
    TokenType type;
    TokenValue value;
} Token;

typedef union {
    array tokens;  // array of Tokens. may contain ones with .type == Arg
    PredefFunc f;  // function pointer. args - array of array of Token
} FunctionBody;

typedef struct {
    char *name;
    array args;     // array of char* - arg names
    bool predef;    // whether the funcs body is an array of
                    // Tokens(false, 0) or a c-function(true, 1)
    FunctionBody body;
} DefinedFunction;  // callable function

typedef struct {
    char *name;
    double value;
} NamedValue;

typedef struct {
    unsigned int id;
    bool is_func;
} LValue;

/*---------------MACROS--------------*/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define GET_TOKEN(arr,idx) (*(Token *)getArrayItemValue(arr, idx))

#define GET_PTOKEN(arr,idx) ((Token *)getArrayItemValue(arr, idx))

#define GET_DOUBLE(arr, idx) (*(double *)getArrayItemValue(arr, idx))

#define PREDEF_ONELINE_FUNC(name, args_name, argv_name, expr) \
    static double name(array args_name) { \
        array argv_name = eval_all_args(args_name); \
        double res = expr; \
        destroyArray(argv_name); \
        return res; \
    }

#define FUNCDEF(fname, fargs, fpredef) \
        set_func_func(set_func_args(add_func(fname), fargs), fpredef)

#define FUNCDEF_ARGS(fname, fargc, fargs, fpredef) \
        FUNCDEF(fname, ARRAY_CONV(char *, fargc, fargs), fpredef)


/*---------------FUNCS---------------*/

void arrayAppendToken(array arr, Token token);

void init_calc();

void destroy_calc();

double eval_atom(Token);

unsigned int count_var();
unsigned int count_const();
unsigned int count_func();

unsigned int add_var(double, char *);

double get_var(unsigned int);

char *get_var_name(unsigned int);

void set_var(unsigned int, double);

void remove_var(unsigned int);

unsigned int add_const(double, char *);

double get_const(unsigned int);

char *get_const_name(unsigned int);

void set_const(unsigned int, double);

void remove_const(unsigned int);

unsigned int add_func(char *);

unsigned int set_func_args(unsigned int, array);  // args: array of char*

unsigned int set_func_body(unsigned int, array);  // body: array of Token

unsigned int set_func_func(unsigned int fid, PredefFunc f);

array get_func_args(unsigned int);

unsigned int get_func_argc(unsigned int);

char *get_func_arg_name(unsigned int, unsigned int);

char *get_func_name(unsigned int);

array eval_all_args(array);

double call_func(unsigned int, array);  // args: array of array of Token

void remove_func(unsigned int);

bool is_predefined_func(unsigned int);

double eval_angle(double);  // in radians

double eval_operation(OperatorType, double, double);

double eval_uoperation(UOperatorType, double);

LValue eval_lvalue(array);

LValue eval_assign(LValue, array);  // rvalue: array of Token

double eval_funcall(unsigned int, array);  // args: array of array of Token

double eval_expr(array, int);

/*----------------END----------------*/

#endif

