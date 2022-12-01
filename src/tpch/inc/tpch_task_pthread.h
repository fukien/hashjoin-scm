/**
 * @file	tpch_task_queue.h
 * @author  Cagri Balkesen <cagri.balkesen@inf.ethz.ch>
 * @date	Sat Feb  4 20:00:58 2012
 * @version $Id: tpch_task_queue.h 3017 2012-12-07 10:56:20Z bcagri $
 * 
 * @brief  Implements tpch_task queue facility for the join processing.
 * 
 */
#ifndef TPCH_TASK_PTHREAD_H
#define TPCH_TASK_PTHREAD_H

#include <pthread.h>
#include <stdlib.h>

#include "store.h"

typedef struct tpch_task_t tpch_task_t; 
typedef struct tpch_task_list_t tpch_task_list_t; 
typedef struct tpch_task_queue_t tpch_task_queue_t;

struct tpch_task_t {
	fltrq_part_t *fltrq_part, *fltrq_part_tmp;
	fltrq_lineitem_t *fltrq_lineitem, *fltrq_lineitem_tmp;

	size_t self_part_num;
	size_t self_lineitem_num;

	tpch_task_t *next;

#if NUM_PASSES != 1
	size_t part_global_offset;
	size_t lineitem_global_offset;

#ifdef BW_REG
	size_t *part_hist_2nd_pass;
	size_t *lineitem_hist_2nd_pass;

#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
	void *part_swwcb_2nd_pass_mem;
	void *lineitem_swwcb_2nd_pass_mem;
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */
#endif /* BW_REG */
#endif /* NUM_PASSES != 1 */
};

struct tpch_task_list_t {
	tpch_task_t *tpch_tasks;
	tpch_task_list_t *next;
	size_t curr;
};

struct tpch_task_queue_t {
	pthread_mutex_t lock;
	pthread_mutex_t alloc_lock;
	tpch_task_t *head;
	tpch_task_list_t *free_list;
	size_t count;
	size_t alloc_num;
};


/**************** DECLARATIONS ********************************************/

static inline tpch_task_t * get_next_tpch_task(tpch_task_queue_t *tq) __attribute__((always_inline));

static inline void add_tpch_tasks(tpch_task_queue_t *tq, tpch_task_t *t) __attribute__((always_inline));

/* atomically get the next available tpch_task */
static inline tpch_task_t * tpch_task_queue_get_atomic(tpch_task_queue_t *tq) __attribute__((always_inline));

/* atomically add a tpch_task */
static inline void tpch_task_queue_add_atomic(tpch_task_queue_t *tq, tpch_task_t *t) __attribute__((always_inline));

static inline void tpch_task_queue_add(tpch_task_queue_t *tq, tpch_task_t *t) __attribute__((always_inline));

static inline void tpch_task_queue_copy_atomic(tpch_task_queue_t *tq, tpch_task_t *t) __attribute__((always_inline));

/* get a free slot of tpch_task_t */
static inline tpch_task_t * tpch_task_queue_get_slot_atomic(tpch_task_queue_t *tq) __attribute__((always_inline));

static inline tpch_task_t * tpch_task_queue_get_slot(tpch_task_queue_t *tq) __attribute__((always_inline));

/* initialize a tpch_task queue with given allocation block size */
static tpch_task_queue_t * tpch_task_queue_init(size_t alloc_num);

static void tpch_task_queue_free(tpch_task_queue_t *tq);

/**************** DEFINITIONS ********************************************/

static inline tpch_task_t* get_next_tpch_task(tpch_task_queue_t *tq) {
	pthread_mutex_lock(&tq->lock);
	tpch_task_t *ret = NULL;
	if (tq->count > 0){
		ret = tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	pthread_mutex_unlock(&tq->lock);

	return ret;
}

static inline void add_tpch_tasks(tpch_task_queue_t *tq, tpch_task_t *t) {
	pthread_mutex_lock(&tq->lock);
	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
	pthread_mutex_unlock(&tq->lock);
}

static inline tpch_task_t * tpch_task_queue_get_atomic(tpch_task_queue_t *tq) {
	pthread_mutex_lock(&tq->lock);
	tpch_task_t *ret = NULL;
	if(tq->count > 0) {
		ret	= tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	pthread_mutex_unlock(&tq->lock);

	return ret;
}


static inline void tpch_task_queue_add_atomic(tpch_task_queue_t *tq, tpch_task_t *t) {

	pthread_mutex_lock(&tq->lock);

	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;

	pthread_mutex_unlock(&tq->lock);
}

static inline void tpch_task_queue_add(tpch_task_queue_t *tq, tpch_task_t *t) {
	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
}

static inline void tpch_task_queue_copy_atomic(tpch_task_queue_t *tq, tpch_task_t *t) {
	pthread_mutex_lock(&tq->lock);
	tpch_task_t *slot = tpch_task_queue_get_slot(tq);
	*slot = *t; /* copy */
	tpch_task_queue_add(tq, slot);
	pthread_mutex_unlock(&tq->lock);
}

static inline tpch_task_t * tpch_task_queue_get_slot(tpch_task_queue_t *tq) {
	tpch_task_list_t *l = tq->free_list;
	tpch_task_t *ret;
	if(l->curr < tq->alloc_num) {
		ret = &(l->tpch_tasks[l->curr]);
		(l->curr) ++;
	} else {
		tpch_task_list_t * nl = (tpch_task_list_t *) malloc(sizeof(tpch_task_list_t));	
		nl->tpch_tasks = (tpch_task_t *) malloc(tq->alloc_num * sizeof(tpch_task_t));		// could be modified to alloc on NVM
		nl->curr = 1;
		nl->next = tq->free_list;
		tq->free_list = nl;
		ret = &(nl->tpch_tasks[0]);
	}

	return ret;
}

/* get a free slot of tpch_task_t */
static inline tpch_task_t * tpch_task_queue_get_slot_atomic(tpch_task_queue_t *tq) {
	pthread_mutex_lock(&tq->alloc_lock);
	tpch_task_t *ret = tpch_task_queue_get_slot(tq);
	pthread_mutex_unlock(&tq->alloc_lock);

	return ret;
}

/* initialize a tpch_task queue with given allocation block size */
static inline tpch_task_queue_t * tpch_task_queue_init(size_t alloc_num) {
	tpch_task_queue_t *ret = (tpch_task_queue_t *) malloc(sizeof(tpch_task_queue_t));
	ret->free_list = (tpch_task_list_t *) malloc(sizeof(tpch_task_list_t));
	ret->free_list->tpch_tasks = (tpch_task_t *) malloc(alloc_num * sizeof(tpch_task_t));	// could be modified to alloc on NVM
	ret->free_list->curr = 0;
	ret->free_list->next = NULL;
	ret->count = 0;
	ret->alloc_num = alloc_num;
	ret->head = NULL;
    pthread_mutex_init(&ret->lock, NULL);
    pthread_mutex_init(&ret->alloc_lock, NULL);

	return ret;
}

static inline void tpch_task_queue_free(tpch_task_queue_t *tq)  {
	tpch_task_list_t *tmp = tq->free_list;
	while(tmp) {
		free(tmp->tpch_tasks);													// dealloc_memory(tmp->tpch_tasks, sizeof(tpch_task_t) * tmp->alloc_num)
		tpch_task_list_t *tmp2 = tmp->next;
		free(tmp);
		tmp = tmp2;
	}
	free(tq);
}


#endif /* TPCH_TASK_PTHREAD_H */