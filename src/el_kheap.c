#include "el_kheap.h"
#include <string.h>

/* the global heap */
uint8_t sys_heap[SYS_HEAP_SIZE];
hcb_t * sysheap_hcb;

/* heap align size */
#if KHEAP_ALIGNMENT_UINT == 16
#define KHEAP_BYTE_ALIGNMENT_MASK   			( 0x000f )
#elif KHEAP_ALIGNMENT_UINT == 8
#define KHEAP_BYTE_ALIGNMENT_MASK   			( 0x0007 )
#elif KHEAP_ALIGNMENT_UINT == 4
#define KHEAP_BYTE_ALIGNMENT_MASK				( 0x0003 )
#endif

/* align the addr or size up and down */
#define ALIGNED_UP(_SZ)  						(((_SZ) + KHEAP_ALIGNMENT_UINT-1 ) & (~(uint32_t)KHEAP_BYTE_ALIGNMENT_MASK ))
#define ALIGNED_DOWN(_SZ)   					((_SZ) & (~(uint32_t)KHEAP_BYTE_ALIGNMENT_MASK ))

/* some macros */
#define KHEAP_MNGHEADSZ_MIN 					(ALIGNED_UP(sizeof(hcb_t)))
#define KHEAP_MNGNODEESZ_MIN 					(ALIGNED_UP(sizeof(linknode_t)))
#define KHEAP_MNGSZ_MIN 						(KHEAP_MNGHEADSZ_MIN + KHEAP_MNGNODEESZ_MIN)
#define KHEAP_ALLOC_FIX_HEAD_MIN 				(ALIGNED_UP(sizeof(struct usdinfo)))
#define KHEAP_ALLOCSZ_MIN						(KHEAP_MNGNODEESZ_MIN)

/* set allocate pattern */
#define KHEAP_ALLOCPATTERN_SET(phcb,md) 		((phcb)->strat.alloc_pttn = (AllocStrategy_t)md)

/* insert node operations */
#define KHEAP_ADDTO_FREELIST(phead,pnode) 		(list_add((pnode), (phead)))
#define KHEAP_ADDTO_FREELIST_TAIL(phead,pnode) 	(list_add_tail((pnode), (phead)))
#define KHEAP_ADDTO_FREELIST_BTWN(pnew,pprev,pnext)	(__list_add((pnew),(pprev),(pnext)))

/* delate node operation */
#define KHEAP_DEL_FREELIST(pnode)				(list_del((pnode)))

/* if free list empty */
#define KHEAP_FREELIST_EMPTY(phead) 			(list_empty((phead)))

/* repalce node */
#define KHEAP_REPLACE_NODE(pold,pnew) 			(list_replace((pold),(pnew)))

/* if free list has only one node */
#define KHEAP_FREENODE_SINGLE(phead) 			((!list_empty((phead)))&&((phead)->next=(phead)->prev))

/* traversal free list */
#define KHEAP_TRAVERSAL_FREELIST				list_for_each_safe

/* heap lock and unlock operations */
#if HEAPLOCK_TYPE == mutex_lock_t
#define KHEAP_LOCK_INIT(hcb)		 			//EL_Mutex_Lock_Init( &((hcb)->hp_lock), (mutex_lock_attr_t)MUTEX_LOCK_NESTING )
#define KHEAP_LOCK(hcb, state)       			//EL_MutexLock_Take( &((hcb)->hp_lock) )
#define KHEAP_LOCK_WAIT(hcb, state, tick)		tick?0:1//EL_MutexLock_Take_Wait( &((hcb)->hp_lock),tick )
#define KHEAP_UNLOCK(hcb, state)     			//EL_MutexLock_Release( &((hcb)->hp_lock) )
#else
#define KHEAP_LOCK_INIT(hcb)		 			ELOS_SpinLockInit( &((hcb)->hp_lock) )
#define KHEAP_LOCK(hcb, state)       			ELOS_SpinLock( &((hcb)->hp_lock) )
#define KHEAP_LOCK_WAIT(hcb, state, tick)    	ELOS_SpinLock_Wait( &((hcb)->hp_lock),tick )
#define KHEAP_UNLOCK(hcb, state)  				ELOS_SpinUnlock( &((hcb)->hp_lock) )
#endif

