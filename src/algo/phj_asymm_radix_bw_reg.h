// #pragma once

#ifdef BW_REG
#if NUM_PASSES == 2

#ifndef PHJ_ASYMM_RADIX_BW_REG_H
#define PHJ_ASYMM_RADIX_BW_REG_H

#include "phj_asymm_radix.h"
#include "phj_bw_reg.h"


void phj_asymm_radix_bw_reg_bc(const datameta_t, timekeeper_t * const);
void phj_asymm_radix_bw_reg_sc(const datameta_t, timekeeper_t * const);
void phj_asymm_radix_bw_reg_hr(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void phj_asymm_radix_bw_reg_hro(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 

#endif /* PHJ_ASYMM_RADIX_BW_REG_H */

#endif /* NUM_PASSES == 2 */
#endif /* BW_REG */