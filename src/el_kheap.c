#include "el_kheap.h"
#include <string.h>

/* 堆内存池对齐方式 */
#if KHEAP_ALIGNMENT_UINT == 16
#define KHEAP_BYTE_ALIGNMENT_MASK   			( 0x000f )
#elif KHEAP_ALIGNMENT_UINT == 8
#define KHEAP_BYTE_ALIGNMENT_MASK   			( 0x0007 )
#elif KHEAP_ALIGNMENT_UINT == 4
#define KHEAP_BYTE_ALIGNMENT_MASK				( 0x0003 )
#endif
/* 根据对齐方式自动对齐 */
#define ALIGNED_UP(_SZ)  						(((_SZ) + KHEAP_ALIGNMENT_UINT-1 ) & (~(uint32_t)KHEAP_BYTE_ALIGNMENT_MASK ))
#define ALIGNED_DOWN(_SZ)   					((_SZ) & (~(uint32_t)KHEAP_BYTE_ALIGNMENT_MASK ))

/* 一块堆的最小管理单元空间 */
#define KHEAP_MNGHEADSZ_MIN 					(ALIGNED_UP(sizeof(hcb_t)))
#define KHEAP_MNGNODEESZ_MIN 					(ALIGNED_UP(sizeof(linknode_t)))
#define KHEAP_MNGSZ_MIN 						(KHEAP_MNGHEADSZ_MIN + KHEAP_MNGNODEESZ_MIN)
#define KHEAP_ALLOC_FIX_HEAD_MIN 				(ALIGNED_UP(sizeof(struct usdinfo)))
/* 最小分配单元 */
#define KHEAP_ALLOCSZ_MIN						( KHEAP_ALLOC_FIX_HEAD_MIN + KHEAP_MNGNODEESZ_MIN )

/* 设置分配方式 */
#define KHEAP_ALLOCPATTERN_SET(phcb,md) 		((phcb)->strat.alloc_pttn = (AllocStrategy_t)md)
/* 加入待分配列表 */
#define KHEAP_ADDTO_FREELIST(phead,pnode) 		(list_add((pnode), (phead)))			/*设置为头节点*/
#define KHEAP_ADDTO_FREELIST_TAIL(phead,pnode) 	(list_add_tail((pnode), (phead)))		/*设置为尾节点*/
#define KHEAP_ADDTO_FREELIST_BTWN(pnew,pprev,pnext)	(__list_add((pnew),(pprev),(pnext)))/*中插*/
/*删除待分配列表*/
#define KHEAP_DEL_FREELIST(pnode)				(list_del((pnode)))
/*待分配列表是否为空*/
#define KHEAP_FREELIST_EMPTY(phead) 			(list_empty((phead)))
/*替换原节点*/
#define KHEAP_REPLACE_NODE(pold,pnew) 			(list_replace((pold),(pnew)))
/*待分配列表长度是否为1（只有一个节点）*/
#define KHEAP_FREENODE_SINGLE(phead) 			((!list_empty((phead)))&&((phead)->next=(phead)->prev))
/* 遍历空闲列表 */
#define KHEAP_TRAVERSAL_FREELIST				list_for_each_safe

/* 堆内存的互斥访问 */
#if HEAPLOCK_TYPE == mutex_lock_t
#define KHEAP_LOCK_INIT(hcb)		 			//EL_Mutex_Lock_Init( &((hcb)->hp_lock), (mutex_lock_attr_t)MUTEX_LOCK_NESTING )
#define KHEAP_LOCK(hcb, state)       			//EL_MutexLock_Take( &((hcb)->hp_lock) )
#define KHEAP_LOCK_WAIT(hcb, state, tick)		1//EL_MutexLock_Take_Wait( &((hcb)->hp_lock),tick )
#define KHEAP_UNLOCK(hcb, state)     			//EL_MutexLock_Release( &((hcb)->hp_lock) )
#else
#define KHEAP_LOCK_INIT(hcb)		 			ELOS_SpinLockInit( &((hcb)->hp_lock) )
#define KHEAP_LOCK(hcb, state)       			ELOS_SpinLock( &((hcb)->hp_lock) )
#define KHEAP_LOCK_WAIT(hcb, state, tick)   	ELOS_SpinLock_Wait( &((hcb)->hp_lock),tick )
#define KHEAP_UNLOCK(hcb, state)  				ELOS_SpinUnlock( &((hcb)->hp_lock) )
#endif

