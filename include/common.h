#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

// Returns the nth bit of a (0 or 1)
#define BIT(a, n) ((a & (1 << n)) ? 1 : 0)

// Sets the nth bit of a to on (0 or 1)
#define BIT_SET(a, n, on) (on ? (a) |= (1 << n) : (a) &= ~(1 << n))

// Checks if value of a is between b and c
#define BETWEEN(a, b, c) ((a >= b) && (a <= c))

void delay(u32 ms);

#define NO_IMPL { fprint(stderr, "NOT YET IMPLEMENTED\n"); exit(-5); }

#endif /* __COMMON_H__ */
