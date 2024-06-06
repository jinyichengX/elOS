#include "el_kheap.h"
#include <string.h>

/* ���ڴ�ض��뷽ʽ */
#if KHEAP_ALIGNMENT_UINT == 16
#define KHEAP_BYTE_ALIGNMENT_MASK   			( 0x000f )
#elif KHEAP_ALIGNMENT_UINT == 8
#define KHEAP_BYTE_ALIGNMENT_MASK   			( 0x0007 )
#elif KHEAP_ALIGNMENT_UINT == 4
#define KHEAP_BYTE_ALIGNMENT_MASK				( 0x0003 )
#endif
/* ���ݶ��뷽ʽ�Զ����� */
#define ALIGNED_UP(_SZ)  						(((_SZ) + KHEAP_ALIGNMENT_UINT-1 ) & (~(uint32_t)KHEAP_BYTE_ALIGNMENT_MASK ))
#define ALIGNED_DOWN(_SZ)   					((_SZ) & (~(uint32_t)KHEAP_BYTE_ALIGNMENT_MASK ))

/* һ��ѵ���С����Ԫ�ռ� */
#define KHEAP_MNGHEADSZ_MIN 					(ALIGNED_UP(sizeof(hcb_t)))
#define KHEAP_MNGNODEESZ_MIN 					(ALIGNED_UP(sizeof(linknode_t)))
#define KHEAP_MNGSZ_MIN 						(KHEAP_MNGHEADSZ_MIN + KHEAP_MNGNODEESZ_MIN)
#define KHEAP_ALLOC_FIX_HEAD_MIN 				(ALIGNED_UP(sizeof(struct usdinfo)))
/* ��С���䵥Ԫ */
#define KHEAP_ALLOCSZ_MIN						( KHEAP_MNGNODEESZ_MIN )

/* ���÷��䷽ʽ */
#define KHEAP_ALLOCPATTERN_SET(phcb,md) 		((phcb)->strat.alloc_pttn = (AllocStrategy_t)md)
/* ����������б� */
#define KHEAP_ADDTO_FREELIST(phead,pnode) 		(list_add((pnode), (phead)))			/*����Ϊͷ�ڵ�*/
#define KHEAP_ADDTO_FREELIST_TAIL(phead,pnode) 	(list_add_tail((pnode), (phead)))		/*����Ϊβ�ڵ�*/
#define KHEAP_ADDTO_FREELIST_BTWN(pnew,pprev,pnext)	(__list_add((pnew),(pprev),(pnext)))/*�в�*/
/*ɾ���������б��еĽڵ�*/
#define KHEAP_DEL_FREELIST(pnode)				(list_del((pnode)))
/*�������б��Ƿ�Ϊ��*/
#define KHEAP_FREELIST_EMPTY(phead) 			(list_empty((phead)))
/*�滻ԭ�ڵ�*/
#define KHEAP_REPLACE_NODE(pold,pnew) 			(list_replace((pold),(pnew)))
/*�������б����Ƿ�Ϊ1��ֻ��һ���ڵ㣩*/
#define KHEAP_FREENODE_SINGLE(phead) 			((!list_empty((phead)))&&((phead)->next=(phead)->prev))
/* ���������б� */
#define KHEAP_TRAVERSAL_FREELIST				list_for_each_safe

/* ���ڴ�Ļ������ */
#if HEAPLOCK_TYPE == mutex_lock_t
#define KHEAP_LOCK_INIT(hcb)		 			EL_Mutex_Lock_Init( &((hcb)->hp_lock), (mutex_lock_attr_t)MUTEX_LOCK_NESTING )
#define KHEAP_LOCK(hcb, state)       			EL_MutexLock_Take( &((hcb)->hp_lock) )
#define KHEAP_LOCK_WAIT(hcb, state, tick)		EL_MutexLock_Take_Wait( &((hcb)->hp_lock),tick )
#define KHEAP_UNLOCK(hcb, state)     			EL_MutexLock_Release( &((hcb)->hp_lock) )
#else
#define KHEAP_LOCK_INIT(hcb)		 			ELOS_SpinLockInit( &((hcb)->hp_lock) )
#define KHEAP_LOCK(hcb, state)       			ELOS_SpinLock( &((hcb)->hp_lock) )
#define KHEAP_LOCK_WAIT(hcb, state, tick)    	ELOS_SpinLock_Wait( &((hcb)->hp_lock),tick )
#define KHEAP_UNLOCK(hcb, state)  				ELOS_SpinUnlock( &((hcb)->hp_lock) )
#endif