static void * hp_alloc_bf( hcb_t * hcb, uint32_t size );
static void * hp_alloc_ff( hcb_t * hcb, uint32_t size );
static void * hp_alloc_wf( hcb_t * hcb, uint32_t size );

#if KHEAP_NAME_EN == 1
/* 设置堆名 */
void Kheap_name_set( hcb_t * hcb,const char * n)
{
	int state;
	if( n == NULL )  return;

	KHEAP_LOCK(hcb, state);
	memcpy( hcb->heap_name,n,HEAP_NAME_LEN );
	KHEAP_UNLOCK(hcb, state);
}
#endif

/* 堆初始化 */
hcb_t * Kheap_Initialise(void * surf,void * bottom)
{
	hcb_t * hcb = NULL;
	linknode_t * first_node;
	uint32_t alg_surf,alg_bottom;
	if(( surf == NULL ) || ( bottom == NULL ))
		return (hcb_t *)0;

	/* 检查是否满足最小堆空间 */
	alg_surf   = ALIGNED_UP(( uint32_t )surf);
	alg_bottom = ALIGNED_UP(( uint32_t )bottom);
	if( ( alg_bottom - alg_surf) < KHEAP_MNGSZ_MIN )
		return (hcb_t *)0;

	/* 初始化堆控制块 */
	hcb = (hcb_t *)alg_surf;
#if KHEAP_USAGE_STATISTICS_EN == 1
	hcb->usage_int = 0;
	hcb->usage_flo = 0;
#endif
	hcb->start_addr = alg_surf;
	hcb->heap_size  = alg_bottom - alg_surf;
	hcb->valid_size = hcb->heap_size - KHEAP_MNGHEADSZ_MIN;
	hcb->strat.php_alloc[BEST_FIT]  = hp_alloc_bf;
	hcb->strat.php_alloc[FIRST_FIT] = hp_alloc_ff;
	hcb->strat.php_alloc[WORST_FIT] = hp_alloc_wf;
	KHEAP_ALLOCPATTERN_SET(hcb,BEST_FIT);
	/* 初始化堆节点 */
	first_node = (linknode_t *)(alg_surf+KHEAP_MNGHEADSZ_MIN);
	first_node->size = hcb->valid_size;
	INIT_LIST_HEAD(&first_node->hpnode);
	KHEAP_ADDTO_FREELIST(&first_node->hpnode,&hcb->free_entry);
#if KHEAP_NAME_EN == 1
	Kheap_name_set(hcb,"kheap");
#endif
	KHEAP_LOCK_INIT(hcb);

	return hcb;
}

/* 最佳拟合（最小拟合） */
static void * hp_alloc_bf( hcb_t * hcb, uint32_t size )
{
	linknode_t * fit_node = NULL;
	struct list_head *pos,*next;
	uint32_t temp_size = 0xffffffff;
	KHEAP_TRAVERSAL_FREELIST( pos, next, &hcb->free_entry )
	{
	 if(( ((linknode_t *)pos)->size >= size )\
	  && ( temp_size >= ((linknode_t *)pos)->size))
	 {
	  fit_node = (linknode_t *)pos;
	  temp_size = ((linknode_t *)pos)->size;
	  if( temp_size == size ) break;
	 }
	}
	return (void *)fit_node;
}

/* 首次拟合 */
static void * hp_alloc_ff( hcb_t * hcb, uint32_t size )
{
	linknode_t * fit_node = NULL;
	struct list_head *pos,*next;
	KHEAP_TRAVERSAL_FREELIST( pos, next, &hcb->free_entry )
	{
	 if( ((linknode_t *)pos)->size >= size ){
	  fit_node = (linknode_t *)pos;break;
	 }
	}
	return (void *)fit_node;
}

