/**
 * @file	task_queue.h
 * @author  Cagri Balkesen <cagri.balkesen@inf.ethz.ch>
 * @date	Sat Feb  4 20:00:58 2012
 * @version $Id: task_queue.h 3017 2012-12-07 10:56:20Z bcagri $
 * 
 * @brief  Implements task queue facility for the join processing.
 * 
 */
#ifndef TASK_CAS_H
#define TASK_CAS_H

#include <immintrin.h>  // _mm_mfence
#include <stdlib.h>

#include "lock_cas.h"
#include "store.h"

typedef struct task_t task_t; 
typedef struct task_list_t task_list_t; 
typedef struct task_queue_t task_queue_t;

struct task_t {
	relation_t r_rel;
	relation_t r_tmp;
	relation_t s_rel;
	relation_t s_tmp;
	task_t *next;

#if NUM_PASSES != 1
	size_t r_global_offset;
	size_t s_global_offset;
#endif /* NUM_PASSES != 1 */
};

struct task_list_t {
	task_t *tasks;
	task_list_t *next;
	size_t curr;
};

struct task_queue_t {
	lock_t lock;
	lock_t alloc_lock;
	task_t *head;
	task_list_t *free_list;
	size_t count;
	size_t alloc_num;
};


/**************** DECLARATIONS ********************************************/

static inline task_t * get_next_task(task_queue_t *tq) __attribute__((always_inline));

static inline void add_tasks(task_queue_t *tq, task_t *t) __attribute__((always_inline));

/* atomically get the next available task */
static inline task_t * task_queue_get_atomic(task_queue_t *tq) __attribute__((always_inline));

/* atomically add a task */
static inline void task_queue_add_atomic(task_queue_t *tq, task_t *t) __attribute__((always_inline));

static inline void task_queue_add(task_queue_t *tq, task_t *t) __attribute__((always_inline));

static inline void task_queue_copy_atomic(task_queue_t *tq, task_t *t) __attribute__((always_inline));

/* get a free slot of task_t */
static inline task_t * task_queue_get_slot_atomic(task_queue_t *tq) __attribute__((always_inline));

static inline task_t * task_queue_get_slot(task_queue_t *tq) __attribute__((always_inline));

/* initialize a task queue with given allocation block size */
static task_queue_t * task_queue_init(size_t alloc_num);

static void task_queue_free(task_queue_t *tq);

/**************** DEFINITIONS ********************************************/

static inline task_t* get_next_task(task_queue_t *tq) {
	lock(&tq->lock);
	task_t *ret = NULL;
	if (tq->count > 0){
		ret = tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	unlock(&tq->lock);

	return ret;
}

static inline void add_tasks(task_queue_t *tq, task_t *t) {
	lock(&tq->lock);
	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
	unlock(&tq->lock);
}

static inline task_t * task_queue_get_atomic(task_queue_t *tq) {
	lock(&tq->lock);
	task_t *ret = NULL;
	if(tq->count > 0) {
		ret	= tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	unlock(&tq->lock);

	return ret;
}

static inline void task_queue_add_atomic(task_queue_t *tq, task_t *t) {
	lock(&tq->lock);

	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
	_mm_mfence();

	unlock(&tq->lock);
}

static inline void task_queue_add(task_queue_t *tq, task_t *t) {
	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
}

static inline void task_queue_copy_atomic(task_queue_t *tq, task_t *t) {
	lock(&tq->lock);
	task_t *slot = task_queue_get_slot(tq);
	*slot = *t; /* copy */
	task_queue_add(tq, slot);
	unlock(&tq->lock);
}

static inline task_t * task_queue_get_slot(task_queue_t *tq) {
	task_list_t *l = tq->free_list;
	task_t *ret;
	if(l->curr < tq->alloc_num) {
		ret = &(l->tasks[l->curr]);
		(l->curr) ++;
	} else {
		task_list_t * nl = (task_list_t *) malloc(sizeof(task_list_t));
		nl->tasks = (task_t *) malloc(tq->alloc_num * sizeof(task_t));		// could be modified to alloc on NVM
		nl->curr = 1;
		nl->next = tq->free_list;
		tq->free_list = nl;
		ret = &(nl->tasks[0]);
	}

	return ret;
}

/* get a free slot of task_t */
static inline task_t * task_queue_get_slot_atomic(task_queue_t *tq) {
	lock(&tq->alloc_lock);
	task_t *ret = task_queue_get_slot(tq);
	unlock(&tq->alloc_lock);

	return ret;
}

/* initialize a task queue with given allocation block size */
static inline task_queue_t * task_queue_init(size_t alloc_num) {
	task_queue_t *ret = (task_queue_t *) malloc(sizeof(task_queue_t));
	ret->free_list = (task_list_t *) malloc(sizeof(task_list_t));
	ret->free_list->tasks = (task_t *) malloc(alloc_num * sizeof(task_t));	// could be modified to alloc on NVM
	ret->free_list->curr = 0;
	ret->free_list->next = NULL;
	ret->count = 0;
	ret->alloc_num = alloc_num;
	ret->head = NULL;
	ret->lock = 0;
	ret->alloc_lock = 0;

	return ret;
}

static inline void task_queue_free(task_queue_t *tq) {
	task_list_t *tmp = tq->free_list;
	while(tmp) {
		free(tmp->tasks);													// dealloc_memory(tmp->tasks, sizeof(task_t) * tmp->alloc_num)
		task_list_t *tmp2 = tmp->next;
		free(tmp);
		tmp = tmp2;
	}
	free(tq);
}


#endif /* TASK_CAS_H */