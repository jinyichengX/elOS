#include "el_slab.h"
#include "el_sem.h"
#include "port.h"
#include "el_kheap.h"

#define ALIGNED_DOWN_REGION(ADDR,SEGSZ) ((ADDR) & (~(size_t)((SEGSZ)-1) ))
#define ALIGNED_UP_REGION(ADDR,SEGSZ)  	(((ADDR) + (SEGSZ) ) & (~(size_t)((SEGSZ)-1) ))

/* slab class template */
static int g_SlabPoolSize[SLAB_GROUP_LEVEL] = {
    [ 0 ... SLAB_GROUP_LEVEL-1 ] = SLAB_PAGE_SZ,
};

#if SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE
/* the global page point for slab page management */
page_t * page;/* unused */

/* system page allocator initialise */
page_t * sys_page_init(page_t ** page,void * surf, void * bottom)
{
	size_t page_num;
	size_t aligned_surf, aligned_bottom;
	
	if(( page == NULL )||( !surf )||( !bottom )){
	 * page = NULL;
	 return NULL;
	}

	/* align the surf up and the bottom down */
	aligned_surf = ALIGNED_UP_REGION((size_t)surf,SLAB_PAGE_SZ);
	aligned_bottom = ALIGNED_DOWN_REGION((size_t)bottom,SLAB_PAGE_SZ);
	
	if( aligned_surf >= aligned_bottom ){
	 * page = NULL;
	 return NULL;
	}
	
	/* calculate page number */
	page_num = (aligned_bottom - aligned_surf)/SLAB_PAGE_SZ;
	
	/* get the first page as head */
	* page = (page_t *)aligned_surf;
	
	/* initialise the page list */
	(* page)->next = NULL;
	(* page)->page_num = page_num;
	
	return * page;
}

/* allocate pages from specity page list */
void * _slab_pages_alloc(page_t ** page, int page_num)
{
	page_t **pos,**prev;
	void * page_addr = NULL;
	int lpage = 0;
	if(( page == NULL )||( !page_num ))
	 return NULL;
	
	OS_Enter_Critical_Check();
	for(prev = pos = page; *pos; pos = &((*pos)->next))
	{
	 /* first fit match success */
	 if((*pos)->page_num > page_num)
	 {
	  lpage = (*pos)->page_num - page_num;
	  page_addr = (void *)(*pos);
	
	  /* splite the list */
	  *prev = (page_t * )((char *)page_addr+page_num*SLAB_PAGE_SZ);
	  (*prev)->next = (*pos)->next;
	  (*prev)->page_num = lpage;
		 
	  break;
	 }
	 /* first fit match success */
	 if((*pos)->page_num == page_num)
	 {
	  page_addr = (void *)(*pos);
	
	  /* allocate all pages */
	  *pos = (*pos)->next;
		 
	  break;
	 }
	 /* goto next loop */
	 prev = pos;
	}
	OS_Exit_Critical_Check();
	
	return page_addr;
}

/* free pages to specity page list */
void _slab_pages_free(page_t ** page,void * addr,int page_num)
{
	page_t **pos,**prev;
	if(( page == NULL )||( !addr )||( !page_num ))
	 return;
	
	OS_Enter_Critical_Check();
	for(prev = pos = page; *pos; pos = &((*pos)->next))
	{
		/* empty,need add */
	}
	OS_Exit_Critical_Check();
	
	return;
}
#endif

/* add slab zone to list */
void slab_cache_add_front(slab_disp_t **head,slab_disp_t * slab)
{
	if(* head == NULL){
	 * head = slab;
	 return;
	}
	slab->next = * head;
	* head = slab;
}

/* remove slab zone from list */
void slab_cache_remove(slab_disp_t **head,slab_disp_t * slab)
{
	* head = slab->next;
}

/* add slab chunk to list */
void slab_blk_add_front(slab_disp_t *slab,void * p)
{
	if(NULL == slab->plist)
	{
	 slab->plist = p;
	 return;
	}
	*(size_t *)p = (size_t)slab->plist;
	slab->plist = p;
}

/* remove slab chunk from list */
void slab_blk_remove(slab_disp_t *slab,void * p)
{
	if(NULL == p) return;
	slab->plist = *(char **)p;
}

