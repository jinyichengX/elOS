#include "el_avl.h"
#include <stdio.h>
#include <stdlib.h>

/* avl树初始化 */
void * avl_Initialise(avl_t * tree,compare vcomp,uint8_t node_off)
{
    if(( tree == NULL ) || ( vcomp == NULL )) 
        return NULL;
    tree->root = NULL;
    tree->node_off = node_off;
    tree->vcomp = vcomp;
}

/* 节点初始化 */
static inline void avl_node_init(avl_node_t * node)
{
    node->height = 1;
    node->lchild = NULL;
    node->rchild = NULL;
    node->parent = NULL;
}

/* 更新树高 */
static inline void avl_node_height_update(avl_node_t * node)
{
    int lheight,rheight;
    if( node == NULL ) return;
    lheight = AVL_LCHILD_HEIGHT(node);
    rheight = AVL_RCHILD_HEIGHT(node);
    node->height = AVL_MAX(lheight,rheight) + 1;
}

/* 第一个节点 */
avl_node_t * avl_find_first_node(avl_t * tree)
{
    avl_node_t * node = tree->root;
    if(node == NULL) return NULL;
    while(node->lchild)
        node = node->lchild;
    return node;
}

/* 最后一个节点 */
avl_node_t * avl_find_last_node(avl_t * tree)
{
    avl_node_t * node = tree->root;
    if(node == NULL) return NULL;
    while(node->rchild)
        node = node->rchild;
    return node;
}

/* 获取后继节点 */
avl_node_t * avl_next_node(avl_node_t * node)
{
    avl_node_t * index = node->rchild;
    /* 没有右孩就返回父节点 */
    if(node->rchild == NULL)
        return node->parent;
    while(index->lchild != NULL)
        index = index->lchild;
    return index;
}

/* 获取前驱节点 */
avl_node_t * avl_prev_node(avl_node_t * node)
{
    avl_node_t * index = node->lchild;
    /* 没有左孩就返回父节点 */
    if(node->lchild == NULL)
        return node->parent;
    while(index->rchild != NULL)
        index = index->rchild;
    return index;
} 

/* 交换节点和它的左孩 */
void avl_node_exchange_with_l(avl_node_t * node, avl_node_t * lchild)
{
    avl_node_t * parent = node->parent;
    lchild->rchild = node;
    lchild->parent = parent;
    node->parent = lchild;
    node->lchild = NULL;
	if(parent) parent->rchild = lchild;
}