#define KHEAP_DBG_WARNING

static void * hp_alloc_bf( hcb_t * hcb, uint32_t size );
static void * hp_alloc_ff( hcb_t * hcb, uint32_t size );
static void * hp_alloc_wf( hcb_t * hcb, uint32_t size );

#if KHEAP_NAME_EN == 1
/* set name of heap */
void Kheap_name_set( hcb_t * hcb,const char * n)
{
	int state;
	if( n == NULL )  return;

	KHEAP_LOCK(hcb, state);
	
	memcpy( hcb->heap_name,n,HEAP_NAME_LEN );
	hcb->heap_name[HEAP_NAME_LEN] = '\0';
	
	KHEAP_UNLOCK(hcb, state);
}
#endif

/* initialise kernel heap memory */
__API__ hcb_t * hp_init(void * surf,void * bottom)
{
	hcb_t * hcb = NULL;
	linknode_t * first_node;
	uint32_t alg_surf,alg_bottom;
	
	if(( surf == NULL ) || ( bottom == NULL ))
	 return (hcb_t *)0;
	if( KHEAP_MNGNODEESZ_MIN < KHEAP_ALLOC_FIX_HEAD_MIN)
	 return (hcb_t *)0;
	
	/* calculate memory size */
	alg_surf   = ALIGNED_UP(( uint32_t )surf);
	alg_bottom = ALIGNED_DOWN(( uint32_t )bottom);
	
	/* check if memory size is enough */
	if( ( alg_bottom - alg_surf) < KHEAP_MNGSZ_MIN )
	 return (hcb_t *)0;

	OS_Enter_Critical_Check();
	
	/* initialise the heap control block */
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
	
	/* initialise the first free node */
	first_node = (linknode_t *)(alg_surf+KHEAP_MNGHEADSZ_MIN);
	first_node->size = hcb->valid_size;
	
	/* initialise the free list */
	INIT_LIST_HEAD(&first_node->hpnode);
	INIT_LIST_HEAD(&hcb->free_entry);
	/* add the first free node */
	KHEAP_ADDTO_FREELIST(&first_node->hpnode,&hcb->free_entry);
#if KHEAP_NAME_EN == 1
	Kheap_name_set(hcb,"kheap");
#endif
	KHEAP_LOCK_INIT(hcb);
	
	OS_Exit_Critical_Check();
	
	return hcb;
}

/* best fit */
static void * hp_alloc_bf( hcb_t * hcb, uint32_t size )
{
	linknode_t * fit_node = NULL;
	struct list_head *pos,*next;
	uint32_t temp_size = 0xffffffff;
	
	KHEAP_TRAVERSAL_FREELIST( pos, next, &hcb->free_entry ){
	 if(( ((linknode_t *)pos)->size >= size )\
	  && ( temp_size >= ((linknode_t *)pos)->size)){
	  fit_node = (linknode_t *)pos;
	  temp_size = ((linknode_t *)pos)->size;
	  if( temp_size == size ) break;
	 }
	}
	return (void *)fit_node;
}

/* first fit */
static void * hp_alloc_ff( hcb_t * hcb, uint32_t size )
{
	linknode_t * fit_node = NULL;
	struct list_head *pos,*next;
	
	KHEAP_TRAVERSAL_FREELIST( pos, next, &hcb->free_entry ){
	 if( ((linknode_t *)pos)->size >= size )
	  fit_node = (linknode_t *)pos;break;
	}
	return (void *)fit_node;
}

/* worst fit */
static void * hp_alloc_wf( hcb_t * hcb, uint32_t size )
{
	linknode_t * fit_node = NULL;
	struct list_head *pos,*next;
	uint32_t temp_size = 0;
	
	KHEAP_TRAVERSAL_FREELIST( pos, next, &hcb->free_entry ){
	 if(( ((linknode_t *)pos)->size >= size )\
	  && ( temp_size <= ((linknode_t *)pos)->size)){
	  fit_node = (linknode_t *)pos;
	  temp_size = ((linknode_t *)pos)->size;
	  if( temp_size == size ) break;
	 }
	}
	return (void *)fit_node;
}

