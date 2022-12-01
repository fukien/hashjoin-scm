// #pragma once

#ifndef RNG_H
#define RNG_H

#include "store.h"

double zeta_func(const key_t_  n, const double theta);
double eta_func(const double theta, const double zetan, const key_t_  min_key, const key_t_  max_key);

#endif /* RNG_H */