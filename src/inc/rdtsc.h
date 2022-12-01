/*
	Copyright 2011, Spyros Blanas.
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
 * CHANGELOG
 *  - changed `unsigned long long' declerations to size_t and added include
 *	for <stdint.h>. May 2012, Cagri.
 *
 */

#ifndef RDTSC_H
#define RDTSC_H


#include <stdlib.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#if !defined(__i386__) && !defined(__x86_64__) && !defined(__sparc__)
#warning No supported architecture found -- timers will return junk.
#endif

static inline size_t curtick() {
	size_t tick;
#if defined(__i386__)
	unsigned long lo, hi;
	__asm__ __volatile__ (".byte 0x0f, 0x31" : "=a" (lo), "=d" (hi));
	tick = (size_t) hi << 32 | lo;
#elif defined(__x86_64__)
	unsigned long lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	tick = (size_t) hi << 32 | lo;
#elif defined(__sparc__)
	__asm__ __volatile__ ("rd %%tick, %0" : "=r" (tick));
#endif
	return tick;
}

static inline void startTimer(size_t* t) {
	*t = curtick();
}

static inline void stopTimer(size_t* t) {
	*t = curtick() - *t;
}

static inline double diff_usec(struct timeval * start, struct timeval * end) {
	return (((*end).tv_sec*1000000L + (*end).tv_usec)
			- ((*start).tv_sec*1000000L+(*start).tv_usec));
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif  /* RDTSC_H */