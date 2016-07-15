/*
 * osAbstraction.h
 *
 *  Created on: 2016-7-4
 *      Author: Administrator
 */

#ifndef OSABSTRACTION_H_
#define OSABSTRACTION_H_
#include "error_head.h"
#include <pthread.h>





#define SYS_ARCH_DECL_PROTECT(lev)
#define SYS_ARCH_PROTECT(lev)	pthread_mutex_lock( &lev );
#define SYS_ARCH_UNPROTECT(lev)	pthread_mutex_unlock( &lev );


#define LWIP_MAX(x , y)  (((x) > (y)) ? (x) : (y))
#define LWIP_MIN(x , y)  (((x) < (y)) ? (x) : (y))










err_t osSetEvent( void *event);
int osSetEventFromIsr( void *event);

#endif /* OSABSTRACTION_H_ */