/* allocate block from heap */
__API__ void * hp_alloc( hcb_t * hcb, uint32_t size )
{
	char ptn, idx;
	uint32_t needsz;
	int leftsz, state;
	linknode_t * fit_node, * next_node;
	struct list_head bk;
	struct usdinfo * alloc;
	
	if(( hcb == NULL ) || (!size)) return NULL;
	if(KHEAP_FREELIST_EMPTY(&hcb->free_entry)) return NULL;
	
	/* ensure that memory is aligned */
	needsz = ALIGNED_UP(size);
	needsz += KHEAP_ALLOC_FIX_HEAD_MIN;
	
	/* if need allocated size larger than limited size */
	needsz = (needsz < KHEAP_ALLOCSZ_MIN)?(KHEAP_ALLOCSZ_MIN):(needsz);

	KHEAP_LOCK(hcb, state);
	
	/* alloc suitable block head here */
	ptn = (char)hcb->strat.alloc_pttn;
	fit_node = ( linknode_t * )(hcb->strat.php_alloc[ptn](hcb, needsz));
	
	/* if allocate pattern aotumatic switch */
#if KHEAP_ALLOC_PATTERN_AUTO_SWITCH == 1
	/* 
	 * if worst fit allocate failed return 
	 * but if not,switch to the other allocate pattern 
	 * and try again
	 */
	if((fit_node == NULL)&&(ptn == WORST_FIT))
	 return NULL;
	 
	/* if not use the worst fit pattern */
	else if((fit_node == NULL) && (ptn != WORST_FIT)){
	 for( idx = 0; idx < MAX_PATTERN; idx++ ){
	  if( idx == ptn ) continue;
	  
	  /* switch allocate pattern */
	  KHEAP_ALLOCPATTERN_SET(hcb, idx);
	  
	  /* retry */
	  if ((fit_node = (linknode_t *)hcb->strat.php_alloc[ptn](hcb, needsz)) != NULL) break;
	 }
	 
	 /* resume the old pattern */
	 KHEAP_ALLOCPATTERN_SET(hcb, ptn);
	}
#endif

	if( fit_node == NULL ) return NULL;
	
	/* if matched node has enough left size */
	leftsz = fit_node->size - needsz;
	
	/* left size can generate one extra node */
	if(leftsz >= KHEAP_MNGNODEESZ_MIN){
	 
	 /* generate it */
	 next_node = (linknode_t *)((char *)fit_node + needsz);
	 bk.next = fit_node->hpnode.next;
	 bk.prev = fit_node->hpnode.prev;
		
	 /* extra node replace the old node */
	 KHEAP_REPLACE_NODE(&bk, &next_node->hpnode);
	 next_node->size = leftsz;
	}
	/* left size cannot generate one extra node */
	else{
	 /* delate orignal node,allocate all of the block size */
	 KHEAP_DEL_FREELIST( (struct list_head *)fit_node );
	 needsz = fit_node->size;
	}
	
	/* record the block size */
	alloc = (struct usdinfo *)fit_node;
	alloc->usdsz = needsz;
	
	KHEAP_UNLOCK(hcb, state);
	
	/* return the allocated addr if existing */
	return (void *)(((char *)fit_node)+KHEAP_ALLOC_FIX_HEAD_MIN);
}

/* allocate block from heap aligned */ 
__API__ void * hp_alloc_align(hcb_t * hcb,\
						uint32_t ** unaligned_addr,\
						uint32_t align_size,\
						int n)
{
	uint32_t size;
	void * p = NULL;
	
	if(( hcb == NULL ) || (!align_size) || (n == 0)) return NULL;
	if( 0 != align_size % 4 ) return NULL;
	
	/* some warning here */
	if(unaligned_addr == NULL) KHEAP_DBG_WARNING;
	
	/* calculate needed size */
	size = align_size * (n + 1);
	/* allocate block */
	if(NULL == (p = hp_alloc(hcb,size))) return NULL;
	
	* unaligned_addr = p;
	/* align the addr user need */
	p = (void *)((((size_t)p) + align_size-1) & (~(size_t)(align_size-1)));
	
	return (void *)p;
}

