#ifndef EL_SLAB_H
#define EL_SLAB_H

#include "el_klist.h"
#include "el_type.h"

#define SLAB_CHUNK_MINSZ	64		
#define SLAB_PAGE_SZ 		(4*1024)	
#define SLAB_GROUP_LEVEL 	5 		

/* slabҳ�ķ��䷽ʽ����ҳ������/���з��� */
#define SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE 0/* ��Ϊ0�ǴӶѷ��䣬Ϊ1��ҳ���������䣬slabҳ�ͷŹ���ûд�� */

typedef struct stSlabDispensor slab_disp_t;
typedef struct stSlabContrlBlock slab_ctrl_t;
typedef struct stPageControlHead page_t;

#if SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE
/* ҳ������ */
typedef struct stPageControlHead
{
	page_t * next;
	size_t page_num;
}page_t;
#endif

/* slab������ */
typedef struct stSlabDispensor
{
    slab_disp_t * next;
    void * plist;		
    uint32_t chunk_num;	
    uint32_t chunk_used;
	slab_ctrl_t * owner;
	uint32_t * punaligned;		//�����Ա��ʹ��page������ʱ���Բ���
}slab_disp_t;

/* slab���ƿ� */
typedef struct stSlabContrlBlock
{  
    uint32_t zone_size;			/* һ��slab�����С */
    uint32_t zone_mounted;		/* ���ص�slab������ */
    uint32_t chunk_size;		/* ����obj��С */
	
    slab_disp_t * full;			/* ����������ͷ */
    slab_disp_t * partical;		/* ����������ͷ */
    slab_disp_t * empty;		/* ����������ͷ */
}slab_ctrl_t;
#if SLAB_CACHE_ALLOC_FROM_HEAP_OR_PAGE
extern page_t * sys_page_init(page_t **page,void *surf, void *bottom);
extern void * _slab_pages_alloc(page_t **page, int page_num);
extern void _slab_pages_free(page_t ** page,void * addr,int page_num);
#endif
extern void slab_mem_init(void *surf, int size);
extern slab_disp_t * slab_cache_new(slab_ctrl_t *slab_ctrl);
extern void slab_cache_destroy(slab_disp_t * slab);
extern void * slab_cache_alloc(slab_disp_t * slab);
extern slab_disp_t * slab_cache_free(void * p);
extern void * slab_mem_alloc(slab_ctrl_t *slab_ctrl,uint32_t size);
extern void slab_mem_free(void * p);
extern void slab_blk_remove(slab_disp_t *slab,void * p);
/**************************************************
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
            s l a b ϵ ͳ �� ��
����������������������������       _______________
|slab���ƿ�2   |       | slab������   |
| ��2^n��      |������������>|������������   |��������������>........
_______________      |���ڴ�ռ�     |
                     �������������������������������� 
����������������������������       _______________
|slab���ƿ�2   |       | slab������   |
| ��2^n+1��    |������������>|������������   |��������������>........
_______________      |���ڴ�ռ�     |
                     �������������������������������� 
 * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
��������������������������������������������������������������������������������������������������������
Ϊslab����������һ��zoneʱ����slab���ƿ����Ϊ׼
***************************************************/
#endif