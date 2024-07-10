#ifndef EL_MSG_Q_H
#define EL_MSG_Q_H
#include "el_type.h"
#include "el_pthread.h"

/* 若这个宏为1，那么写入的是消息地址，为0写入的是消息 */
#define PUSH_MESSAGE_ADDR_PTR 1

/* define queue struct */
typedef struct stQueue
{
	uint32_t read;			/* 读索引 */
	uint32_t write;			/* 写索引 */
	uint32_t size;			/* 队列长度 */
	uint8_t * pool;			/* 数据池 */
}queue_t;

/* define message queue struct */
typedef struct 
{
	queue_t queue;
	uint16_t item_size;
	uint16_t intem_cnt;
	struct list_head waiters_rd;
	struct list_head waiters_wr;
}msg_q_t;
extern void el_queue_init(queue_t * queue,void * pool,uint32_t size);
extern EL_RESULT_T queue_post(queue_t * queue,void * data,size_t size,size_t * bw);
extern EL_RESULT_T queue_pull(queue_t * queue,void * data,size_t size,size_t * br);

extern __API__ msg_q_t * el_msg_q_create(uint16_t item_size,uint16_t item_cnt);
extern __API__ EL_RESULT_T msg_q_post(msg_q_t * msg_q,void * message,uint32_t timeout_tick);
extern __API__ EL_RESULT_T msg_q_pull(msg_q_t * msg_q,void * message,uint32_t timeout_tick);
#endif