/* allocate block from heap wait */
__API__ void * hp_alloc_wait( hcb_t * hcb, uint32_t size ,uint32_t tick)
{
	int state;void * fit_addr;
	
	if((hcb == NULL) || (!size)) return NULL;
	
	if(KHEAP_LOCK_WAIT(hcb, state, tick)) return NULL;
	
	fit_addr = hp_alloc( hcb, size );

	KHEAP_UNLOCK(hcb, state);

	return (void *)fit_addr;
}

/* allocate and clear block */
__API__ void * hp_calloc( hcb_t * hcb, uint32_t size )
{
	void * addr = NULL;
	
	if(( hcb == NULL ) || (!size)) return NULL;

	if(NULL != (addr = hp_alloc( hcb, size ))) memset(addr, 0, size);
	
	return (void *)addr;
}

/* record neighbour free node of block to free */
static void hp_searchNeighNode(hcb_t * hcb, void * fst_addr,\
								struct list_head ** neigh)
{
	linknode_t * temp;
	struct list_head *pos,*next;
	
	if(( hcb == NULL ) || ( fst_addr == NULL )) return;
	if(KHEAP_FREELIST_EMPTY(&hcb->free_entry)) return;
	
	/* traverse free list */
	KHEAP_TRAVERSAL_FREELIST( pos, next, &hcb->free_entry ){
	 /* record left neighbour node if exsit */
	 if( (uint32_t)pos < (uint32_t)fst_addr )
	  * neigh = (struct list_head *)pos;
	 
	 /* record right neighbour node if exsit*/
	 if( (uint32_t)pos > (uint32_t)fst_addr ){
	  * (neigh + 1) = (struct list_head *)pos;
	  break;
	 }
	}
}

/* free block from heap */
__API__ void hp_free( hcb_t * hcb, void * fst_addr )
{
	int state;
	uint32_t size;
	linknode_t * free_node;
	struct list_head bk;
	struct list_head * neigh_node[2] = { [0] = 0,[1] = 0, };
	if(( hcb == NULL ) || (fst_addr == NULL)) return;

	KHEAP_LOCK(hcb, state);
	
	/* the node to free soon */
	free_node = (linknode_t *)((char *)fst_addr - KHEAP_ALLOC_FIX_HEAD_MIN);
	/* the size to free soon */
	size = *( (uint32_t *)free_node );
	
	/* record neighbour free node of block to free */
	hp_searchNeighNode(hcb,fst_addr,neigh_node);

	/* if prev and next free node exsit */
	if( neigh_node[0] && neigh_node[1] ){
	 if( ((char *)neigh_node[0] + ((linknode_t *)neigh_node[0])->size) == (void *)free_node ){
	  /* if can merge with prev free node */
	  ((linknode_t *)neigh_node[0])->size += size;
		 
	  if( (((linknode_t *)neigh_node[0])->size + (char *)neigh_node[0]) == (void *)neigh_node[1] ){
	   /* if can merge with next free node */
	   ((linknode_t *)neigh_node[0])->size += ((linknode_t *)neigh_node[1])->size;
	   /* delete next free node */
	    KHEAP_DEL_FREELIST(neigh_node[1]);
	  }
	 }
	 /* if can not merge with prev free node */
	 else{
	  if( (void *)((char *)free_node + size) == (void *)neigh_node[1] ){
	   /* if can merge with next free node */
	   bk.next = neigh_node[1]->next; bk.prev = neigh_node[1]->prev;
	   /* replace node */
	   KHEAP_REPLACE_NODE(&bk, &free_node->hpnode);
	   free_node->size = size + ((linknode_t *)neigh_node[1])->size;
	  }
	  /* if can not merge with next free node */
	  else{
	   free_node->size = size;
	   KHEAP_ADDTO_FREELIST_BTWN(&free_node->hpnode,neigh_node[0],neigh_node[1]);
	  }
	 }
	}
	/* if only the prev free node exsit */
	else if( neigh_node[0] ){
	 if( (void *)((char *)neigh_node[0] + ((linknode_t *)neigh_node[0])->size) == (void *)free_node )
	  /* if can merge with prev free node */
	  ((linknode_t *)neigh_node[0])->size += size;
	 else{
	   /* if can not merge with prev free node */
	   KHEAP_ADDTO_FREELIST_TAIL(&hcb->free_entry,&free_node->hpnode);
	   free_node->size = size;
	 }
	}
	/* if only the next free node exsit */
	else if( neigh_node[1] ){
	 if( (void *)((char *)free_node + size) == (void *)neigh_node[1] ){
	  /* if can merge with next free node */
	  bk.next = neigh_node[1]->next; bk.prev = neigh_node[1]->prev;
	  KHEAP_REPLACE_NODE(&bk, &free_node->hpnode);
	  free_node->size = size + ((linknode_t *)neigh_node[1])->size;
	 }
	 else{
	  /* if can not merge with next free node */
	  KHEAP_ADDTO_FREELIST(&hcb->free_entry,&free_node->hpnode);
	  free_node->size = size;
	 }
	}
	/* if prev and next free node do not exsit */
	else if( (!neigh_node[0]) && (!neigh_node[1]) ){
	 /* generate a new free node and insert to free list */
	 KHEAP_ADDTO_FREELIST(&hcb->free_entry,&free_node->hpnode);
	 free_node->size = size;
	}
	
	KHEAP_UNLOCK(hcb, state);
}

