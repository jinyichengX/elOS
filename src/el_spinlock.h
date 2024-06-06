#ifndef EL_SPINLOCK_H
#define EL_SPINLOCK_H

#include "el_pthread.h"

#define cpu_yield PORT_PendSV_Suspend()

#if defined(__C11__)
#include <stdatomic.h>
#endif

typedef struct 
{
	int ticket;
	int turn;
}spinlock_t;

static inline int FetchAndAdd(int* ptr)
{
	int old = *ptr;
	* ptr = old + 1;
	return old;
}

static inline void spinlock_init(spinlock_t * lock)
{
	lock->ticket = 0;
	lock->turn = 0;
}

static inline void spinlock_lock(spinlock_t * lock)
{
	int myturn = FetchAndAdd(&lock->ticket);
	while (lock->turn != myturn)
		cpu_yield;
}

static inline void spinlock_unlock(spinlock_t * lock)
{
	FetchAndAdd(&lock->turn);
}

//FUNCTION(ArchSpinLock)
//    mov     r2, #1
//1:
//    ldrex   r1, [r0]
//    teq     r1, #0
//    wfene
//    strexeq r1, r2, [r0]
//    teqeq   r1, #0
//    bne     1b
//    dmb
//    bx      lr

/*1��LDREX������ȡ�ڴ��е�ֵ������ǶԸö��ڴ�Ķ�ռ���ʣ�
LDREX Rx, [Ry]
�����ָ����ζ�ţ���ȡ�Ĵ���Ryָ���4�ֽ��ڴ�ֵ�����䱣�浽Rx�Ĵ����У�ͬʱ��Ƕ�Ryָ���ڴ�����Ķ�ռ���ʡ�
���ִ��LDREXָ���ʱ�����Ѿ������Ϊ��ռ�����ˣ��������ָ���ִ�в���Ӱ�졣
2����STREX�ڸ����ڴ���ֵʱ������ö��ڴ��Ƿ��Ѿ������Ϊ��ռ���ʣ����Դ��������Ƿ�����ڴ��е�ֵ��
STREX Rx, Ry, [Rz]
���ִ������ָ���ʱ�����Ѿ������Ϊ��ռ�����ˣ��򽫼Ĵ���Ry�е�ֵ���µ��Ĵ���Rzָ����ڴ棬�����Ĵ���Rx���ó�0��ָ��ִ�гɹ��󣬻Ὣ��ռ���ʱ��λ�����
�����ִ������ָ���ʱ����û�����ö�ռ��ǣ��򲻻�����ڴ棬�ҽ��Ĵ���Rx��ֵ���ó�1��
һ��ĳ��STREXָ��ִ�гɹ����Ժ��ٶ�ͬһ���ڴ波��ʹ��STREXָ����µ�ʱ�򣬻ᷢ�ֶ�ռ����Ѿ�������ˣ��Ͳ����ٸ����ˣ��Ӷ�ʵ�ֶ�ռ���ʵĻ��ơ�

���µ����̾�������������ARM�ڲ�Ϊ��ʵ��������ܣ����в��ٸ��ӵ����Ҫ����*/

#endif