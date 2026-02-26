#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SAT_DIV(x, y) __ssdiv(x, y)
#define UNSAT_DIV(x, y) __usdiv(x, y)
#else
#define SAT_DIV(x, y) ((x) / (y))
#define UNSAT_DIV(x, y) ((x) / (y))
#endif

float helper1(float a, float b, int cond) {
    float result = 0.0f;
    for (int i = 0; i < 4; ++i) {
        if (cond & (1 << i)) {
            result += copysignf(a, b);
        } else {
            result -= copysignf(b, a);
        }
    }
    return result;
}

float helper2(float x, float y, int iter) {
    float acc = x;
    while (iter-- > 0) {
        if (acc > 0) {
            acc = copysignf(acc, y);
        } else {
            acc = copysignf(y, acc);
        }
        acc *= 0.9f;
    }
    return acc;
}

float helper3(float p, float q) {
    if (p == q) {
        return copysignf(p, p);
    } else if (p > 0) {
        return copysignf(-p, q);
    } else {
        return copysignf(fabsf(p), q);
    }
}

float helper4(float a, float b, float c) {
    float t1 = copysignf(copysignf(a, b), c);
    float t2 = copysignf(a, copysignf(b, c));
    if (t1 == t2) return t1;
    return t1 + t2;
}

float helper5(float x, float y) {
    return copysignf(x, 2.0f) + copysignf(y, -3.0f);
}

float helper6(float u, float v) {
    return copysignf(u, fabsf(v));
}

int main() {
    volatile int sat_var = 100;
    volatile unsigned unsat_var = 200;
    
    int sat_res = SAT_DIV(sat_var, 1);
    unsigned unsat_res = UNSAT_DIV(unsat_var, 1);
    
    float arr[8];
    for (int i = 0; i < 8; ++i) {
        arr[i] = (i - 4) * 0.5f;
    }
    
    float sum = 0.0f;
    for (int i = 0; i < 8; ++i) {
        if (i % 2 == 0) {
            sum += helper1(arr[i], arr[(i+1)%8], i);
        } else if (i % 3 == 0) {
            sum += helper2(arr[i], arr[(i+2)%8], 3);
        } else if (i % 5 == 0) {
            sum += helper3(arr[i], arr[(i+3)%8]);
        } else {
            sum += helper4(arr[i], arr[(i+4)%8], arr[(i+5)%8]);
        }
        sum += helper5(arr[i], arr[(i+6)%8]);
        sum += helper6(arr[i], arr[(i+7)%8]);
    }
    
    return (int)sum + sat_res + unsat_res;
}