/* realloc memory block */
__API__ void * hp_realloc( hcb_t * hcb, void * fst_addr, uint32_t size )
{
	int _orgsz,state;
	void * new_addr = NULL;
	if(( hcb == NULL ) || ( fst_addr == NULL )) return NULL;
	if( !size ) return (void *)fst_addr;

	KHEAP_LOCK(hcb, state);
	
	_orgsz = (( linknode_t * )fst_addr)->size;

  	/* if new size and old size equal */
    if (size == _orgsz) goto _return;
	
    new_addr = hp_alloc(hcb, size);
    if (new_addr == NULL) goto _return;
	
    /* copy old data to new block */
    memcpy(new_addr,fst_addr,(_orgsz<size)?_orgsz:size);
	
    /* free old block */
    hp_free(hcb, fst_addr);
	
_return:
    KHEAP_UNLOCK(hcb, state);
	return (void *)new_addr;
}

#if KHEAP_USAGE_STATISTICS_EN == 1

static void calc_usage(uint32_t used, uint32_t total, char * seg_int, char * seg_flo)
{
	(* seg_int) = (used * 100) / total;
	(* seg_flo) = ((used * 100) % total) * 100 / total;
}

/* calculate usage of heap */
static void hp_CalcUsage(hcb_t *hcb)
{
	int state;
	struct list_head *pos, *next;linknode_t *node;
    uint32_t total_memory = hcb->valid_size;
    uint32_t used_memory = 0;
	
    if (hcb == NULL) return;

	KHEAP_LOCK(hcb,state);
    /* add all of the free blocks size */
    KHEAP_TRAVERSAL_FREELIST(pos,next,&hcb->free_entry){
     node = list_entry(pos, linknode_t, hpnode);
     used_memory += node->size;
    }
	KHEAP_UNLOCK(hcb, state);
	
	/* used memory size */
    used_memory = total_memory - used_memory;
	
	if( used_memory )
	 calc_usage(used_memory,total_memory,&hcb->usage_int,&hcb->usage_flo);
}

#endif

/* take heap statistics */
__API__ EL_RESULT_T hp_StatisticsTake( hcb_t * hcb, uint32_t *heap_size, uint32_t * start_addr,
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
