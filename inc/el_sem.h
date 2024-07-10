#ifndef EL_SEM_H
#define EL_SEM_H

#include "el_pthread.h"
#include "el_kobj.h"
#include <stdint.h>

typedef struct stSemaphore
{
	struct list_head waiters;
	uint32_t value;
}el_sem_t;
extern void el_sem_init(el_sem_t * sem,uint32_t value);
#if SEM_OBJ_STATIC == 1
extern el_sem_t * el_sem_create(uint32_t value);
#endif
extern EL_RESULT_T el_sem_take(el_sem_t *sem,uint32_t tick);
extern EL_RESULT_T el_sem_trytake(el_sem_t *sem);
extern void el_sem_release(el_sem_t *sem);
extern EL_RESULT_T el_sem_StatisticsTake(el_sem_t *sem,uint32_t * val);
#endif