/* zone list handle */
void slab_cache_add_post(slab_ctrl_t *slab_ctrl,\
					 slab_disp_t *slab)
{
	if(NULL == slab) return;
	
	if(++slab->chunk_used < slab->chunk_num){
		
	 /* chunk used one */
	 if(slab->chunk_used == 1){
	  /* remove from empty list */
	  slab_cache_remove(&slab_ctrl->empty,slab);
		 
	  /* add to partical list */
	  slab_cache_add_front(&slab_ctrl->partical,slab);
	 }
	}
	/* chunks all used */
	else{
	 /* remove from partical list */
	 slab_cache_remove(&slab_ctrl->partical,slab);
		
	 /* add to full list */
	 slab_cache_add_front(&slab_ctrl->full,slab);
	}
}

/* zone list handle */
void slab_cache_remove_post(slab_ctrl_t *slab_ctrl,\
					 slab_disp_t *slab)
{
	if(NULL == slab) return;
	
	if(--slab->chunk_used > 0){
		
	 /* chunk left one */
	 if(slab->chunk_used == slab->chunk_num - 1){
	  /* remove from full list */
	  slab_cache_remove(&slab_ctrl->full,slab);
		 
	  /* add to partical list */
	  slab_cache_add_front(&slab_ctrl->partical,slab);
	 }
	}
	else{
	 /* remove from partical list */
	 slab_cache_remove(&slab_ctrl->partical,slab);
		
	 /* add to empty list */
	 slab_cache_add_front(&slab_ctrl->empty,slab);
	}
}

/* slab control block initialise */
void slab_mem_init(void *surf,int size)
{
	int idx;
    slab_ctrl_t *slab_class;
	
    if(surf == NULL) return;
    if(size < SLAB_GROUP_LEVEL*sizeof(slab_ctrl_t))
     return;

    slab_class = (slab_ctrl_t *)surf;
	
    OS_Enter_Critical_Check();
	
	/* initialise slab class refer to the template array */
    for(idx = 0; idx < SLAB_GROUP_LEVEL; idx++){
	 /* initialise the slab control block*/
	 slab_class->empty = NULL;
	 slab_class->partical = NULL;
	 slab_class->full = NULL;
	 slab_class->zone_mounted = 0;
	 slab_class->chunk_size = SLAB_CHUNK_MINSZ<<idx;
	 slab_class->zone_size = g_SlabPoolSize[idx];
	
	 slab_class++;
    }
    OS_Exit_Critical_Check();
	
    return;
}

/* user request for a chunk from slab cache */
void * slab_mem_alloc(slab_ctrl_t *class_start,uint32_t size)
{
	int idx;
	void * p = NULL;
	slab_ctrl_t * slab_class = class_start;
	slab_disp_t * slab_alloc = NULL;
	
	if( NULL == class_start )
	 return NULL;
	
	OS_Enter_Critical_Check();
	
	/* matching suitable slab dispensor */
	for(idx = 0; idx < SLAB_GROUP_LEVEL; idx++, slab_class++)
	{
	 if(slab_class->chunk_size >= size)
	 {
	  /* slab dispensor matched */
	  if(
		  (slab_class->partical == NULL)&&\
		  (slab_class->empty == NULL)
		)
	   /* if empty list is empty,request for one new slab zone */
	   slab_alloc = slab_cache_new(slab_class);
	  
	  /* allocate preferentially from partical list */
	  /* if not,allocate from empty list */
	  slab_alloc = (NULL != slab_class->partical)?(slab_class->partical):(slab_class->empty);
	  
	  /* allocate from cache */
	  p = slab_cache_alloc(slab_alloc);
	  
	  break;
	 }
	}
	/* zone list post handle */
	slab_cache_add_post(slab_class,slab_alloc);
	
	/* chunk list post handle */
	slab_blk_remove(slab_alloc,p);
	
	OS_Exit_Critical_Check();
	
	return p;
}

/* user free a allocated chunk from slab cache */
void slab_mem_free(void * p)
{
	slab_disp_t * slab;

	OS_Enter_Critical_Check();
	
	/* postion the slab dispensor */
	slab = slab_cache_free(p);
	if( NULL == slab ) return;
	
	/* zone list post hadnle  */
	slab_cache_remove_post(slab->owner,slab);
	
	/* chunk list post handle */
	slab_blk_add_front(slab,p);
	
	OS_Exit_Critical_Check();
}

