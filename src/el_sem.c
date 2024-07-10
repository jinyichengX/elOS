#include "el_sem.h"
#include "kparam.h"

extern EL_G_SYSTICK_TYPE systick_get(void);
extern EL_G_SYSTICK_TYPE systick_passed(EL_G_SYSTICK_TYPE last_tick);

/**********************************************************************
 * �������ƣ� el_sem_init
 * ���������� ��ʼ���ź���
 * ��������� sem : �Ѵ������ź�������
             value : �ź�����ʼ����
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
void el_sem_init(el_sem_t * sem,uint32_t value)
{
	INIT_LIST_HEAD(&sem->waiters);
	sem->value = value;
}

/**********************************************************************
 * �������ƣ� el_sem_create
 * ���������� ��������ʼ���ź���
 * ��������� value : �ź�����ʼ����
 * ��������� ��
 * �� �� ֵ�� �ź������
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
#if SEM_OBJ_STATIC == 1
el_sem_t * el_sem_create(uint32_t value)
{
	kobj_info_t kobj;
	el_sem_t * sem = NULL;
	
	OS_Enter_Critical_Check();
	/* ��������Ƿ�֧���¼���־ */
	if(EL_RESULT_OK != kobj_check( EL_KOBJ_SEM,&kobj)){
		OS_Exit_Critical_Check();
		return NULL;
	}
	sem = (el_sem_t * )kobj_alloc( EL_KOBJ_SEM );
	
	if(NULL == sem){
		OS_Exit_Critical_Check();
		return NULL; 
	}
	OS_Exit_Critical_Check();
	el_sem_init(sem,value);
	
	return sem;
}
#endif

/**********************************************************************
 * �������ƣ� el_sem_take
 * ���������� ������ȡ�ź���
 * ��������� sem : �Ѵ������ź�������
             tick : �ȴ���ʱ����
 * ��������� ��
 * �� �� ֵ�� ok/nok
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
EL_RESULT_T el_sem_take(el_sem_t *sem,uint32_t tick)
{
	int ret = (int)EL_RESULT_ERR;
	EL_PTCB_T *ptcb = NULL;
	Suspend_t *SuspendObj;
	EL_G_SYSTICK_TYPE tick_line = systick_get()+tick;
	EL_G_SYSTICK_TYPE tick_now;
	if( sem == NULL ) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	ptcb = EL_GET_CUR_PTHREAD();
	if(sem->value){
	 sem->value --;
	 OS_Exit_Critical_Check();
	 return EL_RESULT_OK;
	}
	else{
	 if(tick != _MAX_TICKS_TO_WAIT){
	  while(!sem->value){
	   if(!tick){
		OS_Exit_Critical_Check();
		return EL_RESULT_ERR;
	   }
	   /* ���̹߳��ӽڵ���ӵ��ź����ȴ����� */
	   list_add_tail(&ptcb->pthread_node,&sem->waiters);
	   /* ע��ȴ��¼� */
	   EL_Pthread_EventWait(ptcb,EVENT_TYPE_SEM_WAIT,tick,&ret);
	 
	   OS_Exit_Critical_Check();
	   PORT_PendSV_Suspend();
	   OS_Enter_Critical_Check();
	   if((ret == (int)EL_RESULT_OK) && (sem->value))
		sem->value --;
	   else{
	    tick_now = systick_get();
	    tick = (tick_line < tick_now)?0:(tick_line - tick_now);
	    ret = (int)EL_RESULT_ERR;
	    continue;
	   }
	   OS_Exit_Critical_Check();
	
	   return (EL_RESULT_T)ret;
	  }
	 }
	 else{
	  while(!sem->value)
	  {
	   ASSERT(ptcb->pthread_state != EL_PTHREAD_SUSPEND);

	   /* ����ǰ�߳� */
	   if (NULL == (SuspendObj = (Suspend_t *)kobj_alloc(EL_KOBJ_SUSPEND))){
	    OS_Exit_Critical_Check();
		return EL_RESULT_ERR;
	   }
	   /* ���̷߳���ȴ����� */
	   list_add_tail(&ptcb->pthread_node,&sem->waiters);
	   SuspendObj->Pthread = ptcb;
	   SuspendObj->PendingType = (void *)0;
	   ptcb->block_holder = (void *)SuspendObj;
	   EL_Pthread_PushToSuspendList(ptcb,\
				&SuspendObj->Suspend_Node);

	   OS_Exit_Critical_Check();
	   /* ִ��һ���߳��л� */
	   PORT_PendSV_Suspend();
	   /* ���ָ̻߳����к�Ҫ�Ƚ����ٽ��� */
	   OS_Enter_Critical_Check();
	  }
	  sem->value --;
	  OS_Exit_Critical_Check();

	  return EL_RESULT_OK;
	 }
	}
}

/**********************************************************************
 * �������ƣ� el_sem_trytake
 * ���������� ���Ի�ȡ�ź���
 * ��������� sem : �Ѵ������ź�������
 * ��������� ��
 * �� �� ֵ�� ok/nok
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
EL_RESULT_T el_sem_trytake(el_sem_t *sem)
{
	if( sem == NULL ) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	if(!sem->value){
		OS_Exit_Critical_Check();
		return EL_RESULT_ERR;
	}
	sem->value --;
	OS_Exit_Critical_Check();
	
	return EL_RESULT_OK;
}

/**********************************************************************
 * �������ƣ� el_sem_release
 * ���������� �ͷ��ź���
 * ��������� sem : �Ѵ������ź�������
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
void el_sem_release(el_sem_t *sem)
{
	char need_sched = 0;
	EL_PTCB_T *ptcb,*cur_ptcb = NULL;
	if( sem == NULL ) return;

	OS_Enter_Critical_Check();
	cur_ptcb = EL_GET_CUR_PTHREAD();
	if(sem->value < 0xffffffff) sem->value ++;
	else return;
	/* �Ƿ����̵߳ȴ����ź��� */
	if( !list_empty( &sem->waiters ) ){
	 /* ��ȡ��һ���ȴ����ź������߳� */
	 ptcb = (EL_PTCB_T *)sem->waiters.next;

	 /* �ӵȴ�����ɾ���ȴ��ź������̹߳��ӽڵ� */
	 list_del(&ptcb->pthread_node);
	 /* ���ѵ�һ���ȴ����ź������߳� */
	 EL_Pthread_EventWakeup(ptcb);
	 /* ��������ѵ��߳����ȼ�����ִ��һ���߳��л� */
	 if( cur_ptcb->pthread_prio < ptcb->pthread_prio ) 
	  need_sched = 1;
	}
	OS_Exit_Critical_Check();
	
	if( need_sched )
	 PORT_PendSV_Suspend();
}

/**********************************************************************
 * �������ƣ� el_sem_StatisticsTake
 * ���������� �ź���״̬��Ϣͳ��
 * ��������� sem : �Ѵ������ź�������
			  val��д�����ֵ���׵�ַ
 * ��������� ��
 * �� �� ֵ�� ok/nok
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/06/11	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
EL_RESULT_T el_sem_StatisticsTake(el_sem_t *sem,uint32_t * val)
{
	if( sem == NULL ) return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	if( val != NULL ) *val = sem->value;
	OS_Exit_Critical_Check();
	
	return EL_RESULT_OK;
}