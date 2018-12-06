#include "list.h"
#include "eval.h"

extern struct eval_state *default_eval_state;

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
unsigned set_func_func(unsigned id, primitive_function primitive);
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
