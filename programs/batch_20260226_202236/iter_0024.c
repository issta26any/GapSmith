#include <stdint.h>
#include <math.h>

// Saturating division intrinsics simulation
static int ssdiv(int a, int b) {
    if (b == 0) return a >= 0 ? INT32_MAX : INT32_MIN;
    if (a == INT32_MIN && b == -1) return INT32_MAX;
    return a / b;
}

static unsigned usdiv(unsigned a, unsigned b) {
    if (b == 0) return UINT32_MAX;
    return a / b;
}

// Helper functions with complex control flow
float process_copysign_chain(float base, int iter) {
    float result = base;
    for (int i = 0; i < iter; ++i) {
        if (i % 3 == 0) {
            result = copysignf(result, -result);
        } else if (i % 3 == 1) {
            result = copysignf(-result, result);
        } else {
            result = copysignf(result, 2.0f);
        }
        if (result > 100.0f) result /= 2.0f;
    }
    return result;
}

float nested_copysign_pattern(float a, float b, float c) {
    float temp = 0.0f;
    if (a != 0.0f) {
        temp = copysignf(copysignf(a, b), c);
        if (b < 0) {
            temp = copysignf(a, copysignf(b, c));
        }
    }
    return temp;
}

float abs_copysign_combinations(float x, float y) {
    float r1 = copysignf(fabsf(x), y);
    float r2 = copysignf(x, fabsf(y));
    float r3 = copysignf(-x, y);
    
    if (x > y) return r1 + r2;
    else if (x < y) return r2 - r3;
    else return r1 * r3;
}

void process_saturating_div(int *arr, unsigned *uarr, int n) {
    for (int i = 0; i < n; ++i) {
        if (i % 2 == 0) {
            arr[i] = ssdiv(arr[i], 1);
        } else {
            uarr[i] = usdiv(uarr[i], 1u);
        }
        
        if (i > 0) {
            arr[i] = ssdiv(arr[i], arr[i-1] ? arr[i-1] : 1);
            uarr[i] = usdiv(uarr[i], uarr[i-1] ? uarr[i-1] : 1u);
        }
    }
}

int main() {
    // Test data
    int int_data[10] = {100, -200, INT32_MAX, INT32_MIN, 0, 42, -73, 999, -555, 123};
    unsigned uint_data[10] = {100, 200, UINT32_MAX, 0, 42, 73, 999, 555, 123, 456};
    
    // Process saturating divisions
    process_saturating_div(int_data, uint_data, 10);
    
    // Test copysign patterns
    float f1 = 3.14f, f2 = -2.71f, f3 = 0.0f;
    
    // Identical operands
    float r1 = copysignf(f1, f1);
    float r2 = copysignf(f2, f2);
    
    // Constant second operand
    float r3 = copysignf(f1, 2.0f);
    float r4 = copysignf(f2, -3.0f);
    
    // ABS/NEG combinations
    float r5 = abs_copysign_combinations(f1, f2);
    
    // Nested copysign patterns
    float r6 = nested_copysign_pattern(f1, f2, f3);
    
    // Complex chain
    float r7 = process_copysign_chain(f1, 5);
    
    // Control flow with loops and conditions
    float sum = 0.0f;
    for (int i = 0; i < 8; ++i) {
        if (i % 4 == 0) {
            sum += copysignf((float)i, -1.0f);
        } else if (i % 4 == 1) {
            sum += copysignf(-(float)i, fabsf(sum));
        } else if (i % 4 == 2) {
            sum += copysignf(copysignf(sum, (float)i), 5.0f);
        } else {
            sum += copysignf((float)i, copysignf(sum, -2.0f));
        }
    }
    
    // Final check (prevent dead code elimination)
    volatile float check = r1 + r2 + r3 + r4 + r5 + r6 + r7 + sum;
    volatile int icheck = int_data[0] + (int)uint_data[0];
    
    return (check > 0.0f && icheck != 0) ? 0 : 1;
}
