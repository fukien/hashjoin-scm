#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "store.h"


double zeta_func(const key_t_  n, const double theta) {
	double ans = 0.0;
	for (key_t_  i = 1; i < n + 1; i ++) {
		ans += pow( ( (double) 1 ) / i, theta);
	}
	return ans;
}


double eta_func(const double theta, const double zetan, const key_t_  min_key, const key_t_  max_key) {
	/**
	the eta computation from Jim Gray SIGMOD 1994 paper, Quickly Generating Billion-Record Synthetic Databases
	it seems like a typo of Jim Gray
	double eta = ( 1 - pow( ( (double) 2 ) / ( max_key - min_key + 1 ), 1 - theta ) ) / ( 1 - zeta_func( (int) theta , 2 ) / zetan);
	*/
	double eta = ( 1 - pow( ( (double) 2 ) / ( max_key - min_key + 1 ), 1 - theta ) ) / ( 1 - zeta_func( 2 , theta ) / zetan );	// same as "An Experimental Comparison of Thirteen relation_tal Equi-Joins in Main Memory"

	return eta;
}