#include <stdint.h>
#include <math.h>

// Saturating arithmetic intrinsics simulation
static int32_t ssdiv_int32(int32_t a, int32_t b) {
    if (b == 0) return a > 0 ? INT32_MAX : (a < 0 ? INT32_MIN : 0);
    int64_t res = (int64_t)a / (int64_t)b;
    if (res > INT32_MAX) return INT32_MAX;
    if (res < INT32_MIN) return INT32_MIN;
    return (int32_t)res;
}

static uint32_t usdiv_uint32(uint32_t a, uint32_t b) {
    if (b == 0) return a > 0 ? UINT32_MAX : 0;
    uint64_t res = (uint64_t)a / (uint64_t)b;
    if (res > UINT32_MAX) return UINT32_MAX;
    return (uint32_t)res;
}

// Helper functions with control flow
float helper1(float x, float y, int iter) {
    float result = 0.0f;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            // COPYSIGN with identical operands
            result += copysignf(x, x);
        } else if (i % 3 == 1) {
            // COPYSIGN with constant second operand
            result += copysignf(y, 2.5f);
        } else {
            // COPYSIGN with NEG as first operand
            result += copysignf(-y, x);
        }
    }
    return result;
}

float helper2(float a, float b, float c, int limit) {
    float acc = a;
    int counter = 0;
    while (counter < limit) {
        if (counter % 4 == 0) {
            // COPYSIGN with ABS as first operand
            acc += copysignf(fabsf(b), c);
        } else if (counter % 4 == 1) {
            // COPYSIGN with ABS as second operand
            acc += copysignf(a, fabsf(c));
        } else if (counter % 4 == 2) {
            // Nested COPYSIGN: first operand is COPYSIGN
            acc += copysignf(copysignf(a, b), c);
        } else {
            // Nested COPYSIGN: second operand is COPYSIGN
            acc += copysignf(a, copysignf(b, c));
        }
        ++counter;
        if (acc > 1000.0f) acc *= 0.5f;
    }
    return acc;
}

int main() {
    volatile int32_t sval = -1000;
    volatile uint32_t uval = 5000;
    volatile float f1 = -3.14f;
    volatile float f2 = 2.71f;
    volatile float f3 = -1.618f;
    
    // Trigger SS_DIV/US_DIV by constant 1
    int32_t sres = ssdiv_int32(sval, 1);
    uint32_t ures = usdiv_uint32(uval, 1);
    
    // Complex control flow with multiple COPYSIGN patterns
    float sum = 0.0f;
    for (int outer = 0; outer < 5; ++outer) {
        if (outer % 2 == 0) {
            sum += helper1(f1, f2, outer + 2);
        } else {
            sum += helper2(f1, f2, f3, outer + 1);
        }
        // Additional COPYSIGN with constant second operand (negative)
        sum += copysignf(f3, -7.0f);
        // Additional COPYSIGN with NEG as first operand
        sum += copysignf(-f1, f2);
    }
    
    // Use results to prevent elimination
    return (int)(sum + sres + ures) % 256;
}
