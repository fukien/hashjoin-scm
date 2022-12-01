#ifndef _TPCH_Q14_H_
#define _TPCH_Q14_H_

#include "schema.h"


typedef struct {
	tpch_key_t p_partkey;
	char p_type[25];
	char pad[24];
} fltrq14_part_t;

typedef struct {
	tpch_key_t l_partkey;
	double l_extendedprice;
	double l_discount;
	char l_shipdate[10];
	char pad[24];
} fltrq14_lineitem_t;


#endif	/* _TPCH_Q14_H_ */