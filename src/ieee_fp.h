#ifndef TRUNC_INCLUDED
#define TRUNC_INCLUDED

// define epsilon here
#define EPS 0.00000001

#define FP_EQ(a, b) (ieee_fabs((a) - (b)) < EPS)
#define FP_GT(a, b) ((a) >= (b) + EPS)
#define FP_LT(a, b) ((a) <= (b) - EPS)

double ieee_copysign(double x, double y);
double ieee_trunc(double x);
double ieee_fabs(double x);
double ieee_fmod(double x, double y);

#endif
