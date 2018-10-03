#include "ftoa.h"

#define ALMOST_ZERO 0.00000001
#define ALMOST_ONE  0.99999998
// crutch master    ^
#define WRITE_IF_ENOUGH(what, where, n) { if (n == 2) { *(where++) = '>'; break; } *(where++) = what; n--; }

void ftoa(double d, char *buf, size_t num) {
    if (num < 2)
        return;
    if (d > -ALMOST_ZERO && d < ALMOST_ZERO) {
        buf[0] = '0';
        buf[1] = 0;
        return;
    } else if (d < 0) {
        d = -d;
        *(buf++) = '-';
        num--;
    }
    double intp;
    double frp = modf(d, &intp);
    int j = intp;
    unsigned int i;
    for (i = 0; (int)__pow(10, i) <= j; i++);
    for (; i; i--) {
        int p = (int)__pow(10, i - 1);
        WRITE_IF_ENOUGH('0' + j / p, buf, num)
        j = j % p;
    }
    for (i = 1; frp > ALMOST_ZERO; i++) {
        if (num < 2)
            break;
        if (i == 1)
            WRITE_IF_ENOUGH('.', buf, num)
        frp = modf(frp * 10, &intp);
        if (frp >= ALMOST_ONE) {
            frp = 0;
            intp += 1;
        }
        WRITE_IF_ENOUGH('0' + (int)intp, buf, num)
    }
    *buf = 0;
}