/* 最差拟合（最大拟合） */
static void * hp_alloc_wf( hcb_t * hcb, uint32_t size )
{
	linknode_t * fit_node = NULL;
	struct list_head *pos,*next;
	uint32_t temp_size = 0;
	KHEAP_TRAVERSAL_FREELIST( pos, next, &hcb->free_entry )
	{
	 if(( ((linknode_t *)pos)->size >= size )\
	  && ( temp_size <= ((linknode_t *)pos)->size))
	 {
	  fit_node = (linknode_t *)pos;
	  temp_size = ((linknode_t *)pos)->size;
	  if( temp_size == ((linknode_t *)pos)->size ) break;
	 }
	}
	return (void *)fit_node;
}

/* 从指定的堆中分配内存（阻塞方式） */
void * hp_alloc( hcb_t * hcb, uint32_t size )
{
	char ptn, i;
	uint32_t needsz;
	int leftsz, state;
	linknode_t * fit_node;
	linknode_t * next_node;
	struct list_head bk;
	struct usdinfo * alloc;
	if(( hcb == NULL ) || (!size)) return NULL;
	if(KHEAP_FREELIST_EMPTY(&hcb->free_entry)) return NULL;
	/* 保证所分配的内存是对齐的 */
	needsz = ALIGNED_UP(size);
	/* 记录所分配的内存大小 */
	needsz += KHEAP_ALLOC_FIX_HEAD_MIN;
	/* 是否满足最小分配单元 */
	needsz = (needsz < KHEAP_ALLOCSZ_MIN)?(KHEAP_ALLOCSZ_MIN):(needsz);

	KHEAP_LOCK(hcb, state);

	ptn = (char)hcb->strat.alloc_pttn;
	fit_node = ( linknode_t * )\
	(hcb->strat.php_alloc[ptn](hcb, needsz));
#if KHEAP_ALLOC_PATTERN_AUTO_SWITCH == 1
	/* 如果连最大拟合都匹配失败就返回，否则切换其他拟合方式 */
	if(( fit_node == NULL )&&( ptn == (char)WORST_FIT )){
	 return NULL;
	}else if(( fit_node == NULL )&&( ptn != (char)WORST_FIT )){
	 for( i = 0; i < MAX_PATTERN; i++ ){
	  if( i == (char)ptn ) continue;
	  KHEAP_ALLOCPATTERN_SET(hcb, i);
	  if ((fit_node = (linknode_t *)\
	  hcb->strat.php_alloc[ptn](hcb, needsz)) != NULL)
	   break;
	 }
	 /* 恢复原拟合方式 */
	 KHEAP_ALLOCPATTERN_SET(hcb, ptn);
	}
#endif
	if( fit_node == NULL ) return NULL;
	/* 查看此节点是否由剩余，合并到下一节点 */

	leftsz = fit_node->size - needsz;
	if(leftsz >= KHEAP_MNGNODEESZ_MIN){/*够生成一个节点*/
	 next_node = (linknode_t *)((void *)fit_node + needsz);
	 bk.next = fit_node->hpnode.next;
	 bk.prev = fit_node->hpnode.prev;
	 /* 替换原空闲节点 */
	 KHEAP_REPLACE_NODE(&bk, &next_node->hpnode);
	 next_node->size = leftsz;
	}else needsz = fit_node->size;
	/* 记录已分配的内存块大小 */
	alloc = (struct usdinfo *)fit_node;
	alloc->usdsz = needsz;

	KHEAP_UNLOCK(hcb, state);

	return (void *)(((void *)fit_node)+KHEAP_ALLOC_FIX_HEAD_MIN);
}

/* 阻塞等待分配内存 */
void * hp_alloc_wait( hcb_t * hcb, uint32_t size ,uint32_t tick)
{
	int state;
	void * fit_addr;
	if((hcb == NULL) || (!size)) return NULL;
	if( KHEAP_LOCK_WAIT(hcb, state, tick) ){
	 return NULL;
	}
	fit_addr = hp_alloc(  hcb, size );

	KHEAP_UNLOCK(hcb, state);

	return fit_addr;
}

