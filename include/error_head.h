/*
 * error_head.h
 *
 *  Created on: 2016-7-1
 *      Author: Administrator
 *      ����Ĵ����붨��
 */

#ifndef ERROR_HEAD_H_
#define ERROR_HEAD_H_

#ifndef __err_t_defined
typedef int err_t;
#define __err_t_defined
#endif

/*
err_t :
MSB  ----------|----------- LSB
1  	|	ģ���ʶ  |	�����ʶ
1bit| 15b				| 16b


*/

#define MODULE_BITS	15
#define ERROR_BITS	16

#define ERROR_BIT		(1<<(MODULE_BITS + ERROR_BITS))
#define ERROR_BEGIN(_module_id)	((_module_id) << ERROR_BITS)
#define	ERROR_T(_module_error)	(ERROR_BIT | _module_error)

//��ȡ�����ʶ
#define MODULE_ERROR(_error_t) ((_error_t) & ((1 << ERROR_BITS) - 1))
//��ȡģ���ʶ
#define MODULE_ID(_error_t)		(((_error_t) & ~(ERROR_BIT)) >> ERROR_BITS)

#define INVERSE_ERROR(_error_t) ((_error_t) & ( ~(ERROR_BIT)))
#endif /* ERROR_HEAD_H_ */
