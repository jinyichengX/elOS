#ifndef EL_LIST_SORT_H
#define EL_LIST_SORT_H

#include "el_klist.h"

typedef int (*container_val_compare)(void *,void *);

extern void klist_insert_inorder(container_val_compare cmp,\
						  struct list_head *head,\
						  void *node_container,\
						  int node_off);
extern void EL_Klist_InsertSorted(struct list_head *new, struct list_head *head);
						  
#endif