/* 分配指定大小的内存块并初始化为零 */
void * hp_calloc( hcb_t * hcb, uint32_t size )
{
	void * addr = NULL;
	if(( hcb == NULL ) || (!size)) return NULL;

	addr = hp_alloc( hcb, size );
	if(addr != NULL){
	 memset(addr, 0, size);
	}
	return addr;
}

/* 查找已分配节点的邻居待分配节点 */
static void hp_searchNeighNode(hcb_t * hcb, void * fst_addr,\
								struct list_head ** neigh)
{
	linknode_t * temp;
	struct list_head *pos,*next;
	if(( hcb == NULL ) || ( fst_addr == NULL )) return;
	if(KHEAP_FREELIST_EMPTY(&hcb->free_entry)) return;
	/* 遍历整个空闲列表 */
	KHEAP_TRAVERSAL_FREELIST( pos, next, &hcb->free_entry )
	{
	 if( (uint32_t)pos < (uint32_t)fst_addr )
	 {
	  * neigh = (struct list_head *)pos;
	 }
	 if( (uint32_t)pos > (uint32_t)fst_addr )
	 {
	  * (neigh + 1) = (struct list_head *)pos;
	  break;
	 }
	}
	return ;
}

/* 从指定的堆中释放内存（阻塞方式） */
void hp_free( hcb_t * hcb, void * fst_addr )
{
	int state;
	uint32_t size;
	linknode_t * free_node;
	struct list_head bk;
	struct list_head * neigh_node[2] = { [0] = 0,[1] = 0, };/* 邻居节点 */
	if(( hcb == NULL ) || (fst_addr == NULL)) return;

	KHEAP_LOCK(hcb, state);
	/* 获取需要释放的内存块大小（总） */
	size = *((uint32_t *)(fst_addr - KHEAP_ALLOC_FIX_HEAD_MIN));
	/*被释放的内存块节点指针*/
	free_node = (linknode_t *)fst_addr;
	hp_searchNeighNode(hcb,fst_addr,&neigh_node[0]);

	/* 如果有前后空闲节点 */
	if( neigh_node[0] && neigh_node[1] ){
	 /* 是否能与前空闲节点合并 */
	 if( ((void *)neigh_node[0] + ((linknode_t *)neigh_node[0])->size)\
					== (void *)free_node ) {/*可以合并*/
	  ((linknode_t *)neigh_node[0])->size += size;
	  /*再看是否可以和后空闲节点合并*/
	  if( (((linknode_t *)neigh_node[0])->size + (void *)neigh_node[0])\
			== (void *)neigh_node[1] ){
	   ((linknode_t *)neigh_node[0])->size += ((linknode_t *)neigh_node[1])->size;
	   /*删除后节点*/
	    KHEAP_DEL_FREELIST(neigh_node[1]);
	  }
	 }else{
	  /*是否能与后空闲节点合并*/
	  if( (fst_addr + size) == (void *)neigh_node[1] ){
	   bk.next = neigh_node[1]->next;
	   bk.prev = neigh_node[1]->prev;
	   KHEAP_REPLACE_NODE(&bk, &free_node->hpnode);
	   free_node->size = size + ((linknode_t *)neigh_node[1])->size;
	  }else{
	   free_node->size = size;
	   KHEAP_ADDTO_FREELIST_BTWN(&free_node->hpnode,neigh_node[0],neigh_node[1]);
	  }
	 }
	}else if( neigh_node[0] ){/* 如果只有前空闲节点 */
	/* 是否能与前空闲节点合并 */
	 if( (void *)((void *)neigh_node[0] + ((linknode_t *)neigh_node[0])->size)\
		== (void *)free_node ) {/*可以合并*/
	  ((linknode_t *)neigh_node[0])->size += size;
	 }else{
	   KHEAP_ADDTO_FREELIST_TAIL(&hcb->free_entry,&free_node->hpnode);	/*尾插*/
	 }
	}else if( neigh_node[1] ){/* 如果只有后空闲节点 */
	 /* 是否能与后空闲节点合并 */
	 if( (fst_addr + size) == (void *)neigh_node[1] ){/* 可以合并 */
	  bk.next = neigh_node[1]->next;
	  bk.prev = neigh_node[1]->prev;
	  KHEAP_REPLACE_NODE(&bk, &free_node->hpnode);/*替换原节点*/
	  free_node->size = size + ((linknode_t *)neigh_node[1])->size;
	 }else{/* 不能合并 */
	  KHEAP_ADDTO_FREELIST(&hcb->free_entry,&free_node->hpnode);/* 头插 */
	 }
	}else if( (!neigh_node[0]) && (!neigh_node[1]) ){/* 如果无前后空闲节点 */
	 /* 自成空闲节点 */
	 KHEAP_ADDTO_FREELIST(&hcb->free_entry,&free_node->hpnode);
	 free_node->size = size;
	}
	KHEAP_UNLOCK(hcb, state);
	return;
}

