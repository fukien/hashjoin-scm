// #pragma once

#if NUM_PASSES == 2

#ifndef PHJ_ASYMM_RADIX_H
#define PHJ_ASYMM_RADIX_H

#include "phj.h"

#ifndef NUM_RADIX_BITS_PASS1
#define NUM_RADIX_BITS_PASS1 11
#endif /* NUM_RADIX_BITS_PASS1 */

#if NUM_RADIX_BITS_PASS1 > NUM_RADIX_BITS
#undef NUM_RADIX_BITS_PASS1
#define NUM_RADIX_BITS_PASS1 NUM_RADIX_BITS
#endif /* NUM_RADIX_BITS_PASS1 > NUM_RADIX_BITS */

#define NUM_RADIX_BITS_PASS2 (NUM_RADIX_BITS - NUM_RADIX_BITS_PASS1)

#define FANOUT_PASS1_ASYMM ( 1 << NUM_RADIX_BITS_PASS1 )
#define FANOUT_PASS2_ASYMM ( 1 << (NUM_RADIX_BITS_PASS2) )


void phj_asymm_radix_bc(const datameta_t, timekeeper_t * const);
void phj_asymm_radix_sc(const datameta_t, timekeeper_t * const);
void phj_asymm_radix_hr(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_asymm_radix_hro(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 

#endif /* PHJ_ASYMM_RADIX_H */

#endif /* NUM_PASSES == 2 */