static void * hp_alloc_bf( hcb_t * hcb, uint32_t size );
static void * hp_alloc_ff( hcb_t * hcb, uint32_t size );
static void * hp_alloc_wf( hcb_t * hcb, uint32_t size );

#if KHEAP_NAME_EN == 1
/**********************************************************************
 * �������ƣ� Kheap_name_set
 * ���������� ���ö���
 * ��������� hcb : �Ѵ����Ķѹ������
             n : ����
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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

/**********************************************************************
 * �������ƣ� Kheap_Initialise
 * ���������� �ѳ�ʼ��
 * ��������� surf : �ڴ���׵�ַ
             bottom : �ڴ��β��ַ
 * ��������� ��
 * �� �� ֵ�� �ڴ�ѿ��ƿ�ָ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
hcb_t * Kheap_Initialise(void * surf,void * bottom)
{
	hcb_t * hcb = NULL;
	linknode_t * first_node;
	uint32_t alg_surf,alg_bottom;
	if(( surf == NULL ) || ( bottom == NULL ))
	 return (hcb_t *)0;
	if( KHEAP_MNGNODEESZ_MIN < KHEAP_ALLOC_FIX_HEAD_MIN)
	 return (hcb_t *)0;
	/* ����Ƿ�������С�ѿռ� */
	alg_surf   = ALIGNED_UP(( uint32_t )surf);
	alg_bottom = ALIGNED_DOWN(( uint32_t )bottom);
	if( ( alg_bottom - alg_surf) < KHEAP_MNGSZ_MIN )
	 return (hcb_t *)0;

	/* ��ʼ���ѿ��ƿ� */
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
	/* ��ʼ���ѽڵ� */
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

/**********************************************************************
 * �������ƣ� hp_alloc_bf
 * ���������� �����ϣ���С��ϣ�
 * ��������� hcb : �Ѵ����Ķѹ������
             size : ��Ҫ������ڴ��С
 * ��������� ��
 * �� �� ֵ�� ��������ڴ��׵�ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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

/**********************************************************************
 * �������ƣ� hp_alloc_ff
 * ���������� �״����
 * ��������� hcb : �Ѵ����Ķѹ������
             size : ��Ҫ������ڴ��С
 * ��������� ��
 * �� �� ֵ�� ��������ڴ��׵�ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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

/**********************************************************************
 * �������ƣ� hp_alloc_wf
 * ���������� �����ϣ������ϣ�
 * ��������� hcb : �Ѵ����Ķѹ������
             size : ��Ҫ������ڴ��С
 * ��������� ��
 * �� �� ֵ�� ��������ڴ��׵�ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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
	  if( temp_size == size ) break;
	 }
	}
	return (void *)fit_node;
}

/**********************************************************************
 * �������ƣ� hp_alloc
 * ���������� ��ָ���Ķ��з����ڴ棨������ʽ��
 * ��������� hcb : �Ѵ����Ķѹ������
             size : ��Ҫ������ڴ��С
 * ��������� ��
 * �� �� ֵ�� ��������ڴ��׵�ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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
	/* ��֤��������ڴ��Ƕ���� */
	needsz = ALIGNED_UP(size);
	/* �Ƿ�������С���䵥Ԫ */
	needsz = (needsz < KHEAP_ALLOCSZ_MIN)?(KHEAP_ALLOCSZ_MIN):(needsz);

	KHEAP_LOCK(hcb, state);

	ptn = (char)hcb->strat.alloc_pttn;
	fit_node = ( linknode_t * )\
	(hcb->strat.php_alloc[ptn](hcb, needsz));
#if KHEAP_ALLOC_PATTERN_AUTO_SWITCH == 1
	/* ����������϶�ƥ��ʧ�ܾͷ��أ������л�������Ϸ�ʽ */
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
	 /* �ָ�ԭ��Ϸ�ʽ */
	 KHEAP_ALLOCPATTERN_SET(hcb, ptn);
	}
