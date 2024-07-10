#include "el_flag.h"
#include "kparam.h"

#if EL_USE_EVENTFLAG

extern EL_G_SYSTICK_TYPE systick_get(void);
extern EL_G_SYSTICK_TYPE systick_passed(EL_G_SYSTICK_TYPE last_tick);

/**********************************************************************
 * �������ƣ� el_EventFlagInitialise
 * ���������� ��ʼ���¼���־
 * ��������� evtflg : �Ѵ������¼���־����
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/27	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
void el_EventFlagInitialise(evtflg_t * evtflg)
{
	evtflg->evt_bits = 0;
	INIT_LIST_HEAD(&evtflg->waiters);
}

#if EVENTFLAG_OBJ_STATIC == 1
/* �����¼���־�� */
evtflg_t * el_EventFlagCreate(void)
{
	kobj_info_t kobj;
	evtflg_t * evtflg = NULL;
	
	OS_Enter_Critical_Check();
	/* ��������Ƿ�֧���¼���־ */
	if(EL_RESULT_OK != kobj_check(EL_KOBJ_EVTFLG,&kobj))
	 return NULL;
	/* ����һ���ں˶�ʱ�� */
	evtflg = (evtflg_t *)kobj_alloc( EL_KOBJ_EVTFLG );
	OS_Exit_Critical_Check();
	if(NULL == evtflg) return NULL;
	/* ��ʼ���¼���־ */
	el_EventFlagInitialise(evtflg);
	
	return evtflg;
}

/**********************************************************************
 * �������ƣ� el_EventFlagDestroy
 * ���������� ɾ���¼���־
 * ��������� evtflg : �Ѵ������¼���־����
 * ��������� ��
 * �� �� ֵ�� ok/nok
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/27	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
EL_RESULT_T el_EventFlagDestroy(evtflg_t * evtflg)
{
	EL_PTCB_T *ptcb = NULL;
	struct list_head *pos;
	if(evtflg == NULL) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	/* �������б��������߳� */
	while(!list_empty(&evtflg->waiters))
	{
	 pos = evtflg->waiters.next;
	 ptcb = container_of(pos,EL_PTCB_T,pthread_node);
	 EL_Pthread_EventWakeup(ptcb);
	 list_del(pos);
	}
	/* �ڴ�ػ��� */
	kobj_free(EL_KOBJ_EVTFLG,(void * )evtflg);
	OS_Exit_Critical_Check();
	
	return EL_RESULT_OK;
}
#endif

/**********************************************************************
 * �������ƣ� el_EventFlagSetBits
 * ���������� �����¼���־
 * ��������� evtflg : �Ѵ������¼���־����
			  flags������1�ı�־λ
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/27	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
void el_EventFlagSetBits(evtflg_t * evtflg,uint32_t flags)
{
	char need_sched = 0;
	struct list_head *pos,*next;
	EL_PTCB_T *ptcb,*cur_ptcb = NULL;
	if((evtflg == NULL) || (flags == 0))
		return;
	OS_Enter_Critical_Check();
	
	cur_ptcb = EL_GET_CUR_PTHREAD();
	evtflg->evt_bits |= flags;
	/* �������еȴ����¼���־������ */
	list_for_each_safe(pos,next,&evtflg->waiters)
	{
	 ptcb = container_of(pos,EL_PTCB_T,pthread_node);
	 /* �������̵߳ȵȴ��ı�־�Ƿ����� */
	 if(
	    ((WAITBITS_AND == ptcb->andor)&&((evtflg->evt_bits & ptcb->event_wait) == ptcb->event_wait))
	    ||(((WAITBITS_OR == ptcb->andor)) && (evtflg->evt_bits & ptcb->event_wait))
	   ){
	  list_del(&(ptcb->pthread_node));
	  EL_Pthread_EventWakeup(ptcb);
	  /* ��������ѵ��߳����ȼ�����ִ��һ���߳��л� */
	  if( cur_ptcb->pthread_prio < ptcb->pthread_prio ) 
	   need_sched = 1;
	 }
	}
	OS_Exit_Critical_Check();
	if( need_sched )
	 PORT_PendSV_Suspend();
}

