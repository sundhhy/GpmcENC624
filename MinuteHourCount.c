/*
 * MinuteHourCount.c
 *
 *  Created on: 2016-7-26
 *      Author: Administrator
 *
 *      实现最近一分钟和最近一小时的数据统计工作.
 *      使用时间桶来实现。
 *      用60个秒时间桶存储过去60秒中每秒的累计值，60个秒时间桶累加就是一分钟的累计值。
 *      用60个分钟时间桶存储过去60分钟中每分钟的累计值， 60个分钟时间桶累加就是一小时的累计值。
 */
#include "MinuteHourCount.h"
#include <stdlib.h>

void COQ_init(void *t, int max_item)
{
	ConveyorQueue *cthis = ( ConveyorQueue *)t;

	cthis->max_items = max_item;
	cthis->total_sum = 0;
	queue_init( &cthis->q, free);

}

void COQ_destroy(void *t)
{
	ConveyorQueue *cthis = ( ConveyorQueue *)t;

	queue_destroy( &cthis->q);		//清空队列
}

uint32_t TotalSum(void *t)
{
	ConveyorQueue *cthis = ( ConveyorQueue *)t;

	return cthis->total_sum;
}

void Shift(void *t, int num_shifted)
{
	int *element;
	ConveyorQueue *cthis = ( ConveyorQueue *)t;

	if( num_shifted >= cthis->max_items)
	{
		queue_destroy( &cthis->q);		//清空队列
		cthis->total_sum = 0;
		return;

	}

	//想队列加入数据
	while( num_shifted > 0)
	{
		element = malloc(sizeof(int));
		*element = 0;
		queue_push( &cthis->q, element);
		num_shifted --;
	}

	while( queue_size( &cthis->q) > cthis->max_items)
	{

		queue_pop( &cthis->q, (void *)&element);
		cthis->total_sum -= *element;
		free( element);
	}
}

void AddToBack( void *t, uint32_t count)
{
	ConveyorQueue *cthis = ( ConveyorQueue *)t;
	int *back_element;

	//确保队列中至少有一个元素
	if( queue_peek( &cthis->q) == NULL)
	{
		cthis->Shift( cthis, 1);
	}

	back_element = queue_back( &cthis->q);

	if( back_element == NULL)
		return ;

	*back_element += count;

	cthis->total_sum += count;

}



void Update( void *t, time_t now)
{
	TrailingBucketCounter *cthis = ( TrailingBucketCounter *)t;
	uint32_t current_bucket = now / cthis->secs_per_bucket;
	uint32_t last_update_bucket = cthis->last_update_time / cthis->secs_per_bucket;

	cthis->buckets->Shift( cthis->buckets, ( current_bucket - last_update_bucket));
	cthis->last_update_time = now;


}

void TBC_init( void *t, int num_buckets, int secs_per_bucket)
{
	TrailingBucketCounter *cthis = ( TrailingBucketCounter *)t;
	cthis->buckets = ConveyorQueue_new();
	cthis->buckets->init( cthis->buckets, num_buckets);
	cthis->secs_per_bucket = secs_per_bucket;



}
void TBC_destroy( void *t)
{
	TrailingBucketCounter *cthis = ( TrailingBucketCounter *)t;
	cthis->buckets->destroy( cthis->buckets);
	lw_oopc_delete( cthis->buckets);
}


void TBC_Add( void *t, uint32_t count, time_t now)
{
	TrailingBucketCounter *cthis = ( TrailingBucketCounter *)t;
	cthis->Update( cthis, now);
	cthis->buckets->AddToBack( cthis->buckets, count);
}

uint32_t TrailingCount(void *t, time_t now)
{
	TrailingBucketCounter *cthis = ( TrailingBucketCounter *)t;
	cthis->Update( cthis, now);

	return cthis->buckets->TotalSum( cthis->buckets);

}

void MHC_init(void*t)
{
	MinuteHourCount *cthis = (MinuteHourCount *)t;

	cthis->minute_counts = TrailingBucketCounter_new();
	cthis->minute_counts->init( cthis->minute_counts, 60, 1);

	cthis->hour_counts = TrailingBucketCounter_new();
	cthis->hour_counts->init( cthis->hour_counts, 60, 60);
}
void MHC_destroy(void*t)
{

	MinuteHourCount *cthis = (MinuteHourCount *)t;
	cthis->minute_counts->destroy( cthis->minute_counts);
	cthis->hour_counts->destroy( cthis->hour_counts);
	lw_oopc_delete( cthis->minute_counts);
	lw_oopc_delete( cthis->hour_counts);
}


void MHC_Add(void *t, uint32_t count)
{
	MinuteHourCount *cthis = (MinuteHourCount *)t;
	time_t now = time(NULL);

	cthis->minute_counts->Add( cthis->minute_counts, count, now);
	cthis->hour_counts->Add( cthis->hour_counts, count, now);
}

uint32_t Minute_count(void *t)
{
	MinuteHourCount *cthis = (MinuteHourCount *)t;
	time_t now = time(NULL);

	return cthis->minute_counts->TrailingCount( cthis->minute_counts, now);
}

uint32_t Hour_count(void *t)
{
	MinuteHourCount *cthis = (MinuteHourCount *)t;
	time_t now = time(NULL);

	return cthis->hour_counts->TrailingCount( cthis->hour_counts, now);
}

CTOR(ConveyorQueue)
FUNCTION_SETTING(init, COQ_init);
FUNCTION_SETTING(destroy, COQ_destroy);
FUNCTION_SETTING(AddToBack, AddToBack);
FUNCTION_SETTING(Shift, Shift);
FUNCTION_SETTING(TotalSum, TotalSum);
END_CTOR

CTOR(TrailingBucketCounter)
FUNCTION_SETTING(init, TBC_init);
FUNCTION_SETTING(destroy, TBC_destroy);
FUNCTION_SETTING(Add, TBC_Add);
FUNCTION_SETTING(TrailingCount, TrailingCount);
FUNCTION_SETTING(Update, Update);
END_CTOR

CTOR(MinuteHourCount)
FUNCTION_SETTING(init, MHC_init);
FUNCTION_SETTING(destroy, MHC_destroy);
FUNCTION_SETTING(Add, MHC_Add);
FUNCTION_SETTING(Minute_Count, Minute_count);
FUNCTION_SETTING(Hour_Count, Hour_count);

END_CTOR
