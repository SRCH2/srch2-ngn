/*
 * mypthread.h
 *
 *  Created on: Jun 11, 2013
 *      Author: sina
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