/**********************************************************************
 * �������ƣ� el_EventFlagClearBits
 * ���������� ����¼���־λ
 * ��������� evtflg : �Ѵ������¼���־����
			  flags��������ı�־λ
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/27	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
uint32_t el_EventFlagClearBits(evtflg_t * evtflg, uint32_t flags)
{
	uint32_t ret;
	if((evtflg == NULL) || (flags == 0))
	 return 0;
	
    OS_Enter_Critical_Check();
	ret = evtflg->evt_bits;
    evtflg->evt_bits &= ~flags;
    OS_Exit_Critical_Check();
	
	return ret;
}

/**********************************************************************
 * �������ƣ� el_EventFlagWaitBits
 * ���������� �ȴ��¼���־
 * ��������� evtflg : �Ѵ������¼���־����
			  flags���ȴ��ı�־λ
			  clear���Ƿ���Ҫ����ñ�־λ
			  andor����ȴ�/��ȴ�
			  tick����ʱʱ��
 * ��������� ��
 * �� �� ֵ�� �����������¼���־
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/27	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
uint32_t el_EventFlagWaitBits(evtflg_t * evtflg,uint32_t flags,\
							bool clear,andor_t andor,uint32_t tick)
{
	EL_PTCB_T *cur_ptcb = NULL;
	Suspend_t *SuspendObj;
	uint32_t ret = 0;
	int evt_ret = (int)EL_RESULT_ERR;
	EL_G_SYSTICK_TYPE tick_line = systick_get()+tick;
	EL_G_SYSTICK_TYPE tick_now;
	if((evtflg == NULL) || (flags == 0))
		return 0;
	cur_ptcb = EL_GET_CUR_PTHREAD();

	OS_Enter_Critical_Check();
	if(
		!(((WAITBITS_AND == andor)&&((evtflg->evt_bits & flags) == flags))
		||(((WAITBITS_OR == andor)) && (evtflg->evt_bits & flags)))
	   )
	{
	 if(!tick){
	  OS_Exit_Critical_Check();
	  return ret;
	 }
	 cur_ptcb->event_wait = flags;
	 cur_ptcb->andor = andor;
	 if(tick != _MAX_TICKS_TO_WAIT){
	  while(
	   !(
	    ((WAITBITS_AND == andor)&&((evtflg->evt_bits & flags) == flags))
	    ||(((WAITBITS_OR == andor)) && (evtflg->evt_bits & flags))
	    )
	  ){
	   if(!tick){
		OS_Exit_Critical_Check();
		return ret;
	   }
	   /* ���̹߳��ӽڵ���ӵ��ź����ȴ����� */
	   list_add_tail(&cur_ptcb->pthread_node,&evtflg->waiters);
	   /* ע��ȴ��¼� */
	   EL_Pthread_EventWait(cur_ptcb,EVENT_TYPE_EVTFLAG_WAIT,tick,&evt_ret);
	   OS_Exit_Critical_Check();
	   PORT_PendSV_Suspend();
	   OS_Enter_Critical_Check();
	   tick_now = systick_get();
	   tick = (tick_line < tick_now)?0:(tick_line - tick_now);
	  }
	 }else{
	  while(
	   !(
		((WAITBITS_AND == andor)&&((evtflg->evt_bits & flags) == flags))
		||(((WAITBITS_OR == andor)) && (evtflg->evt_bits & flags))
		)
	   )
	  {
	   /* ����ǰ�߳� */
	   if (NULL == (SuspendObj = (Suspend_t *)kobj_alloc(EL_KOBJ_SUSPEND))){
		OS_Exit_Critical_Check();
		return EL_RESULT_ERR;
	   }
	   /* ���̷߳���ȴ����� */
	   list_add_tail(&cur_ptcb->pthread_node,&evtflg->waiters);
	   SuspendObj->Pthread = cur_ptcb;
	   SuspendObj->PendingType = (void *)0;
	   cur_ptcb->block_holder = (void *)SuspendObj;
	   EL_Pthread_PushToSuspendList(cur_ptcb,&SuspendObj->Suspend_Node);

	   OS_Exit_Critical_Check();
	   PORT_PendSV_Suspend();
	   OS_Enter_Critical_Check();
	  }
	 }
	}
	ret = evtflg->evt_bits;
	if(clear == true)
	 el_EventFlagClearBits(evtflg, flags);
	OS_Exit_Critical_Check();
	
	return ret;
}

#endif