#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "ieee_fp.h"
#include "ftoa.h"

#define MAX_EXPT 7
#define MIN_DIGITS 3

#define ABS_V(v) ((v >= 0) ? v : -v)

#define DIGIT_CHAR(x) ('0' + (x))
#define TOO_SMALL_CHAR '<'
#define TOO_HUGE_CHAR '>'
#define EXPT_CHAR 'e'

static int get_digit(double *d_rw, double p10,
                     int add_round_next_p)
{
    double d = *d_rw, head, next;
    
    head = trunc(d / p10);
    d -= head * p10;
    next = d / p10;
    
    if (FP_EQ(next, 1.0)) {
        head += 1.0;
        next = 0.0;
        d -= p10;
    }

    if (add_round_next_p)
        head += round(next);

    *d_rw = d;

    return (int)head;
}

static void write_nan(double d, char *buf, size_t num)
{
    strncpy(buf, "NaN", num);
}

static void write_inf(int sign, char *buf, size_t num)
{
    if (num <= 4 && sign > 0) {
        strncpy(buf, "inf", num);
    } else {
        buf[0] = (sign > 0) ? '+' : '-';
        strncpy(&buf[1], "inf", num - 1);
    }
}

void ftoa(double d, char *buf, size_t num)
{
    // we may want to write d in scientific notation:
    // exponent is at most 3 digits (2^128 ≈ 10^38)
    int expt = 0;
    double p10 = 1.0;
    
    int is_negative = FP_LT(d, 0.0);
    int is_expt_0;
    int is_expt_g10;
    int is_expt_neg;
    
    size_t expt_width;
    size_t normal_width;
    size_t zero_count;
    size_t chars = num - 1;
    
    char *pos = buf;

    if (num < 2) return;

    switch (ieee_fpclassify(d)) {
    case FP_NAN:
        return write_nan(d, buf, num);
    case FP_INFINITE:
        return write_inf((d > 0) ? 1 : -1, buf, num);
    default:
        break;
    }

    if (FP_EQ(d, 0.0)) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    }

    if (is_negative) d = -d;

    // find the right power of 10 to begin with
    //      0.000abcdef
    //           ^--(p10)
    //      0.0001
    // abcdef.ghi
    // ^-------(p10)
    // 100000.0
    while (p10 > d) {
        p10 /= 10.0;
        expt -= 1;
    }
    while (p10 * 10 <= d) {
        p10 *= 10.0;
        expt += 1;
    }

    is_expt_0 = (expt == 0);
    is_expt_g10 = (ABS_V(expt) >= 10);
    is_expt_neg = (expt < 0);

    // max expt_width is 4
    expt_width = !is_expt_0 ? 2 + is_expt_neg + is_expt_g10 : 0;

    // 0.000abcdef
    //  <___>--(3)
    zero_count = is_expt_neg ? -expt - 1 : 0;

    //  abcdef.ghi
    // <______>--(6)
    //  .000abcd
    // <_____>--(5)
    normal_width = is_expt_neg
        ? (zero_count + 1)
        : (size_t)(expt + 1);

    // so what? write in expt mode:
    // - if we’ve got a too large (or too small) exponent
    // - if integral part doesn’t fit
    // - if |x| < 1 and less than 3 meaningful digits will be available
    // .000xxx
    // x.xxe-4

    if (is_negative) {
        *(pos++) = '-';
        chars -= 1;
    }

    if (!is_expt_0
        && (ABS_V(expt) >= MAX_EXPT
            || normal_width > chars
            || (is_expt_neg
                && chars >= 1 + MIN_DIGITS + expt_width
                && chars < normal_width + MIN_DIGITS))) {
        // expt mode

        int idx = 0;

        do {
            if (idx == 1) {
                *(pos++) = '.';
                chars -= 1;
                idx = 2;
            } else if (idx == 0) {
                idx = 1;
            }
            *(pos++) =
                DIGIT_CHAR(get_digit(&d, p10, (--chars == expt_width)));
            p10 /= 10.0;
        } while (chars > expt_width + (idx == 1) && !FP_EQ(d, 0.0));
        *(pos++) = EXPT_CHAR;
        if (is_expt_neg) {
            *(pos++) = '-';
            expt = -expt;
        }
        if (is_expt_g10)
            *(pos++) = DIGIT_CHAR(expt / 10);
        *(pos++) = DIGIT_CHAR(expt % 10);
    } else {
        // normal mode

        // write integral part
        int last_digit_p = 0;

        while (expt-- >= 0) {
            // indicate that we can’t write even the integral part
            if (chars == 1) {
                if (expt >= 0) {
                    *(pos++) = TOO_HUGE_CHAR;
                    break;
                } else {
                    last_digit_p = 1;
                }
            }
            *(pos++) = DIGIT_CHAR(get_digit(&d, p10, last_digit_p));
            chars -= 1;
            p10 /= 10.0;
        }
        // fractional part may be zero
        // also, don’t write zeroes unless we can add at least one
        // meaningful digit
        if (!FP_EQ(d, 0.0) && chars > 1 + zero_count) {
            // reduce remaining space beforehand
            chars -= 1 + zero_count;
            // decimal point is required (it)
            *(pos++) = '.';
            // if the number had negative exponent at the beginning
            while (zero_count--) *(pos++) = '0';
            while (chars && !FP_EQ(d, 0.0)) {
                *(pos++) = DIGIT_CHAR(get_digit(&d, p10, !--chars));
                p10 /= 10.0;
            }
        } else if (is_expt_neg && chars) {
            // can’t display the number in this buffer
            *(pos++) = TOO_SMALL_CHAR;
        }
    }
    *(pos++) = 0;
}
