#include "el_pthread.h"

extern EL_G_SYSTICK_TYPE g_SysTick;

EL_G_SYSTICK_TYPE systick_get(void)
{
	return g_SysTick;
}