#endif
	if( fit_node == NULL ) return NULL;
	/* �鿴�˽ڵ��Ƿ���ʣ�࣬�ϲ�����һ�ڵ� */

	leftsz = fit_node->size - needsz;
	if(leftsz >= KHEAP_MNGNODEESZ_MIN){/*������һ���ڵ�*/
	 next_node = (linknode_t *)((void *)fit_node + needsz);
	 bk.next = fit_node->hpnode.next;
	 bk.prev = fit_node->hpnode.prev;
	 /* �滻ԭ���нڵ� */
	 KHEAP_REPLACE_NODE(&bk, &next_node->hpnode);
	 next_node->size = leftsz;
	}else {
	 KHEAP_DEL_FREELIST( (struct list_head *)fit_node );
	 needsz = fit_node->size;
	}
	/* ��¼�ѷ�����ڴ���С */
	alloc = (struct usdinfo *)fit_node;
	alloc->usdsz = needsz;

	KHEAP_UNLOCK(hcb, state);

	return (void *)(((void *)fit_node)+KHEAP_ALLOC_FIX_HEAD_MIN);
}

/**********************************************************************
 * �������ƣ� hp_alloc_wait
 * ���������� �����ȴ������ڴ�
 * ��������� hcb : �Ѵ����Ķѹ������
             size : ��Ҫ������ڴ��С
			 tick : �ȴ�ʱ��
 * ��������� ��
 * �� �� ֵ�� ��������ڴ��׵�ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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

/**********************************************************************
 * �������ƣ� hp_calloc
 * ���������� �����ȴ������ڴ沢����
 * ��������� hcb : �Ѵ����Ķѹ������
             size : ��Ҫ������ڴ��С
 * ��������� ��
 * �� �� ֵ�� ��������ڴ��׵�ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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

/**********************************************************************
 * �������ƣ� hp_searchNeighNode
 * ���������� �����ѷ���ڵ���ھӴ�����ڵ�
 * ��������� hcb : �Ѵ����Ķѹ������
             fst_addr : ���ͷŵ��ڴ���׵�ַ
			 neigh : ���ڿ����ڴ��ڵ�
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
static void hp_searchNeighNode(hcb_t * hcb, void * fst_addr,\
								struct list_head ** neigh)
{
	linknode_t * temp;
	struct list_head *pos,*next;
	if(( hcb == NULL ) || ( fst_addr == NULL )) return;
	if(KHEAP_FREELIST_EMPTY(&hcb->free_entry)) return;
	/* �������������б� */
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

/**********************************************************************
 * �������ƣ� hp_free
 * ���������� ��ָ���Ķ����ͷ��ڴ�
 * ��������� hcb : �Ѵ����Ķѹ������
             fst_addr : ���ͷŵ��ڴ���׵�ַ
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
void hp_free( hcb_t * hcb, void * fst_addr )
{
	int state;
	uint32_t size;
	linknode_t * free_node;
	struct list_head bk;
	struct list_head * neigh_node[2] = { [0] = 0,[1] = 0, };/* �ھӽڵ� */
	if(( hcb == NULL ) || (fst_addr == NULL)) return;

	KHEAP_LOCK(hcb, state);
	/*���ͷŵ��ڴ��ڵ�ָ��*/
	free_node = (linknode_t *)(fst_addr - KHEAP_ALLOC_FIX_HEAD_MIN);
	/* ��ȡ��Ҫ�ͷŵ��ڴ���С���ܣ� */
	size = *( (uint32_t *)free_node );
	hp_searchNeighNode(hcb,fst_addr,neigh_node);

	/* �����ǰ����нڵ� */
	if( neigh_node[0] && neigh_node[1] ){
	 /* �Ƿ�����ǰ���нڵ�ϲ� */
	 if( ((void *)neigh_node[0] + ((linknode_t *)neigh_node[0])->size)\
					== (void *)free_node ) {/*���Ժϲ�*/
	  ((linknode_t *)neigh_node[0])->size += size;
	  /*�ٿ��Ƿ���Ժͺ���нڵ�ϲ�*/
	  if( (((linknode_t *)neigh_node[0])->size + (void *)neigh_node[0])\
			== (void *)neigh_node[1] ){
	   ((linknode_t *)neigh_node[0])->size += ((linknode_t *)neigh_node[1])->size;
	   /*ɾ����ڵ�*/
	    KHEAP_DEL_FREELIST(neigh_node[1]);
	  }
	 }
	 else{
	  /*�Ƿ��������нڵ�ϲ�*/
	  if( ((void *)free_node + size) == (void *)neigh_node[1] ){
	   bk.next = neigh_node[1]->next;
	   bk.prev = neigh_node[1]->prev;
	   KHEAP_REPLACE_NODE(&bk, &free_node->hpnode);
	   free_node->size = size + ((linknode_t *)neigh_node[1])->size;
	  }
	  else{
	   free_node->size = size;
	   KHEAP_ADDTO_FREELIST_BTWN(&free_node->hpnode,neigh_node[0],neigh_node[1]);
	  }
	 }
	}
	else if( neigh_node[0] ){/* ���ֻ��ǰ���нڵ� */
	/* �Ƿ�����ǰ���нڵ�ϲ� */
	 if( (void *)((void *)neigh_node[0] + ((linknode_t *)neigh_node[0])->size)\
		== (void *)free_node ) {/*���Ժϲ�*/
	  ((linknode_t *)neigh_node[0])->size += size;
	 }
	 else{
	   KHEAP_ADDTO_FREELIST_TAIL(&hcb->free_entry,&free_node->hpnode);	/*β��*/
	   free_node->size = size;
	 }
	}
	else if( neigh_node[1] ){/* ���ֻ�к���нڵ� */
	 /* �Ƿ��������нڵ�ϲ� */
	 if( ((void *)free_node + size) == (void *)neigh_node[1] ){/* ���Ժϲ� */
	  bk.next = neigh_node[1]->next;
	  bk.prev = neigh_node[1]->prev;
	  KHEAP_REPLACE_NODE(&bk, &free_node->hpnode);/*�滻ԭ�ڵ�*/
	  free_node->size = size + ((linknode_t *)neigh_node[1])->size;
	 }
	 else{/* ���ܺϲ� */
	  KHEAP_ADDTO_FREELIST(&hcb->free_entry,&free_node->hpnode);/* ͷ�� */
	  free_node->size = size;
	 }
	}
	else if( (!neigh_node[0]) && (!neigh_node[1]) ){/* �����ǰ����нڵ� */
	 /* �Գɿ��нڵ� */
	 KHEAP_ADDTO_FREELIST(&hcb->free_entry,&free_node->hpnode);
	 free_node->size = size;
	}
	KHEAP_UNLOCK(hcb, state);
	return;
}

