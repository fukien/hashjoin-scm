// #pragma once

#ifndef LOCK_CAS_H
#define LOCK_CAS_H

typedef volatile char lock_t;

inline void unlock(lock_t *_l) __attribute__((always_inline));
inline void lock(lock_t *_l) __attribute__((always_inline));

/** Unlocks the lock object. */
inline void unlock(lock_t* _l) { 
	*_l = 0;
}

/** Unlocks the lock object. */
inline void lock(lock_t* _l) { 
	while(!__sync_bool_compare_and_swap(_l, 0, 1)) {
#if defined(__i386__) || defined(__x86_64__)
		__asm__ __volatile__ ("pause\n");
#endif
	}
}

#endif  /* LOCK_CAS_H */