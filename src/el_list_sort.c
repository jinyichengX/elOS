#include "el_list_sort.h"
#include "el_pthread.h"
#include "el_type.h"

#ifndef _LISTNODE2CONTAINER
#define _LISTNODE2CONTAINER(pnode,off) ((void *)(((size_t)(pnode))-(off)))
#endif
#ifndef _CONTAINER2LISTNODE
#define _CONTAINER2LISTNODE(pcont,off) ((struct list_head *)(((size_t)(pcont))+(off)))
#endif

/* 升序/降序插入列表节点（通用方法） */
void klist_insert_inorder(container_val_compare cmp,\
						  struct list_head *head,\
						  void *node_container,\
						  int node_off)
{
	struct list_head *head_pos = head->next;
	void * cont_idx = _LISTNODE2CONTAINER(head_pos,node_off);
	struct list_head *new = _CONTAINER2LISTNODE(node_container,node_off);;
	while(
		  (!list_empty(head))&&\
     	  (cmp(node_container,cont_idx) == 1)
         )
	{
		head_pos = head_pos->next;
        if(head_pos == head) break;
		cont_idx = _LISTNODE2CONTAINER(head_pos,node_off);
	}
	__list_add(new,head_pos->prev,head_pos);
}

/* 升序插入列表节点（内核定制） */
void EL_Klist_InsertSorted(struct list_head *new,\
						   struct list_head *head)
{
    EL_UINT tick,overtick;
	
    struct list_head *head_pos = head->next;
    /* 获取系统时基 */
    tick = ((TickPending_t *)new)->TickSuspend_Count;
    overtick = ((TickPending_t *)new)->TickSuspend_OverFlowCount;

    while(
		  (!list_empty(head))\
          && ((( tick >= ((TickPending_t *)head_pos)->TickSuspend_Count )&&\
          ( overtick == ((TickPending_t *)head_pos)->TickSuspend_OverFlowCount ))\
          ||( overtick > ((TickPending_t *)head_pos)->TickSuspend_OverFlowCount ))
         )
    {
        head_pos = head_pos->next;
    }
    __list_add(new,head_pos->prev,head_pos);
}

/* 通用方法测试用例 */
typedef struct
{
	int value;
	struct list_head node;
}test_t;
int compare(void * data1,void * data2)
{
    test_t * cont1 = (test_t *)data1;
    test_t * cont2 = (test_t *)data2;
	if(1)
		return (cont1->value) < (cont2->value);//降序排列
	else
		return (cont1->value) < (cont2->value);//升序排列
}
void _list_sort_test(void)
{

	struct list_head list;
	int offset_node = offsetof(test_t, node);
	test_t node1= {.value = 20};
	test_t node2= {.value = 80};
	test_t node3= {.value = 0};
	test_t node4= {.value = 3};
	test_t node5= {.value = 95};
	test_t node6= {.value = 66};
	
	INIT_LIST_HEAD(&list);
    klist_insert_inorder(compare,&list,(void *)&node1,offset_node);
    klist_insert_inorder(compare,&list,(void *)&node2,offset_node);
    klist_insert_inorder(compare,&list,(void *)&node3,offset_node);
    klist_insert_inorder(compare,&list,(void *)&node4,offset_node);
    klist_insert_inorder(compare,&list,(void *)&node5,offset_node);
    klist_insert_inorder(compare,&list,(void *)&node6,offset_node);
}