/**********************************************************************
 * �������ƣ� hp_realloc
 * ���������� ��ָ�������ط����ڴ�
 * ��������� hcb : �Ѵ����Ķѹ������
             fst_addr : ��Ҫ���ط�����ڴ���׵�ַ
             size : ��Ҫ���·�����ڴ��С
 * ��������� ��
 * �� �� ֵ�� ��������ڴ��׵�ַ
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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
  	/* ����´�С�;ɴ�С��ͬ��������� */
    if (size == _orgsz) {
     KHEAP_UNLOCK(hcb, state);
     return fst_addr;
    }
    new_addr = hp_alloc(hcb, size);
    if (new_addr == NULL) {
     /* �������ʧ�ܣ�����������NULL */
     KHEAP_UNLOCK(hcb, state);
     return NULL;
    }
    /* ����ԭʼ�ڴ������ݵ��µ��ڴ�� */
    memcpy(new_addr, fst_addr, (_orgsz < size) ? _orgsz : size);
    /* �ͷ�ԭʼ�ڴ�� */
    hp_free(hcb, fst_addr);
    KHEAP_UNLOCK(hcb, state);
	
	return new_addr;
}

#if KHEAP_USAGE_STATISTICS_EN == 1
/**********************************************************************
 * �������ƣ� hp_CalcUsage
 * ���������� ͳ���ڴ�ʹ����
 * ��������� hcb : �Ѵ����Ķѹ������
 * ��������� ��
 * �� �� ֵ�� ��
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
static void hp_CalcUsage(hcb_t *hcb)
{
	int state;
	/*���ڴ��С*/
    uint32_t total_memory = hcb->valid_size;
    uint32_t used_memory = 0;
    struct list_head *pos, *next;
    linknode_t *node;
    if (hcb == NULL) return;

	KHEAP_LOCK(hcb, state);
    /*���������б��ۼ����п��п�Ĵ�С*/
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

/**********************************************************************
 * �������ƣ� hp_StatisticsTake
 * ���������� ���ڴ�״̬ͳ��
 * ��������� hcb : �Ѵ����Ķѹ������
             heap_size : �ڴ���ܴ�С
             start_addr : �ڴ����ʼ��ַ
             strat : �ڴ�ѷ������
             usg1 : �ڴ��ʹ���ʣ��ٷֱ��������֣�
			 usg2 : �ڴ��ʹ���ʣ��ٷֱ���λС�����֣�
 * ��������� ��
 * �� �� ֵ�� EL_RESULT_OK/EL_RESULT_ERR
 * �޸�����        �汾��     �޸���	      �޸�����
 * -----------------------------------------------
 * 2024/05/29	    V1.0	  jinyicheng	      ����
 ***********************************************************************/
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
