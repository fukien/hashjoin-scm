/**
 * @file	task_queue.h
 * @author  Cagri Balkesen <cagri.balkesen@inf.ethz.ch>
 * @date	Sat Feb  4 20:00:58 2012
 * @version $Id: task_queue.h 3017 2012-12-07 10:56:20Z bcagri $
 * 
 * @brief  Implements task queue facility for the join processing.
 * 
 */
#ifndef TASK_PTHREAD_SHR_H
#define TASK_PTHREAD_SHR_H

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#include "store.h"
#include "../algo/part_buffer.h"

typedef struct task_t task_t; 
typedef struct task_list_t task_list_t; 
typedef struct task_queue_t task_queue_t;

struct task_t {
	shr_part_buffer_t *r_part_buffer;
	size_t r_tuple_num;
	shr_part_buffer_t *s_part_buffer;
	size_t s_tuple_num;

#if NUM_PASSES != 1
#ifdef SKEW_HANDLING
	size_t r_link_buffer_num;
	size_t s_link_buffer_num;

	bool skew_skew_flag;
	size_t skew_skew_s_start_idx;
	size_t skew_skew_s_buffer_num;
#endif /* SKEW_HANDLING */
#endif /* NUM_PASSES != 1 */

	task_t *next;
};

struct task_list_t {
	task_t *tasks;
	task_list_t *next;
	size_t curr;
};

struct task_queue_t {
	pthread_mutex_t lock;
	pthread_mutex_t alloc_lock;
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

/* atomically get the next available task and the respective count id */
static inline task_t * task_queue_get_count_atomic(task_queue_t *tq, size_t *count) __attribute__((always_inline));

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
	pthread_mutex_lock(&tq->lock);
	task_t *ret = NULL;
	if (tq->count > 0){
		ret = tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	pthread_mutex_unlock(&tq->lock);

	return ret;
}

static inline void add_tasks(task_queue_t *tq, task_t *t) {
	pthread_mutex_lock(&tq->lock);
	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
	pthread_mutex_unlock(&tq->lock);
}

static inline task_t * task_queue_get_atomic(task_queue_t *tq) {
	pthread_mutex_lock(&tq->lock);
	task_t *ret = NULL;
	if(tq->count > 0) {
		ret	= tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	pthread_mutex_unlock(&tq->lock);

	return ret;
}

static inline task_t * task_queue_get_count_atomic(task_queue_t *tq, size_t *count) {
	pthread_mutex_lock(&tq->lock);
	task_t *ret = NULL;
	*count = tq->count;
	if(tq->count > 0) {
		ret	= tq->head;
		tq->head = ret->next;
		(tq->count) --;
	}
	pthread_mutex_unlock(&tq->lock);

	return ret;
}

static inline void task_queue_add_atomic(task_queue_t *tq, task_t *t) {

	pthread_mutex_lock(&tq->lock);

	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;

	pthread_mutex_unlock(&tq->lock);
}

static inline void task_queue_add(task_queue_t *tq, task_t *t) {
	t->next = tq->head;
	tq->head = t;
	(tq->count) ++;
}

static inline void task_queue_copy_atomic(task_queue_t *tq, task_t *t) {
	pthread_mutex_lock(&tq->lock);
	task_t *slot = task_queue_get_slot(tq);
	*slot = *t; /* copy */
	task_queue_add(tq, slot);
	pthread_mutex_unlock(&tq->lock);
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
	pthread_mutex_lock(&tq->alloc_lock);
	task_t *ret = task_queue_get_slot(tq);
	pthread_mutex_unlock(&tq->alloc_lock);

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
    pthread_mutex_init(&ret->lock, NULL);
    pthread_mutex_init(&ret->alloc_lock, NULL);

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


#endif /* TASK_PTHREAD_SHR_H */