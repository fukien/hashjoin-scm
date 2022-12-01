#ifndef _TPCH_SCHEMA_H_
#define _TPCH_SCHEMA_H_

#include <stdlib.h>


#define Q1 1
#define Q2 2
#define Q3 3
#define Q4 4
#define Q5 5
#define Q6 6
#define Q7 7
#define Q8 8
#define Q9 9
#define Q10 10
#define Q11 11
#define Q12 12
#define Q13 13
#define Q14 14
#define Q15 15
#define Q16 16
#define Q17 17
#define Q18 18
#define Q19 19
#define Q20 20
#define Q21 21
#define Q22 22


typedef size_t tpch_key_t;
#ifndef TPCH_KEY_T__SIZE
#define TPCH_KEY_T__SIZE sizeof(tpch_key_t)
#endif /* TPCH_KEY_T__SIZE */


typedef struct {
	tpch_key_t p_partkey;
	char p_name[55];
	char p_mfgr[25];
	char p_brand[10];
	char p_type[25];
	int p_size;
	char p_container[10];
	double p_retailprice;
	char p_comment[23];
} __attribute__((packed)) part_t;


typedef struct {
	tpch_key_t l_orderkey;
	tpch_key_t l_partkey;
	tpch_key_t l_suppkey;
	tpch_key_t l_linenumber;
	double l_quantity;
	double l_extendedprice;
	double l_discount;
	double l_tax;
	char l_returnflag[1];
	char l_linestatus[1];
	char l_shipdate[10];
	char l_commitdate[10];
	char l_receiptdate[10];
	char l_shipinstruct[25];
	char l_shipmode[10];
	char l_comment[44];
} __attribute__((packed)) lineitem_t;



#define PART_T_SIZE sizeof(part_t)
#define LINEITEM_T_SIZE sizeof(lineitem_t)



#endif	/* _TPCH_SCHEMA_H_ */