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

//一个有最大数量的槽,旧的数据向尾部滑落
CLASS(ConveyorQueue)
{
	Queue	q;
	int 	max_items;
	uint32_t total_sum;
	void (*init)(void *t, int max_item);
	void (*destroy)(void*t);


	void (*AddToBack)(void *t,uint32_t count);

	//队列中的每个值移位'num_shifted'
	//新的项目被初始化成0
	//最旧的项目被移除，因此总数<= max_item
	void (*Shift)(void *t, int num_shifted);

	//返回当前队列中的总和
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
	//构造函数 init( 30, 60) : 追踪过去30个分钟-桶
	void (*init)( void *t, int num_buckets, int secs_per_bucket);
	void (*destroy)(void*t);


	void (*Add)( void *t, uint32_t count, time_t now);

	uint32_t (*TrailingCount)(void *t, time_t now);
};

CLASS(MinuteHourCount) {


	TrailingBucketCounter  *minute_counts;
	TrailingBucketCounter  *hour_counts;

	//构造函数
	void (*init)(void*t);
	void (*destroy)(void*t);

	void (*Add)(void *t, uint32_t count);
	uint32_t (*Minute_Count)( void *t);
	uint32_t (*Hour_Count)( void *t);

};


#endif /* MINUTEHOURCOUNT_H_ */