/* 从指定堆中重分配内存 */
void * hp_realloc( hcb_t * hcb, void * fst_addr, uint32_t size )
{
	int _orgsz,state;
	linknode_t * pidx;
	void * new_addr = NULL;
	if(( hcb == NULL ) || ( fst_addr == NULL )) return NULL;
	if( !size ) return (void *)fst_addr;

	KHEAP_LOCK(hcb, state);
	pidx   = ( linknode_t * )fst_addr;
	_orgsz = pidx->size;
  	/* 如果新大小和旧大小相同，无需操作 */
    if (size == _orgsz) {
     KHEAP_UNLOCK(hcb, state);
     return fst_addr;
    }
    new_addr = hp_alloc(hcb, size);
    if (new_addr == NULL) {
     /* 如果分配失败，解锁并返回NULL */
     KHEAP_UNLOCK(hcb, state);
     return NULL;
    }
    /* 复制原始内存块的内容到新的内存块 */
    memcpy(new_addr, fst_addr, (_orgsz < size) ? _orgsz : size);
    /* 释放原始内存块 */
    hp_free(hcb, fst_addr);
    KHEAP_UNLOCK(hcb, state);
	
	return new_addr;
}

#if KHEAP_USAGE_STATISTICS_EN == 1
/* 统计内存使用率 */
static void hp_CalcUsage(hcb_t *hcb)
{
	int state;
	/*总内存大小*/
    uint32_t total_memory = hcb->valid_size;
    uint32_t used_memory = 0;
    struct list_head *pos, *next;
    linknode_t *node;
    if (hcb == NULL) return;

	KHEAP_LOCK(hcb, state);
    /*遍历空闲列表，累加所有空闲块的大小*/
    KHEAP_TRAVERSAL_FREELIST(pos, next, &hcb->free_entry)
	{
     node = list_entry(pos, linknode_t, hpnode);
     used_memory += node->size;
    }
	KHEAP_UNLOCK(hcb, state);

    used_memory = total_memory - used_memory;
	if( used_memory ){
     hcb->usage_int = (used_memory*100)/total_memory;
	 hcb->usage_flo = ((used_memory*100)%total_memory)*100/total_memory;
	}
}
#endif

/* 堆内存状态统计 */
EL_RESULT_T hp_StatisticsTake( hcb_t * hcb, uint32_t *heap_size, uint32_t * start_addr,
								uint8_t * strat, uint8_t * usg1, uint8_t * usg2)
{
	if (hcb == NULL) return EL_RESULT_ERR;

	if(heap_size != NULL)  *heap_size = (( hcb_t * )hcb)->heap_size;
	if(start_addr != NULL) *start_addr = (( hcb_t * )hcb)->start_addr;
	if(strat != NULL)      *strat = (uint8_t)((( hcb_t * )hcb)->strat.alloc_pttn);
#if KHEAP_USAGE_STATISTICS_EN == 1
	hp_CalcUsage( hcb );
	if(usg1 != NULL)  	   *usg1 = (uint8_t)(( hcb_t * )hcb)->usage_int;
	if(usg2 != NULL)  	   *usg2 = (uint8_t)(( hcb_t * )hcb)->usage_flo;
#endif
	return EL_RESULT_OK;
}

