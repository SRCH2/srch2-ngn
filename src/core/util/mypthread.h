/*
 * Copyright (c) 2016, SRCH2
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the SRCH2 nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL SRCH2 BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * mypthread.h
 *
 *  Created on: Jun 11, 2013
 */

#ifndef MYPTHREAD_H_
#define MYPTHREAD_H_
#include <pthread.h>

#if defined(__MACH__) || defined(ANDROID) // For Mac OSX and Android system
/**
 * DO NOT USE sem_ini
 * As it is provided bellow sem_ini does not work on mac. But instead we can use
 * sem_open which works on both Mac OSX and Linux.
 * http://stackoverflow.com/questions/1413785/sem-init-on-os-x
 */
typedef pthread_mutex_t pthread_spinlock_t;

inline int pthread_spin_init(pthread_spinlock_t *lock, int pshared){
	return pthread_mutex_init(lock,0);
}

inline int pthread_spin_destroy(pthread_spinlock_t *lock){
	return pthread_mutex_destroy(lock);
}

inline int pthread_spin_lock(pthread_spinlock_t *lock){
	return pthread_mutex_lock(lock);
}

inline int pthread_spin_unlock(pthread_spinlock_t *lock){
	return pthread_mutex_unlock(lock);
}


#else // For Linux

#endif
#endif /* MYPTHREAD_H_ */
