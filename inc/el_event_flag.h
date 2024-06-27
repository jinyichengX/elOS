#ifndef EL_EVENT_FLAG_H
#define EL_EVENT_FLAG_H

#include "el_pthread.h"
#include "el_kobj.h"
#include <stdint.h>
#if EL_USE_EVENTFLAG
typedef enum 
{
	WAITBITS_AND,
	WAITBITS_OR
}andor_t;

typedef struct stEventFlag
{
	struct list_head waiters;
	uint32_t evt_bits;
}evtflg_t;
#if EVENTFLAG_OBJ_STATIC == 1
extern evtflg_t * el_EventFlagCreate(void);
extern EL_RESULT_T el_EventFlagrDestroy(evtflg_t * evtflg);
#endif
extern void el_EventFlagSetBits(evtflg_t * evtflg,uint32_t flags);
extern void el_EventFlagClearBits(evtflg_t * evtflg, uint32_t flags);
extern uint32_t el_EventFlagWaitBits(evtflg_t * evtflg,uint32_t flags,\
							bool clear,andor_t andor,uint32_t tick);
#endif
#endif