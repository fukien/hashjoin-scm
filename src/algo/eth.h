// #pragma once

#ifndef ETH_H
#define ETH_H

#include "phj.h"


void eth_pro(const datameta_t, timekeeper_t * const);
void eth_prh(const datameta_t, timekeeper_t * const);
#if TUPLE_T_SIZE <= AVX512_SIZE
void eth_prho(const datameta_t, timekeeper_t * const);
#endif /* TUPLE_T_SIZE <= AVX512_SIZE */ 


#endif /* ETH_H */