/**
 * @file	task_queue.h
 * @author  Cagri Balkesen <cagri.balkesen@inf.ethz.ch>
 * @date	Sat Feb  4 20:00:58 2012
 * @version $Id: task_queue.h 3017 2012-12-07 10:56:20Z bcagri $
 * 
 * @brief  Implements task queue facility for the join processing.
 * 
 */
#ifndef TASK_PTR_PTHREAD_H
#define TASK_PTR_PTHREAD_H

#include <pthread.h>
#include <stdlib.h>

#include "store.h"

typedef struct ptr_task_t ptr_task_t; 
typedef struct ptr_task_list_t ptr_task_list_t; 
typedef struct ptr_task_queue_t ptr_task_queue_t;

struct ptr_task_t {
	ptr_relation_t r_rel;
	ptr_relation_t r_tmp;
	ptr_relation_t s_rel;
	ptr_relation_t s_tmp;
	ptr_task_t *next;

#if NUM_PASSES != 1
	size_t r_global_offset;
	size_t s_global_offset;

#ifdef BW_REG
	size_t *r_hist_2nd_pass;
	size_t *s_hist_2nd_pass;
#ifdef USE_SWWCB_OPTIMIZED_RADIX_CLUSTER
	void *r_swwcb_2nd_pass_mem;
	void *s_swwcb_2nd_pass_mem;
#endif /* USE_SWWCB_OPTIMIZED_RADIX_CLUSTER */
#endif /* BW_REG */
#endif /* NUM_PASSES != 1 */
};

struct ptr_task_list_t {
	ptr_task_t *tasks;
	ptr_task_list_t *next;
	size_t curr;
};

struct ptr_task_queue_t {
	pthread_mutex_t lock;
	pthread_mutex_t alloc_lock;
	ptr_task_t *head;
	ptr_task_list_t *free_list;
	size_t count;
	size_t alloc_num;
};


/**************** DECLARATIONS ********************************************/

static inline ptr_task_t * get_next_task(ptr_task_queue_t *tq) __attribute__((always_inline));

static inline void add_tasks(ptr_task_queue_t *tq, ptr_task_t *t) __attribute__((always_inline));

/* atomically get the next available task */
static inline ptr_task_t * task_queue_get_atomic(ptr_task_queue_t *tq) __attribute__((always_inline));

/* atomically add a task */
static inline void task_queue_add_atomic(ptr_task_queue_t *tq, ptr_task_t *t) __attribute__((always_inline));

static inline void task_queue_add(ptr_task_queue_t *tq, ptr_task_t *t) __attribute__((always_inline));

static inline void task_queue_copy_atomic(ptr_task_queue_t *tq, ptr_task_t *t) __attribute__((always_inline));

/* get a free slot of ptr_task_t */
static inline ptr_task_t * task_queue_get_slot_atomic(ptr_task_queue_t *tq) __attribute__((always_inline));

static inline ptr_task_t * task_queue_get_slot(ptr_task_queue_t *tq) __attribute__((always_inline));

/* initialize a task queue with given allocation block size */
static ptr_task_queue_t * task_queue_init(size_t alloc_num);

static void task_queue_free(ptr_task_queue_t *tq);

/**************** DEFINITIONS ********************************************/

static inline ptr_task_t* get_next_task(ptr_task_queue_t *tq) {
	pthread_mutex_lock(&tq->lock);
	ptr_task_t *ret = NULL;
	if (tq->count > 0){
		ret = tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	pthread_mutex_unlock(&tq->lock);

	return ret;
}

static inline void add_tasks(ptr_task_queue_t *tq, ptr_task_t *t) {
	pthread_mutex_lock(&tq->lock);
	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
	pthread_mutex_unlock(&tq->lock);
}

static inline ptr_task_t * task_queue_get_atomic(ptr_task_queue_t *tq) {
	pthread_mutex_lock(&tq->lock);
	ptr_task_t *ret = NULL;
	if(tq->count > 0) {
		ret	= tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	pthread_mutex_unlock(&tq->lock);

	return ret;
}


static inline void task_queue_add_atomic(ptr_task_queue_t *tq, ptr_task_t *t) {

	pthread_mutex_lock(&tq->lock);

	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;

	pthread_mutex_unlock(&tq->lock);
}

static inline void task_queue_add(ptr_task_queue_t *tq, ptr_task_t *t) {
	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
}

static inline void task_queue_copy_atomic(ptr_task_queue_t *tq, ptr_task_t *t) {
	pthread_mutex_lock(&tq->lock);
	ptr_task_t *slot = task_queue_get_slot(tq);
	*slot = *t; /* copy */
	task_queue_add(tq, slot);
	pthread_mutex_unlock(&tq->lock);
}

static inline ptr_task_t * task_queue_get_slot(ptr_task_queue_t *tq) {
	ptr_task_list_t *l = tq->free_list;
	ptr_task_t *ret;
	if(l->curr < tq->alloc_num) {
		ret = &(l->tasks[l->curr]);
		(l->curr) ++;
	} else {
		ptr_task_list_t * nl = (ptr_task_list_t *) malloc(sizeof(ptr_task_list_t));	
		nl->tasks = (ptr_task_t *) malloc(tq->alloc_num * sizeof(ptr_task_t));		// could be modified to alloc on NVM
		nl->curr = 1;
		nl->next = tq->free_list;
		tq->free_list = nl;
		ret = &(nl->tasks[0]);
	}

	return ret;
}

/* get a free slot of ptr_task_t */
static inline ptr_task_t * task_queue_get_slot_atomic(ptr_task_queue_t *tq) {
	pthread_mutex_lock(&tq->alloc_lock);
	ptr_task_t *ret = task_queue_get_slot(tq);
	pthread_mutex_unlock(&tq->alloc_lock);

	return ret;
}

/* initialize a task queue with given allocation block size */
static inline ptr_task_queue_t * task_queue_init(size_t alloc_num) {
	ptr_task_queue_t *ret = (ptr_task_queue_t *) malloc(sizeof(ptr_task_queue_t));
	ret->free_list = (ptr_task_list_t *) malloc(sizeof(ptr_task_list_t));
	ret->free_list->tasks = (ptr_task_t *) malloc(alloc_num * sizeof(ptr_task_t));	// could be modified to alloc on NVM
	ret->free_list->curr = 0;
	ret->free_list->next = NULL;
	ret->count = 0;
	ret->alloc_num = alloc_num;
	ret->head = NULL;
    pthread_mutex_init(&ret->lock, NULL);
    pthread_mutex_init(&ret->alloc_lock, NULL);

	return ret;
}

static inline void task_queue_free(ptr_task_queue_t *tq)  {
	ptr_task_list_t *tmp = tq->free_list;
	while(tmp) {
		free(tmp->tasks);													// dealloc_memory(tmp->tasks, sizeof(ptr_task_t) * tmp->alloc_num)
		ptr_task_list_t *tmp2 = tmp->next;
		free(tmp);
		tmp = tmp2;
	}
	free(tq);
}


#endif /* TASK_PTR_PTHREAD_H */