/* request for a slab zone */
slab_disp_t * slab_cache_new(slab_ctrl_t *slab_ctrl)
{
	uint32_t *p;
    slab_ctrl_t * slab_cb;
    slab_disp_t * slab;
	void * zone_mem;
	int idx;
	uint32_t zone_sz;
	uint32_t chunk_sz;
	
    if(slab_ctrl == NULL) return NULL;
	
	zone_sz = slab_ctrl->zone_size;
	chunk_sz = slab_ctrl->chunk_size;
	
	/* allocate one page from kernel heap */
#if !SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE
	zone_mem = hp_alloc_align(sysheap_hcb,&p,zone_sz,1);
#else
	zone_mem = _slab_pages_alloc(&page,zone_sz/SLAB_PAGE_SZ);
#endif
	if(NULL == zone_mem) return NULL;
	
    OS_Enter_Critical_Check();
	
	/* position slab object to the end of the slab page	*/
	slab = (slab_disp_t *)((char *)zone_mem + zone_sz - sizeof(slab_disp_t));
	
	/* initialise the slab dispensor */
	slab->next = NULL;
	slab->plist = zone_mem;
	slab->chunk_used = 0;
	slab->owner = slab_ctrl;
	slab->punaligned = p;
	slab->chunk_num = (zone_sz - sizeof(slab_disp_t)) / chunk_sz;
	
	/* create slab chunk linkage */
	for(idx = 0;idx<slab->chunk_num-1;idx++){
	 *((size_t *)((char *)zone_mem + idx*chunk_sz)) =\
     (size_t)((char *)zone_mem + (idx+1)*chunk_sz);
	}
	
	/* add slab zone to slab control block */
	slab_cache_add_front(&slab_ctrl->empty,slab);
	
	/* slab control block mounted num add */
	slab_ctrl->zone_mounted ++;
	
    OS_Exit_Critical_Check();
    
    return slab;
}

/* free a slab zone */
void slab_cache_destroy(slab_disp_t * slab)
{
	slab_disp_t **head;
	if(NULL == slab) return;
	
	OS_Enter_Critical_Check();
	
	/* take the slab cache list */
	if(slab->chunk_num == slab->chunk_used)
	 head = &(slab->owner->full);
	else if(slab->chunk_used == 0)
	 head = &(slab->owner->empty);
	else 
	 head = &(slab->owner->partical);
	
	/* remove it from cache list */
	slab_cache_remove(head, slab);
	
	/* slab control block mounted num sub */
	slab->owner->zone_mounted --;
	
	/* free cache memory */
#if !SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE
	hp_free(sysheap_hcb,(void *)slab->punaligned);
#else
	_slab_pages_free(&page,0,0);/* some bugs here,need fix */
#endif
	
	OS_Exit_Critical_Check();
}

/* allocate a chunk from cache */
void * slab_cache_alloc(slab_disp_t * slab)
{
	if(slab == NULL) return NULL;
	return slab->plist;
}

/* free a chunk from cache */
slab_disp_t * slab_cache_free(void * p)
{
	slab_disp_t * slab = NULL;
	void * slab_mem;
	
	if(NULL == p) return NULL;

	/* position the slab dispensor */
	slab_mem = (void *)ALIGNED_DOWN_REGION((size_t)p,SLAB_PAGE_SZ);
	slab = (slab_disp_t *)((char *)slab_mem+SLAB_PAGE_SZ-sizeof(slab_disp_t));
}

/* take statistics of slab cache */
void slab_cache_statistics_take(slab_ctrl_t *slab_ctrl,uint32_t * zs,uint32_t *cs,uint32_t *mtn)
{
	if(NULL == slab_ctrl) return;
	
	OS_Enter_Critical_Check();
	if(NULL != zs) *zs = slab_ctrl->zone_size;
	if(NULL != zs) *cs = slab_ctrl->chunk_size;
	if(NULL != mtn) *zs = slab_ctrl->zone_mounted;
	OS_Exit_Critical_Check();
}

/* ≤‚ ‘”√¿˝ */
void slab_test(void)
{
	char test[200];
	void * p1;
	void * p2;
	void * p3;
	slab_mem_init((void *)test, 200);
	slab_cache_new((slab_ctrl_t *)test+1);
	slab_cache_new((slab_ctrl_t *)test+1);
	p1 = slab_mem_alloc((slab_ctrl_t *)test,1024);
	p2 = slab_mem_alloc((slab_ctrl_t *)test,1024);
	p3 = slab_mem_alloc((slab_ctrl_t *)test,1024);
	slab_mem_free(p3);
	slab_mem_free(p1);
	slab_mem_free(p2);
	p1 = slab_mem_alloc((slab_ctrl_t *)test,1024);
	p2 = slab_mem_alloc((slab_ctrl_t *)test,1024);
	p3 = slab_mem_alloc((slab_ctrl_t *)test,1024);
}