#ifndef EL_MEM_H
#define EL_MEM_H

#include "el_kheap.h"
#include "el_slab.h"
#include "el_kpool_static.h"

#define KMEM_TYPE_HEAP 0X0001
#define KMEM_TYPE_SLAB 0X0002
#define KMEM_TYPE_POOL 0X0004

typedef void* (*AllocFunc)(size_t size);
typedef void (*FreeFunc)(void* ptr);
typedef size_t (*GetSizeFunc)(void* ptr); // 如果需要的话

typedef struct {
    const char * name;
    AllocFunc alloc;
    FreeFunc free;
    GetSizeFunc get_size;
} MemManager;

#endif