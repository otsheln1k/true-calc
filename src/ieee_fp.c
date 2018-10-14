#include "ieee_fp.h"

#ifdef __PEBBLE__
#ifndef __SMALL_BITFIELDS
#define __SMALL_BITFIELDS
#endif
#include <ieeefp.h>

double ieee_copysign(double x, double y)
{
    __ieee_double_shape_type shape_x, shape_y;
    shape_x.value = x;
    shape_y.value = y;
    shape_x.number.sign = shape_y.number.sign;
    return shape_x.value;
}

double ieee_trunc(double x)
{
    __ieee_double_shape_type shape;
    shape.value = x;

    int exp = shape.number.exponent - 1023;

    // no bits in integral part
    if (exp < 0)
        return ieee_copysign(0.0, x);

    // all bits form the integral part
    if (exp > 52)
        return x;

#define NBITS(n) ((1 << (n)) - 1)

    if (exp < 4) {
        shape.number.fraction0 &= ~NBITS(4 - exp);
        goto clear_3;
    } else if (exp < 20) {
        shape.number.fraction1 &= ~NBITS(20 - exp);
        goto clear_2;
    } else if (exp < 36) {
        shape.number.fraction2 &= ~NBITS(36 - exp);
        goto clear_1;
    } else {
        shape.number.fraction3 &= ~NBITS(52 - exp);
        goto clear_0;
    }
    
 clear_3: shape.number.fraction1 = 0;
 clear_2: shape.number.fraction2 = 0;
 clear_1: shape.number.fraction3 = 0;
 clear_0: return shape.value;
}

#else
#include <ieee754.h>

double ieee_copysign(double x, double y)
{
    union ieee754_double dx, dy;
    dx.d = x;
    dy.d = y;
    dx.ieee.negative = dy.ieee.negative;
    return dx.d;
}

double ieee_trunc(double x)
{
    union ieee754_double d;
    d.d = x;

    unsigned int exp = d.ieee.exponent;
    if (exp < 1023)
        return ieee_copysign(0.0, x);

    if (exp >= 1023 + 53)
        return x;

    int exp_s = exp - 1023;

    // mantissa0:20
    // mantissa1:32
    if (exp_s < 20) {
        d.ieee.mantissa0 &= ~((1 << (20 - exp_s)) - 1);
        d.ieee.mantissa1 = 0;
    } else {
        d.ieee.mantissa1 &= ~((1 << (52 - exp_s)) - 1);
    }

    return d.d;
}

#endif


double ieee_fabs(double x)
{
    return ieee_copysign(x, 1.0);
}


double ieee_fmod(double x, double y)
{
    double k = x / y;
    double tk = ieee_trunc(k);
    
    if (ieee_fabs(k - tk) >= 1 - EPS) {
        return ieee_copysign(0.0, x);
    }

    return x - ieee_copysign(tk * y, x);
}
