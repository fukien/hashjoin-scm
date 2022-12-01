/**
 * @file
 *
 * Generate Zipf-distributed input data.
 *
 * @author Jens Teubner <jens.teubner@inf.ethz.ch>
 *
 * (c) ETH Zurich, Systems Group
 *
 * $Id: genzipf.c 3017 2012-12-07 10:56:20Z bcagri $
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "eth_genzipf.h"


/**
 * Create an alphabet, an array of size @a size with randomly
 * permuted values 0..size-1.
 *
 * @param size alphabet size
 * @return an <code>item_t</code> array with @a size elements;
 *		 contains values 0..size-1 in a random permutation; the
 *		 return value is malloc'ed, don't forget to free it afterward.
 */
key_t_ * gen_alphabet(size_t size) {
	key_t_  *alphabet;

	/* allocate */
	alphabet = (key_t_ *) malloc(size * sizeof (*alphabet));
	assert(alphabet);

	/* populate */
	for (size_t idx = 0; idx < size; idx ++) {
		alphabet[idx] = idx + 1;   /* don't let 0 be in the alphabet */
	}

	/* permute */
	for (size_t idx = size - 1; idx > 0; idx --) {
		size_t kdx = (size_t) idx * rand() / RAND_MAX;
		key_t_  tmp;

		tmp = alphabet[idx];
		alphabet[idx] = alphabet[kdx];
		alphabet[kdx] = tmp;
	}

	return alphabet;
}

/**
 * Generate a lookup table with the cumulated density function
 *
 * (This is derived from code originally written by Rene Mueller.)
 */
double * gen_zipf_lut(double zipf_factor, size_t alphabet_size) {
	double scaling_factor;
	double sum;

	double *lut;			  /**< return value */

	lut = (double *) malloc(alphabet_size * sizeof (*lut));
	assert (lut);

	/*
	 * Compute scaling factor such that
	 *
	 *   sum (lut[i], i=1..alphabet_size) = 1.0
	 *
	 */
	scaling_factor = 0.0;
	for (size_t idx = 1; idx <= alphabet_size; idx ++) {
		scaling_factor += 1.0 / pow(idx, zipf_factor);
	}

	/*
	 * Generate the lookup table
	 */
	sum = 0.0;
	for (size_t idx = 1; idx <= alphabet_size; idx ++) {
		sum += 1.0 / pow(idx, zipf_factor);
		lut[idx-1] = sum / scaling_factor;
	}

	return lut;
}


/**
 * Generate a stream with Zipf-distributed content.
 */
void gen_zipf(size_t stream_size, key_t_ alphabet_size, double zipf_factor, tuple_t** output) {
	void* ret;

	/* prepare stuff for Zipf generation */
	key_t_ * alphabet = gen_alphabet(alphabet_size);
	assert (alphabet);

	double* lut = gen_zipf_lut(zipf_factor, alphabet_size);
	assert(lut);

	ret = *output;
	assert(ret);

	tuple_t tuple;

	for (size_t idx = 0; idx < stream_size; idx ++) {

		/* take random number */
		double r = ( (double) rand() ) / RAND_MAX;

		/* binary search in lookup table to determine item */
		size_t left  = 1;
		size_t right = alphabet_size;
		size_t m;	   /* middle between left and right */
		size_t pos;	 /* position to take */

		if (lut[0] >= r) {
			pos = 0;
		} else {
			while (right - left > 1) {
				m = (left + right) / 2;

				if (lut[m] < r) {
					left = m;
				} else {
					right = m;
				}
			}

			pos = right;
		}

		tuple.key = *(alphabet + pos);

		tuple.row_id = idx + 1;
		memcpy(ret + idx * TUPLE_T_SIZE , &tuple, TUPLE_T_SIZE);
	}

	free(lut);
	free(alphabet);

}