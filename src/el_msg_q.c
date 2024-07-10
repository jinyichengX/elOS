#include "el_kpool.h"
#include "el_msg_q.h"
#include "el_debug.h"
#include "el_kobj.h"
#include "el_kheap.h"
#include "kparam.h"

#ifndef BUF_MIN
#define BUF_MIN(x, y)   ((x) < (y) ? (x) : (y))
#endif

#ifndef BUF_MAX
#define BUF_MAX(x, y)   ((x) > (y) ? (x) : (y))
#endif

/* memory copy */
static void os_memcpy(uint8_t *_tar,uint8_t *_src,unsigned len)
{
	int i,temp;
	
    if ((! _tar)||(!_src)||(!len))
	 return;
	
	/* calculate aligned unit size */
	temp = len / sizeof(size_t);
	
	/* copy the aligned data firstly */
    for(i = 0; i < temp; i++)
     ((size_t *)_tar)[i] = ((size_t *)_src)[i];
	
	/* then calculate unaligned data size */
    i *= sizeof(size_t);
	
	/* copy the left scattered data */
    for(; i < len; i++) _tar[i] = _src[i];
}

/* init queue */
__API__ void el_queue_init(queue_t * queue,void * pool,uint32_t size)
{
	queue->pool = (uint8_t *)pool;
	queue->size = size;
	queue->read = 0;
	queue->write = 0;
}

/* get left size to write */
static inline uint32_t queue_free_get(queue_t * queue)
{
    uint32_t size, wr, rd;

    wr = queue->write;
    rd = queue->read;
	
    if (wr >= rd) size = queue->size - (wr - rd);
    else size = rd - wr;

    return size - 1;
}

/* get left size to read */
static inline uint32_t queue_full_get(queue_t * queue)
{
    uint32_t size, wr, rd;

    wr = queue->write;
    rd = queue->read;

    if (wr >= rd) size = wr - rd;
    else size = queue->size - (rd - wr);

    return size;
}

/* write data to queue */
__API__ EL_RESULT_T queue_post(queue_t * queue,void * data,size_t size,size_t * bw)
{
    size_t tocopy, free, w_off;
    uint8_t * d = data;

    if ( !queue || !data || !size )
	 return EL_RESULT_ERR;

	OS_Enter_Critical_Check();
	
    /* get empty size of queue */
    if(0 == (free = queue_free_get(queue))){
	 OS_Exit_Critical_Check();
	 return EL_RESULT_ERR;
	}

    size = BUF_MIN(free, size);
    w_off = queue->write;

    /* write the liner part */
    tocopy = BUF_MIN(queue->size-w_off, size);
    os_memcpy((uint8_t *)&queue->pool[w_off],d,tocopy);
	
    w_off += tocopy;
    size -= tocopy;
	
    /* the left over flow size */
    if (size){
	 os_memcpy((uint8_t *)queue->pool,&d[tocopy],size);
	 w_off = size;
    }
	
    /* check the end of ring pool */
    if (w_off >= queue->size) w_off = 0;

    queue->write = w_off;
	
	/* record the written size */
    if (bw != NULL) *bw = tocopy + size;

	OS_Exit_Critical_Check();
	
    return EL_RESULT_OK;
}

/* write all of the data to queue */
EL_RESULT_T queue_post_force(queue_t * queue,void * data,size_t size)
{
	size_t tocopy, w_off;
    uint8_t * d = data;

    if ( !queue || !data || !size )
	 return EL_RESULT_ERR;

	OS_Enter_Critical_Check();
	
    /* get empty size of queue */
    if(size > queue_free_get(queue)){
	 OS_Exit_Critical_Check();
	 return EL_RESULT_ERR;
	}

	queue_post(queue,data,size,NULL);

	OS_Exit_Critical_Check();
	
    return EL_RESULT_OK;
}

/* read data from queue */
__API__ EL_RESULT_T queue_pull(queue_t * queue,void * data,size_t size,size_t * br)
{
    size_t tocopy, full, r_off;
    uint8_t * d = data;

    if (!queue || !data || !size)
	 return EL_RESULT_ERR;
	
	OS_Enter_Critical_Check();
	
    /* get filled size of queue */
    if(0 == (full = queue_full_get(queue))){
	 OS_Exit_Critical_Check();
	 return EL_RESULT_ERR;
	}
	
    size = BUF_MIN(full, size);
    r_off = queue->read;

    /* read the liner part */
    tocopy = BUF_MIN(queue->size - r_off, size);
    os_memcpy(d, (uint8_t *)&queue->pool[r_off], tocopy);
	
    r_off += tocopy;
    size -= tocopy;
	
    /* the left over flow size */
    if (size) {
	 os_memcpy(&d[tocopy], queue->pool, size);
	 r_off = size;
    }
	
    /* check the end of ring pool */
    if (r_off >= queue->size) r_off = 0;

    queue->read = r_off;
	
	/* record the read size */
    if (br != NULL) {
	 *br = tocopy + size;
    }
	
	OS_Exit_Critical_Check();
	
    return EL_RESULT_OK;
}

