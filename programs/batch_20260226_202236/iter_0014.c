#include <stdint.h>
#include <math.h>

#ifdef __ARM_FEATURE_SAT
#define SS_DIV(x, y) __ssdiv(x, y)
#define US_DIV(x, y) __usdiv(x, y)
#else
static int32_t SS_DIV(int32_t x, int32_t y) {
    if (y == 0) return 0;
    int64_t res = (int64_t)x / y;
    if (res > INT32_MAX) return INT32_MAX;
    if (res < INT32_MIN) return INT32_MIN;
    return (int32_t)res;
}
static uint32_t US_DIV(uint32_t x, uint32_t y) {
    if (y == 0) return 0;
    uint64_t res = (uint64_t)x / y;
    if (res > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)res;
}
#endif

float helper1(float a, float b, int iter) {
    float result = 0.0f;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result += copysignf(a, a); 
        } else if (i % 3 == 1) {
            result += copysignf(b, 2.0f); 
        } else {
            result += copysignf(-a, b); 
        }
    }
    return result;
}

float helper2(float x, float y, float z, int limit) {
    float acc = x;
    int count = 0;
    while (count < limit) {
        if (acc > 0.0f) {
            acc += copysignf(fabsf(y), z); 
        } else {
            acc += copysignf(x, fabsf(z)); 
        }
        if (count % 2 == 0) {
            acc += copysignf(copysignf(acc, y), z); 
        } else {
            acc += copysignf(x, copysignf(acc, z)); 
        }
        ++count;
    }
    return acc;
}

int32_t sat_div_variants(int32_t val, unsigned n) {
    int32_t sum = 0;
    for (unsigned i = 0; i < n; ++i) {
        if (i % 4 == 0) {
            sum += SS_DIV(val + i, 1); 
        } else if (i % 4 == 1) {
            sum += SS_DIV(val - i, 1); 
        } else if (i % 4 == 2) {
            sum += (int32_t)US_DIV((uint32_t)(val + i), 1U); 
        } else {
            sum += (int32_t)US_DIV((uint32_t)(val - i), 1U); 
        }
    }
    return sum;
}

float copysign_nesting(float a, float b, float c, int depth) {
    if (depth <= 0) return a;
    float t = copysignf(copysignf(a, b), c); 
    float u = copysignf(a, copysignf(b, c)); 
    if (t > u) {
        return copysign_nesting(t, u, a, depth - 1);
    } else {
        return copysign_nesting(u, t, b, depth - 1);
    }
}

int main() {
    volatile int32_t base = 1000;
    volatile float f1 = 3.14f;
    volatile float f2 = -2.71f;
    volatile float f3 = 0.0f;
    
    int32_t sat_result = sat_div_variants(base, 8);
    float cs_result1 = helper1(f1, f2, 5);
    float cs_result2 = helper2(f1, f2, f3, 4);
    float cs_result3 = copysign_nesting(f1, f2, f3, 3);
    
    float final_sum = cs_result1 + cs_result2 + cs_result3 + (float)sat_result;
    
    return (final_sum > 0.0f) ? 0 : 1;
}
