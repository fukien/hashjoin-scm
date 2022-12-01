/**
 * @file	genzipf.h
 * @version $Id: genzipf.h 3017 2012-12-07 10:56:20Z bcagri $
 *
 * Data generation with Zipf distribution.
 *
 * @author Jens Teubner <jens.teubner@inf.ethz.ch>
 *
 * (c) 2011 ETH Zurich, Systems Group
 *
 * $Id: genzipf.h 3017 2012-12-07 10:56:20Z bcagri $
 */

#ifndef GENZIPF_H
#define GENZIPF_H

#include "store.h"

typedef struct {
	int flag;
	key_t_  key;
	size_t count;
} hashmap_t;

void gen_zipf(size_t stream_size, key_t_ alphabet_size, double zipf_factor, tuple_t** output);


#endif  /* GENZIPF_H */