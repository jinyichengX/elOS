#include "el_debug.h"

#define DBG_LEVEL_ERR 		1
#define DBG_LEVEL_WARNING 	2
#define DBG_LEVEL_INFO 		3

#define DEBUG_LEVEL_CONF 	DBG_LEVEL_INFO//ÅäÖÃ´òÓ¡µÈ¼¶

void __attribute__((format(printf,1,2))) dbg_info(char *fmt,...)
{
#if(DEBUG_LEVEL_CONF >= DBG_LEVEL_INFO)
	va_list args;
	__va_start(args,fmt);
	vprintf(fmt,args);
	__va_end(args);
#endif
}

void __attribute__((format(printf,1,2))) dbg_warning(char *fmt,...)
{
#if(DEBUG_LEVEL_CONF >= DBG_LEVEL_INFO)
	va_list args;
	__va_start(args,fmt);
	vprintf(fmt,args);
	__va_end(args);
#endif
}

void __attribute__((format(printf,1,2))) dbg_err(char *fmt,...) 
{
#if(DEBUG_LEVEL_CONF >= DBG_LEVEL_INFO)
	va_list args;
	__va_start(args,fmt);
	vprintf(fmt,args);
	__va_end(args);
#endif
}