/* peek the queue */
__API__ EL_RESULT_T queue_peek(queue_t * queue,size_t skip_count,void * data,size_t btp)
{
    size_t full, tocopy, r;
    uint8_t * d = data;

    if (!queue || !data || !btp)
     return EL_RESULT_ERR;

	OS_Enter_Critical_Check();
	
    /* get filled size of queue */
    full = queue_full_get(queue);
    if (skip_count >= full)
     return EL_RESULT_ERR;
	
    r = queue->read;
    r += skip_count;
    full -= skip_count;
	
    if (r >= queue->size) r -= queue->size;
	
    /* skip */
    if(0 == (btp = BUF_MIN(full, btp)))
     return EL_RESULT_ERR;

    /* read the liner part */
    tocopy = BUF_MIN(queue->size - r, btp);
    os_memcpy(d, (uint8_t *)&queue->pool[r], tocopy);
    btp -= tocopy;

    /* the left over flow size */
    if(btp)
	 os_memcpy(&d[tocopy], (uint8_t *)queue->pool, btp);
	
	OS_Exit_Critical_Check();
	
    return tocopy + btp;
}

/* init message queue */
__API__ void el_msg_q_init(msg_q_t * msg_q,void * pool,uint32_t size)
{
	if(( !msg_q)  || ( !pool )) return;
	INIT_LIST_HEAD(&msg_q->waiters_rd);
	INIT_LIST_HEAD(&msg_q->waiters_wr);
	
	el_queue_init(&msg_q->queue, pool, size);
}

#if MESSAGE_QUEUE_OBJ_STATIC

/* create message queue */
__API__ msg_q_t * el_msg_q_create(uint16_t item_size,uint16_t item_cnt)
{
	kobj_info_t kobj;
	msg_q_t * msg_q = NULL;
	void * pool = NULL;
	uint32_t pool_size;
	
	OS_Enter_Critical_Check();
	
	/* check if object pool support message queue */
	if(EL_RESULT_OK != kobj_check(EL_KOBJ_MESSAGE_QUEUE,&kobj))
	 return NULL;
	
	/* allocate one message queue object */
	msg_q = (msg_q_t *)kobj_alloc(kobj.Kobj_type);
	
	OS_Exit_Critical_Check();
	
	if(NULL == msg_q) return NULL;
	
	/* because the queue has some bug,so item_cnt must plus 1 */
	pool_size = item_size * (item_cnt + 1);
	
	/* request for queue data pool from system heap */
	if(NULL == (pool = hp_alloc(sysheap_hcb,pool_size))){
	 /* free message queue object */
	 kobj_free(kobj.Kobj_type,(void *)msg_q);
	 /* reset msg_q */
	 msg_q = (msg_q_t *)0;
	}
	
	/* initialise message queue */
	el_msg_q_init(msg_q, pool ,pool_size);
	msg_q->item_size = item_size;
	msg_q->intem_cnt = item_cnt;
	
	return msg_q;
}

/* destroy a message queue */
__API__ void msg_q_destroy(msg_q_t * msg_q)
{
	
}

#endif

