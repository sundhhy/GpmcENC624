/*
 * MinuteHourCount.h
 *
 *  Created on: 2016-7-26
 *      Author: Administrator
 */

#ifndef MINUTEHOURCOUNT_H_
#define MINUTEHOURCOUNT_H_
#include <stdint.h>
#include "error_head.h"
#include "lw_oopc.h"
#include <time.h>
#include "queue.h"

//һ������������Ĳ�,�ɵ�������β������
CLASS(ConveyorQueue)
{
	Queue	q;
	int 	max_items;
	uint32_t total_sum;
	void (*init)(void *t, int max_item);
	void (*destroy)(void*t);


	void (*AddToBack)(void *t,uint32_t count);

	//�����е�ÿ��ֵ��λ'num_shifted'
	//�µ���Ŀ����ʼ����0
	//��ɵ���Ŀ���Ƴ����������<= max_item
	void (*Shift)(void *t, int num_shifted);

	//���ص�ǰ�����е��ܺ�
	uint32_t (*TotalSum)(void *t);
};


CLASS(TrailingBucketCounter)
{
	//privately
	ConveyorQueue *buckets;
	int secs_per_bucket;
	time_t last_update_time;

	void ( *Update)( void *t, time_t now);

	//public
	//���캯�� init( 30, 60) : ׷�ٹ�ȥ30������-Ͱ
	void (*init)( void *t, int num_buckets, int secs_per_bucket);
	void (*destroy)(void*t);


	void (*Add)( void *t, uint32_t count, time_t now);

	uint32_t (*TrailingCount)(void *t, time_t now);
};

CLASS(MinuteHourCount) {


	TrailingBucketCounter  *minute_counts;
	TrailingBucketCounter  *hour_counts;

	//���캯��
	void (*init)(void*t);
	void (*destroy)(void*t);

	void (*Add)(void *t, uint32_t count);
	uint32_t (*Minute_Count)( void *t);
	uint32_t (*Hour_Count)( void *t);

};


#endif /* MINUTEHOURCOUNT_H_ */
