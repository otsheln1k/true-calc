#ifndef CALC_LIB_INCLUDED
#define CALC_LIB_INCLUDED

#include "eval.h"

/*
 * Loads the standard functions&const:
 * lib_load(), which loads the rest
 * sin/cos/tan + arc-, -d
 * sqrt(x)
 * const pi, deg2rad, rad2deg
 * const_load(), const_save(c)
 */
void init_triglib(struct eval_state *e);

/*
 * Loads:
 * - save_cst
 * - load_cst
 * - clear_cst
 */
void init_constlib(struct eval_state *e);

#endif
