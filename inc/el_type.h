#ifndef EL_TYPE_H
#define EL_TYPE_H
#include "port.h"
#include <stdlib.h>
#include  <stdint.h>
/* 数据类型重定义 */
typedef unsigned char EL_UCHAR;
typedef unsigned short EL_USHORT;
typedef unsigned int EL_UINT;
typedef float EL_FLOAT;

typedef EL_UINT EL_STACK_TYPE;/* 定义栈类型 */
typedef EL_UCHAR EL_PTHREAD_PRIO_TYPE;/* 定义线程优先级 */
typedef uint64_t EL_G_SYSTICK_TYPE;  /* 定义系统时基类型64位 */
typedef uint32_t sys_tick_t;		/* 定义系统时基类型32位 */
typedef char EL_CHAR;
typedef short EL_SHORT;
typedef int EL_INT;
typedef const EL_UCHAR ROM_EL_UCHAR;
typedef enum EL_RESULT{EL_RESULT_OK,EL_RESULT_ERR}EL_RESULT_T;

#endif // !EL_TYPE_H

