#ifndef EL_AVL_H
#define EL_AVL_H

#include <stdint.h>

#define AVL_DEBUG_PRINT 1

typedef struct stAVLTreeNode avl_node_t;

typedef char (*compare)(void * c_data1,void * c_data2);

typedef struct stAVLTree
{
    avl_node_t * root;
    uint8_t node_off;
    compare vcomp;
    uint32_t count;
}avl_t;

typedef struct stAVLTreeNode
{
    avl_node_t * lchild;
    avl_node_t * rchild;
    avl_node_t * parent;
    uint32_t height;
    uint32_t value;
}avl_node_t;

#ifndef AVL_MAX
#define AVL_MAX(x,y) (((x)>(y))?(x):(y))
#endif

#ifndef _AVL_NODE2CONTAINER
#define _AVL_NODE2CONTAINER(pnode,noff) ((void *)(((size_t)(pnode))-(noff)))
#endif

#ifndef _AVL_CONTAINER2NODE
#define _AVL_CONTAINER2NODE(pcont,noff) ((avl_node_t *)(((size_t)(pcont))+(noff)))
#endif

#define AVL_LCHILD_HEIGHT(node) (((node)->lchild)?(((node)->lchild)->height):0)
#define AVL_RCHILD_HEIGHT(node) (((node)->rchild)?(((node)->rchild)->height):0)
#define AVL_TREE_BLNFCT(node)   (AVL_LCHILD_HEIGHT(node) - AVL_RCHILD_HEIGHT(node))

#define AVL_OFFSETOF(TYPE, MEMBER)   ((size_t) &((TYPE *)0)->MEMBER)
#define AVL_CONTAINER(ptr_mem,type,mem) (type *)(((char *)(ptr_mem)) - AVL_OFFSETOF(type,mem))
#define AVL_NODEOFF_CONTAINER(node,type) AVL_OFFSETOF(type,avl_node_t)

extern void * avl_Initialise(avl_t * tree,compare vcomp,uint8_t node_off);
extern avl_node_t * avl_find_first_node(avl_t * tree);
extern avl_node_t * avl_find_last_node(avl_t * tree);
extern avl_node_t * avl_next_node(avl_node_t * node);
extern avl_node_t * avl_prev_node(avl_node_t * node);
extern avl_node_t * avl_node_add(avl_node_t * node, avl_t * tree);
extern avl_node_t * g_avl_node_add(void * node_cont, avl_t * tree);
extern avl_node_t * avl_node_delete(avl_node_t * node,avl_t * tree);
extern avl_node_t * avl_node_search(avl_node_t * node, avl_t * tree);
extern avl_node_t * g_avl_node_search(void * node_cont, avl_t * tree);
extern void avl_subtree_print(avl_node_t *root);
extern void avl_tree_print(avl_t * tree);
#endif