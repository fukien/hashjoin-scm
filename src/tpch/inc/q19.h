#ifndef _TPCH_Q19_H_
#define _TPCH_Q19_H_

#include "schema.h"


typedef struct {
	tpch_key_t p_partkey;
	char p_brand[10];
	int p_size;
	char p_container[10];
	char pad[24];
} fltrq19_part_t;


typedef struct {
	tpch_key_t l_partkey;
	double l_extendedprice;
	double l_discount;
	double l_quantity;
	char l_shipinstruct[25];
	char l_shipmode[10];
	char pad[56];
} fltrq19_lineitem_t;


typedef struct {
	char p_brand[10];
	int p_size;
	char p_container[10];
	double l_quantity;
	char l_shipinstruct[25];
	char l_shipmode[10];
} fltrq19_match_t;


#endif	/* _TPCH_Q19_H_ */