/* 交换节点和它的右孩 */                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
void avl_node_exchange_with_r(avl_node_t * node, avl_node_t * rchild)
{
    avl_node_t * parent = node->parent;
    rchild->lchild = node;
    rchild->parent = parent;
    node->parent = rchild;
    node->rchild = NULL;
	if(parent) parent->lchild = rchild;
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     

/* 右旋节点 */
void avl_node_ratate_right(avl_node_t * node,avl_t * tree)
{
    avl_node_t * lchild = node->lchild;
    avl_node_t * parent = node->parent;

    node->lchild = lchild->rchild;
    node->parent = lchild;
    lchild->parent = parent;
    if(lchild->rchild)
        lchild->rchild->parent = node;
    lchild->rchild = node;
    if(parent != NULL){
        if(parent->lchild == node) parent->lchild = lchild;
        else parent->rchild = lchild;       
    }
    else tree->root = lchild;
}

/* 左旋节点 */
void avl_node_ratate_left(avl_node_t * node,avl_t * tree)
{
    avl_node_t * rchild = node->rchild;
    avl_node_t * parent = node->parent;

    node->rchild = rchild->lchild;
    node->parent = rchild;
    rchild->parent = parent;
    if(rchild->lchild)
        rchild->lchild->parent = node;
    rchild->lchild = node;
    if(parent != NULL)
    {
        if(parent->lchild == node) parent->lchild = rchild;
        else parent->rchild = rchild;
    }
    else tree->root = rchild;
}

/* LL/LR左失衡处理 */
void avl_left_unbalan_handle(avl_node_t * subtree,avl_t * tree)
{
    int lh,rh;
    avl_node_t * tree_root = subtree;
    avl_node_t * left = tree_root->lchild;
    lh = AVL_LCHILD_HEIGHT(left);
    rh = AVL_RCHILD_HEIGHT(left);
    if( rh > lh ){
        /* 左旋LR型成LL型 */
        avl_node_ratate_left(left,tree);
        avl_node_height_update(left->lchild);
        avl_node_height_update(left);
    }
    /*右旋LL型*/
    avl_node_ratate_right(tree_root,tree);
    /*调节被旋转节点的右子树的高度*/
    avl_node_height_update(tree_root->rchild);
    avl_node_height_update(tree_root);
}

/* RR/RL右失衡处理 */
void avl_right_unbalan_handle(avl_node_t * subtree,avl_t * tree)
{
    int lh,rh;
    avl_node_t * tree_root = subtree;
    avl_node_t * right = tree_root->rchild;
    lh = AVL_LCHILD_HEIGHT(right);
    rh = AVL_RCHILD_HEIGHT(right);
    if( lh > rh ){
        /* 右旋RL型成RR型 */
        avl_node_ratate_right(right,tree);
        avl_node_height_update(right->rchild);
        avl_node_height_update(right);
    }
    /*左旋RR型*/
    avl_node_ratate_left(tree_root,tree);
    /*调节被旋转节点的左子树的高度*/
    avl_node_height_update(tree_root->lchild);
    avl_node_height_update(tree_root);
}


/* 遍历更新树高 */
void avl_node_post_height_updata(avl_node_t * start)
{
    avl_node_t * index = start;
    if( start == NULL ) return;
    while(index != NULL)
    {
        avl_node_height_update(index);
        index = index->parent;
    }
}

/* 处理失衡 */
avl_node_t * avl_node_post_unbalance(avl_node_t * start,avl_t * tree)
{
    avl_node_t * pos = start;
    int balance_factor = 0;
    while(NULL != pos)
    {
        avl_node_height_update(pos);
        balance_factor = AVL_TREE_BLNFCT(pos);
        if( balance_factor >= 2 )
        {
            /* LL或LR失衡 */
            avl_left_unbalan_handle(pos,tree);
            break;
        }else if( balance_factor <= -2 )
        {
            /* RR或RL失衡 */
            avl_right_unbalan_handle(pos,tree);
            break;
        }
        pos = pos->parent;
    }
    return pos;
}

/* 添加叶子节点 */
avl_node_t * avl_node_add_leaf(avl_node_t * node,avl_node_t * parent,avl_node_t ** pos)
{
    (*pos) = node;
    node->parent = parent;
    node->lchild = node->rchild = NULL;
    node->height = 1;
}

/* 插入节点 */
avl_node_t * avl_node_add(avl_node_t * node, avl_t * tree)
{
    int ret;
    avl_node_t ** pos = &(tree->root);
    avl_node_t * parent = NULL;
    avl_node_t * node_to_bala;
    if(( node == NULL )||( tree == NULL ))
        return NULL;
    avl_node_init(node);
    while(*pos != NULL){
        ret = tree->vcomp((void *)&((*pos)->value),(void *)&(node->value));
        parent = *pos;
        if( ret == -1 ){
            pos = &((*pos)->lchild);
        }else if( ret == 1 ){
            pos = &((*pos)->rchild);
        }else if( ret == 0 ){
            pos = &((*pos)->rchild);
        }
    }
    /* 找到了插入的节点,先插入 */
    avl_node_add_leaf(node,parent,pos);
    /* post后处理,向上平衡最小不平衡子树 */
    node_to_bala = avl_node_post_unbalance(node,tree);
    /* 向上更新树高 */
    avl_node_post_height_updata(node_to_bala);
    return (*pos != NULL)?(*pos):NULL;  
}

/* 查找节点 */
avl_node_t * avl_node_search(avl_node_t * node, avl_t * tree)
{
    int ret;
    avl_node_t * pos = tree->root;
    if(( node == NULL )||( tree == NULL ))
        return NULL;
    while(pos != NULL){
        ret = tree->vcomp((void *)&pos->value,(void *)&node->value);
        if( ret == -1 ){
            pos = pos->lchild;
        }else if( ret == 1 ){
            pos = pos->rchild;
        }else if( ret == 0 ){
            return pos;
        }
    }
    return NULL;  
}

/* 删除节点 */
avl_node_t * avl_node_delete(avl_node_t * node)
{
    if( node == NULL )
        return NULL;
	/*  */
}

#if AVL_DEBUG_PRINT == 1
/* 中序遍历 */
void avl_subtree_print(avl_node_t *root)
{
    if (root == NULL) return;
    avl_subtree_print(root->lchild);
	printf("\r\n");
    printf("%d ", root->value);
	printf("\r\n");
    avl_subtree_print(root->rchild);
}

void avl_tree_print(avl_t * tree)
{
    avl_subtree_print(tree->root);
}
#endif