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

/*1）LDREX用来读取内存中的值，并标记对该段内存的独占访问：
LDREX Rx, [Ry]
上面的指令意味着，读取寄存器Ry指向的4字节内存值，将其保存到Rx寄存器中，同时标记对Ry指向内存区域的独占访问。
如果执行LDREX指令的时候发现已经被标记为独占访问了，并不会对指令的执行产生影响。
2）而STREX在更新内存数值时，会检查该段内存是否已经被标记为独占访问，并以此来决定是否更新内存中的值：
STREX Rx, Ry, [Rz]
如果执行这条指令的时候发现已经被标记为独占访问了，则将寄存器Ry中的值更新到寄存器Rz指向的内存，并将寄存器Rx设置成0。指令执行成功后，会将独占访问标记位清除。
而如果执行这条指令的时候发现没有设置独占标记，则不会更新内存，且将寄存器Rx的值设置成1。
一旦某条STREX指令执行成功后，以后再对同一段内存尝试使用STREX指令更新的时候，会发现独占标记已经被清空了，就不能再更新了，从而实现独占访问的机制。

大致的流程就是这样，但是ARM内部为了实现这个功能，还有不少复杂的情况要处理。*/

#endif