/* post message to queue */
__API__ EL_RESULT_T msg_q_post(msg_q_t * msg_q,void * message,uint32_t timeout_tick)
{
extern EL_G_SYSTICK_TYPE systick_get(void);
extern EL_G_SYSTICK_TYPE systick_passed(EL_G_SYSTICK_TYPE last_tick);

	uint16_t item_size = msg_q->item_size;
	int ret = (int)EL_RESULT_OK;
	char need_sched = 0;
	EL_PTCB_T * cur_ptcb,* ptcb;
	EL_G_SYSTICK_TYPE tick_line = systick_get()+timeout_tick;
	EL_G_SYSTICK_TYPE tick_now;
	
	if( !msg_q || !message )
	 return EL_RESULT_ERR;
	
	cur_ptcb = EL_GET_CUR_PTHREAD();
	
	OS_Enter_Critical_Check();
	
	/* push the point of message addr */
#if PUSH_MESSAGE_ADDR_PTR == 1
	while(EL_RESULT_ERR == queue_post_force(&msg_q->queue,&message,item_size))
#else
	while(EL_RESULT_ERR == queue_post_force(&msg_q->queue,message,item_size))
#endif
	{
	 /* check if time out */
	 if( !timeout_tick ){
	  ret = (int)EL_RESULT_ERR;
	  break;
	 }
		
	 /* pend current thread some tick */
	 if( timeout_tick != _MAX_TICKS_TO_WAIT ){
	  /* add thread node to wait list */
	  list_add_tail(&cur_ptcb->pthread_node,&msg_q->waiters_wr);
	
	  /* regist wait event */
	  EL_Pthread_EventWait(cur_ptcb,EVENT_TYPE_QUEUE_WAIT,timeout_tick,&ret);
	
	  /* exit and enable scheduler */
	  OS_Exit_Critical_Check();
	  PORT_PendSV_Suspend();
	  OS_Enter_Critical_Check();
	
	  /* check if timeout */
	  tick_now = systick_get();
	  timeout_tick = (tick_line < tick_now)?0:(tick_line - tick_now);
	
	  /* go to next loop */
	  continue;
	 }
	 
	 /* add thread node to wait list */
	 list_add_tail(&cur_ptcb->pthread_node,&msg_q->waiters_wr);
	 
	 OS_Exit_Critical_Check();
	 /* suspend current thread */
     EL_Pthread_Suspend(cur_ptcb);
	 OS_Enter_Critical_Check();
	}
	
	/* resume one thread waiting for reading message */
	if((!list_empty( &msg_q->waiters_rd )) && ( EL_RESULT_OK == (EL_RESULT_T)ret )){
	 /* remove thread node */
	 ptcb = (EL_PTCB_T *)msg_q->waiters_rd.next;
	 list_del(&ptcb->pthread_node);
		
	 /* wakeup first thread wait for queue */
	 EL_Pthread_EventWakeup(ptcb);
		
	 /* if thread wakeup prio less than current thread */
	 if( cur_ptcb->pthread_prio < ptcb->pthread_prio ) 
	  need_sched = 1;
	}
	
	OS_Exit_Critical_Check();
	
	return (EL_RESULT_T)ret;
}

/* pull message from queue */
__API__ EL_RESULT_T msg_q_pull(msg_q_t * msg_q,void * message,uint32_t timeout_tick)
{
extern EL_G_SYSTICK_TYPE systick_get(void);
extern EL_G_SYSTICK_TYPE systick_passed(EL_G_SYSTICK_TYPE last_tick);

	uint16_t item_size = msg_q->item_size;
	int ret = (int)EL_RESULT_OK;
	char need_sched = 0;
	EL_PTCB_T * cur_ptcb,* ptcb;
	EL_G_SYSTICK_TYPE tick_line = systick_get()+timeout_tick;
	EL_G_SYSTICK_TYPE tick_now;
	
	if( !msg_q || !message )
	 return EL_RESULT_ERR;
	
	cur_ptcb = EL_GET_CUR_PTHREAD();
	
	OS_Enter_Critical_Check();
	
	/* push the point of message addr */
#if PUSH_MESSAGE_ADDR_PTR == 1
	while(EL_RESULT_ERR == queue_pull(&msg_q->queue,message,item_size,NULL))
#else
	while(EL_RESULT_ERR == queue_pull(&msg_q->queue,message,item_size,NULL))
#endif
	{
	 /* check if time out */
	 if( !timeout_tick ){
	  ret = (int)EL_RESULT_ERR;
	  break;
	 }
		
	 /* pend current thread some tick */
	 if( timeout_tick != _MAX_TICKS_TO_WAIT ){
	  /* add thread node to wait list */
	  list_add_tail(&cur_ptcb->pthread_node,&msg_q->waiters_rd);
	
	  /* regist wait event */
	  EL_Pthread_EventWait(cur_ptcb,EVENT_TYPE_QUEUE_WAIT,timeout_tick,&ret);
	
	  /* exit and enable scheduler */
	  OS_Exit_Critical_Check();
	  PORT_PendSV_Suspend();
	  OS_Enter_Critical_Check();
	
	  /* check if timeout */
	  tick_now = systick_get();
	  timeout_tick = (tick_line < tick_now)?0:(tick_line - tick_now);
	
	  /* go to next loop */
	  continue;
	 }
	 /* add thread node to wait list */
	 list_add_tail(&cur_ptcb->pthread_node,&msg_q->waiters_rd);
	 
	 OS_Exit_Critical_Check();
	 /* suspend current thread */
     EL_Pthread_Suspend(cur_ptcb);
	 OS_Enter_Critical_Check();
	}
	
	/* resume thread waiting for reading message */
	if((!list_empty( &msg_q->waiters_wr )) && ( EL_RESULT_OK == (EL_RESULT_T)ret )){
	 /* remove thread node */
	 ptcb = (EL_PTCB_T *)msg_q->waiters_wr.next;
	 list_del(&ptcb->pthread_node);
		
	 /* wakeup first thread wait for queue */
	 EL_Pthread_EventWakeup(ptcb);
		
	 /* if thread wakeup prio less than current thread */
	 if( cur_ptcb->pthread_prio < ptcb->pthread_prio ) 
	  need_sched = 1;
	}
	
	OS_Exit_Critical_Check();
	
	return (EL_RESULT_T)ret;
}

