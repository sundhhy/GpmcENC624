#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/neutrino.h>
#include <errno.h>
#include "net.h"
#include "enc624j600.h"
#include <assert.h>
#include <stdbool.h>
#include <fcntl.h>
#include "debug.h"
#include "memp.h"
#include "pbuf.h"
#include "netif.h"
#include "MinuteHourCount.h"
#include "crc16.h"
#include "ChiticDsNet.h"

#define BILLION  1000000000L;
struct test_data {
	uint32_t   	mark;
	uint32_t	seq;
	uint32_t	mark1;
	time_t		tp;


};
static int TD_seq_offset = ( int )&( ( (struct test_data *)0)->seq);
static int TD_tp_offset = ( int )&( ( (struct test_data *)0)->tp);
static int TD_mark_offset = ( int )&( ( (struct test_data *)0)->mark);
static int TD_mark1_offset = ( int )&( ( (struct test_data *)0)->mark1);
//const  uint8_t enc624_mac[2][6] = { { 0x01,0x01,0x01,0x01,0x01,0x01}, { 0x02,0x02,0x02,0x02,0x02,0x02}};
static void MyHandler( int sig_number );
static void show_debug_info( Sys_deginfo *info);
static int set_nonblock_flag( int desc, int value);
static void netif_link_chg(struct netif *netif);
err_t input_fn(struct pbuf *p, struct netif *inp);
static void *net_test_thread(void *arg);
static void *CDS_recv_thread(void *arg);

static void	linkstate_change_callback( NetAppObj *obj);
static NetInterface	*Inet[2];
static bool	Running = true;
static short		Send_instace = 0;
static pthread_t 	Net_test_tid[2];
static char Link_up[2] = {0};
static int Net_connect_handle[2] = {-1, -1};
static uint32_t	Recv_count[2];
Sys_deginfo		Dubug_info;

static uint8_t		*Send_buf, *Recv_buf, target,Is_Tx = 0;;
static sem_t		sme_linkup[2];
static bool			Net_linkdown[2] = {false};		//发生过断网事件

struct netif	*NetIf[2];

#define	SEND_AT_ONCE		1					///< 两路网络同时发送
#define SEND_MUTUALLY		2					///< 两路网络交替发送
#define NET_SEND_TYPE		SEND_MUTUALLY		///< 两路网络的发送方式
NetAppObj	*CDS_Net[2];
static int 		Ds_Hdl[2];
static uint8_t		DS_SendData = 0;
uint16_t	mtu;
int main(int argc, char *argv[])
{

	uint64_t i = 0, count = 0;
	uint32_t *p_u32;
	char opt;
	uint8_t 	instance;
	err_t ret = 0;
	struct pbuf *p_txbuf;

	if(ThreadCtl(_NTO_TCTL_IO, 0) != EOK)
	{
		printf("You must be root.");
		// We will not return
	}
	printf("Drive of ENC624\n");
	signal(SIGCHLD,SIG_IGN);
	signal(SIGUSR1,MyHandler);
	signal(SIGTERM,MyHandler);

#if NET_SEND_TYPE == SEND_AT_ONCE
	printf(" enter instance 1/2 or 3 for both \r\n");
	instance = getchar() - '0' ;
	assert( instance < 4);

	printf(" enter send instance 1/2 or 3 for both:");
	Send_instace = getchar();
	while( Send_instace > '9' || Send_instace < '0')
	{
		Send_instace = getchar();
	}

	Send_instace -= '0' ;
	printf(" %d is send \r\n", Send_instace);
	pbuf_init();

	for( i = 0; i < 2; i++)
	{
		if( instance & ( 1 << i))
		{
			NetIf[i] = malloc( sizeof( struct netif));
			NetIf[i]->num = i;
			NetIf[i]->input = input_fn;
			NetIf[i]->link_callback = netif_link_chg;
			netif_init( NetIf[i]);
			Inet[i] = NetIf[i]->ll_netif;

			ret = pthread_create ( &Net_test_tid[i], NULL, net_test_thread, NetIf[i]);
			CHECK_ERROR( ret, " pthread_create()");
		}
	}

	set_nonblock_flag( 0, 1);
	while( Running)
	{
		opt = getchar();
		if( opt == 'q')
		{


			break;
		}
		show_debug_info(&Dubug_info);
		count ++;

		printf(" run %04x ... \n", count );
		sleep(1);

	}

	for( i = 0; i < 2; i++)
	{
		if( instance & ( 1 << i))
		{

			pthread_cancel( Net_test_tid[i]);
			pthread_join( Net_test_tid[i], NULL);
			netif_remove( NetIf[i]);
			free( NetIf[i]);
		}
	}

#elif NET_SEND_TYPE == SEND_MUTUALLY



		uint16_t	fromid;

		uint8_t		netaddr;
		uint8_t		*mac;
		char		name[8] = "eth";
		pid_t ds_rxid ;
		uint64_t	send_count[2] = {0};
		uint64_t	send_fail[2] = {0};
		int 	delay_ms = 0;

		struct timespec start_tm, now_tm;
		double accum = 0.0;

		printf("enter a even number netaddr : \n");
		netaddr = getchar();
		while( netaddr > '9' || netaddr < '0' )
		{
			netaddr = getchar();
		}

		netaddr -= '0' ;
		printf("the netaddr is %d \n",netaddr);

		printf("enter target netaddr : \n");
		target = getchar();
		while( target > '9' || target < '0' )
		{
			target = getchar();
		}

		target -= '0' ;
		printf("the target is %d \n",target);

		printf("Tx data (y/n): \n");
		Is_Tx = getchar();
		while( Is_Tx != 'y' && Is_Tx != 'n' )
		{
			Is_Tx = getchar();
		}

		if( Is_Tx == 'y')
		{
			printf("send data \n",target);
			printf("Tx data delay time ms 0 - 9: \n");
			delay_ms = getchar();
			while( delay_ms > '9' || delay_ms < '0' )
			{
				delay_ms = getchar();
			}
			delay_ms -= '0';
			printf("delay %d ms \n", delay_ms);
		}


		sem_init( &sme_linkup[0], 0, 0);
		sem_init( &sme_linkup[1], 0, 0);

		///< 创建并初始化网络协议栈
		CDS_Net[0] = NetAppObj_new();
		CDS_Net[0]->netaddr = netaddr;
		CDS_Net[0]->num = 0;
		sprintf(name,"eth0");
		CDS_Net[0]->set_name( CDS_Net[0], name);
		CDS_Net[0]->linkstate_change_callback =  linkstate_change_callback;
		if( CDS_Net[0]->init(CDS_Net[0]))
			goto err_out_1;
		mac = CDS_Net[0]->get_mac(CDS_Net[0]);
		printf("%s mac :%02x:%02x:%02x:%02x:%02x:%02x  \r\n",CDS_Net[0]->get_name(CDS_Net[0]),
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

		CDS_Net[1] = NetAppObj_new();
		CDS_Net[1]->netaddr = netaddr + 1;
		CDS_Net[1]->num = 1;
		sprintf(name,"eth1");
		CDS_Net[1]->linkstate_change_callback =  linkstate_change_callback;
		CDS_Net[1]->set_name( CDS_Net[1], name);
		if( CDS_Net[1]->init(CDS_Net[1]))
		{
			goto err_out_2;
		}
		mac = CDS_Net[1]->get_mac(CDS_Net[1]);
		printf("%s mac :%02x:%02x:%02x:%02x:%02x:%02x  \r\n",CDS_Net[0]->get_name(CDS_Net[0]),
			mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);


		///< 分配发送和接受内存
		mtu = CDS_Net[0]->get_mtu( CDS_Net[0]);
		Send_buf = malloc(USER_DATA_MAX(mtu));
		if( Send_buf == NULL)
			goto err_out_3;
		Recv_buf = malloc(USER_DATA_MAX(mtu));
		if( Recv_buf == NULL)
			goto err_out_4;

		///< 与对方建立连接

		sem_wait(  &sme_linkup[0]);
		ret = -1;
		while( ret < 0)
		{
			ret = DS_Connect( CDS_Net[0],target );
		}
		if( ret == ERR_TIMEOUT)
		{
			///< 超时错误处理
		}
		if( ret >= 0)
			Ds_Hdl[0] = ret;

		sem_wait(  &sme_linkup[1]);
		ret = -1;
		while( ret < 0)
		{
			ret = DS_Connect( CDS_Net[1], target + 1);
		}
		if( ret == ERR_TIMEOUT)
		{
			///< 超时错误处理
		}
		if( ret >= 0)
			Ds_Hdl[1] = ret;

		memset( Send_buf, 0, USER_DATA_MAX(mtu));
		while( clock_gettime( CLOCK_REALTIME, &start_tm) == -1 )
			  perror( "clock gettime" );




		pthread_create ( &ds_rxid, NULL, CDS_recv_thread,NULL);

		set_nonblock_flag( 0, 1);
		DS_SendData = 0;
		for( i = 0; ; i++)
		{
			opt = getchar();
			memset( Send_buf, DS_SendData, USER_DATA_MAX(mtu));
			if( opt == 'q')
			{


				break;
			}
			if( Is_Tx == 'n')
			{
				sleep(1);
				continue;
			}
			ret = DSSendTo( Ds_Hdl[ i & 1], Send_buf, USER_DATA_MAX(mtu));
			if( ret == ERR_UNAVAILABLE)
			{
				delay(1);
				send_fail[  i & 1] ++;
				continue;
			}
			send_count[ i & 1] ++;

			if( i % 1000 == 0)
			{
				clock_gettime( CLOCK_REALTIME, &now_tm);
				accum = ( now_tm.tv_sec - start_tm.tv_sec )
						 + (double)( now_tm.tv_nsec - now_tm.tv_nsec )
						   / (double)BILLION;
				printf("[%lf]min , send_count: %lld - %lld,send fail count: %lld - %lld \n", \
						accum/60, send_count[0], send_count[1], \
						send_fail[0], send_fail[1]);


			}


			///< 网络发生过断开，就重新连接
			if( Net_linkdown[i & 1])
			{
				DS_DisConnect(Ds_Hdl[ i & 1]);
				sem_wait(  &sme_linkup[i & 1]);
				ret = -1;
				while( ret < 0)
				{
					ret = DS_Connect( CDS_Net[i & 1], target + (i & 1));
				}
				Ds_Hdl[ i & 1] = ret;
				send_count[i & 1] = 0;
				send_fail[i & 1] = 0;
				Net_linkdown[i & 1] = false;

			}
			DS_SendData ++;
			if( delay_ms)
				delay(delay_ms);

		}




		pthread_cancel( ds_rxid);
		pthread_join( ds_rxid, NULL);


		err_out_4:
			free(Send_buf);
		err_out_3:
			CDS_Net[1]->destroy( CDS_Net[1]);
		err_out_2:
			CDS_Net[0]->destroy( CDS_Net[0]);
		err_out_1:
			printf(" err exit !\n");
			return EXIT_FAILURE;





#endif


	printf(" program exit ! \n");

	return EXIT_SUCCESS;
}


static void cleanup(void *arg)
{
//	struct netif *p_netif = (struct netif *)arg;
//
//	NetInterface *inet = ( NetInterface *)p_netif->ll_netif;
//	printf("net test %d thread exit \n", inet->instance );
	int *run = (int *)arg;
	*run = 0;
//	printf(" test thread exit , run %d \n", *run);

}


static void *CDS_recv_thread(void *arg)
{
	int ret = 0;
	uint32_t	recv_count[2] = {0};
	uint32_t	rx_err_count= 0;
	MinuteHourCount *rx_mh_count[2] ;
	double		accum = 0.0;
	uint32_t sec = 0;
	uint16_t	fromid;
	uint8_t		correct_data =0;
	rx_mh_count[0] = MinuteHourCount_new();
	rx_mh_count[1] = MinuteHourCount_new();
	assert( rx_mh_count[0] != NULL);
	assert( rx_mh_count[1] != NULL);

	rx_mh_count[0]->init( rx_mh_count[0]);
	rx_mh_count[1]->init( rx_mh_count[1]);

	struct timespec start_tm, stop_tm;
	while( clock_gettime( CLOCK_REALTIME, &start_tm) == -1 )
		  perror( "clock gettime" );
	while(1)
	{
		ret = DSRecvFrom( Ds_Hdl[ 0] | Ds_Hdl[ 1], Recv_buf, USER_DATA_MAX(mtu), &fromid);
		if( ret == USER_DATA_MAX(mtu))
		{
//				printf("[%d] %s recv %d data, vale is %d \n",i, CDS_Net[ other]->get_name(CDS_Net[ other]), \
						ret, Recv_buf[0]);
			if( fromid == target)
			{

				recv_count[0] ++;

			}
			else if( fromid == target + 1)
			{

				recv_count[1] ++;

			}
			if( Recv_buf[0] != correct_data)
			{
				printf("recv data error: correct_data = %d, recv = %d \n", correct_data, Recv_buf[0]);
			}
			correct_data = Recv_buf[0] + 1;
//			DS_SendData = Recv_buf[0] + 1;

		}
		else
		{
			rx_err_count ++;
		}

		clock_gettime( CLOCK_REALTIME, &stop_tm);
		accum = ( stop_tm.tv_sec - start_tm.tv_sec )
				 + (double)( stop_tm.tv_nsec - start_tm.tv_nsec )
				   / (double)BILLION;

		if( accum > 0.999999)
		{
			rx_mh_count[0]->Add( rx_mh_count[0], recv_count[0] * USER_DATA_MAX(mtu));
			rx_mh_count[1]->Add( rx_mh_count[1], recv_count[1] * USER_DATA_MAX(mtu));
			recv_count[0] = 0;
			recv_count[1] = 0;
			clock_gettime( CLOCK_REALTIME, &start_tm);
			sec ++;
			accum = 0;
			if( sec % 10 == 0)
			{	printf("[%d]s rx err count :d \n", rx_err_count);
				printf("recv 0:1 - %fkB --- %fkB;  %fkB --- %fkB\n",
						rx_mh_count[0]->Minute_Count( rx_mh_count[0])/1024.0, rx_mh_count[0]->Hour_Count( rx_mh_count[0])/1024.0, \
						rx_mh_count[1]->Minute_Count( rx_mh_count[1])/1024.0, rx_mh_count[1]->Hour_Count( rx_mh_count[1])/1024.0);

			}
		}
	}

	return  NULL;
}

#define	TX_DATA_LEN	1500

static void *net_test_thread(void *arg)
{

	struct netif *p_netif = (struct netif *)arg;
	NetInterface *inet = ( NetInterface *)p_netif->ll_netif;
	int instance = inet->instance;
	struct pbuf *p_txbuf;
	struct test_data  *ptr_td;
	struct timespec start_tm, stop_tm;
	time_t *tp;
	int run = 1;

	uint32_t	send_byte = 0;
	uint8_t  	count = 0;
	uint8_t		time_past_s = 0;
	uint16_t	*ptrcrc16 ;

	int 		wait_time = 0;
	double  accum = 0;

	//为发送和接受分别创建一个分钟/小时计数器
	MinuteHourCount *rx_mh_count = MinuteHourCount_new();
	MinuteHourCount *tx_mh_count = MinuteHourCount_new();
	assert( rx_mh_count != NULL);
	assert( tx_mh_count != NULL);
	rx_mh_count->init( rx_mh_count);
	tx_mh_count->init( tx_mh_count);


	//等待网络芯片与外部连接
	while( !inet->linkState)
	{
//		printf("linkstate is down: %s,%s,%d \n", __FILE__, __func__, __LINE__);
		wait_time ++;
		if( wait_time > 100)
		{
			netif_restart( p_netif);
			wait_time = 0;
		}
		delay(100);
	}

	printf("eth%d: net name --- direct ---  last minute total --- last hour count \n", inet->instance);

	//连接到任何一个外部网卡
//	if( Send_instace &( 1 << instance ))
	{
		while( Net_connect_handle[instance] < 0)
		{
			Net_connect_handle[instance] = netif_connect( p_netif, instance);
		}

		printf(" Net_connect_handle[%d] = %d \n", instance, Net_connect_handle[instance]);
	}

	//已经外部网卡建立连接，开始进行数据收发测试



	while( clock_gettime( CLOCK_REALTIME, &start_tm) == -1 )
	  perror( "clock gettime" );
	accum = 0.0;
	Dubug_info.send_count[instance] = 1;
	while(run)
	{
		pthread_cleanup_push(cleanup, &run);
		Dubug_info.enc624_test_thread_run[instance] ++;
		clock_gettime( CLOCK_REALTIME, &stop_tm);
		accum = ( stop_tm.tv_sec - start_tm.tv_sec )
				 + (double)( stop_tm.tv_nsec - start_tm.tv_nsec )
				   / (double)BILLION;

		if( accum > 0.999999)
		{
			send_byte = send_byte * TX_DATA_LEN;
			tx_mh_count->Add( tx_mh_count, send_byte);
			rx_mh_count->Add( rx_mh_count, Recv_count[instance]);
			time_past_s ++;
			send_byte = 0;
			Recv_count[instance] = 0;
			accum = 0;
			if( time_past_s > 10)
			{
					printf(" %s --- send --- %fkB --- %fkB \n", inet->name, \
						tx_mh_count->Minute_Count( tx_mh_count)/1024.0, tx_mh_count->Hour_Count( tx_mh_count)/1024.0);
					printf(" %s --- recv --- %fkB --- %fkB \n", inet->name, \
										rx_mh_count->Minute_Count( rx_mh_count)/1024.0, rx_mh_count->Hour_Count( rx_mh_count)/1024.0);

				time_past_s = 0;

			}
			while( clock_gettime( CLOCK_REALTIME, &start_tm) == -1 )
					  perror( "clock gettime" );

		}

		if( Send_instace &( 1 << instance ))
		{
			p_txbuf = pbuf_alloc( PBUF_LINK, TX_DATA_LEN, PBUF_TX_POOL);
			if( p_txbuf != NULL)
			{
				if( p_txbuf->ref == 0)
				{
					printf("%s in %s error pbuf %p , next = %p, len = %d\n", inet->name,__func__, p_txbuf,p_txbuf->next, p_txbuf->len);
				}
//				tp = ( time_t *)p_txbuf->payload;
//				*tp = PP_HTONL(time(NULL));
				ptr_td = (struct test_data *)p_txbuf->payload;
				ptr_td->mark = PP_HTONL(SEND_DATA);
				ptr_td->mark1 = PP_HTONL(SEND_DATA);

				ptr_td->tp = PP_HTONL(time(NULL));
				ptr_td->seq = PP_HTONL(Dubug_info.send_count[instance]);
				ptrcrc16 = p_txbuf->payload + sizeof( struct test_data);
				*ptrcrc16 = crc_ccitt( p_txbuf->payload, sizeof( struct test_data));


				if( NetIf[instance]->upperlayer_output( NetIf[instance], p_txbuf, Net_connect_handle[instance]) != ERR_OK)
				{
					pbuf_free( p_txbuf);
				}

				count ++;
				send_byte ++;
				Dubug_info.send_count[instance] ++;
			}
			else
			{
//				printf("pbuf alloc fail: %s,%d \n", __func__, __LINE__);
				delay(10);
			}
//			delay(1);
			if( Dubug_info.send_count[instance] % 100 == 0 && Dubug_info.send_count[instance] > 0)
			{
				delay(500);
			}
		}
		else
		{
			delay(20);
		}
		pthread_cleanup_pop(0);
	}

//	printf("net test %d thread exit \n", inet->instance );
//	netif_disconnect( p_netif, Net_connect_handle[instance] );
	rx_mh_count->destroy( rx_mh_count);
	tx_mh_count->destroy( tx_mh_count);
	lw_oopc_delete(rx_mh_count );
	lw_oopc_delete(tx_mh_count );
	pthread_exit(NULL);
}

//
err_t input_fn(struct pbuf *p, struct netif *inp)
{

	static uint32_t    last_recv_seq[2] = {0};
	NetInterface *inet = ( NetInterface *)inp->ll_netif;
	short	i = 0;
	struct test_data  ptr_td;
	short instance = inet->instance;
	uint8_t		*data = (uint8_t *)( p->payload + PBUF_LINK_HLEN);
	uint16_t	*ptrcrc = data + sizeof( struct test_data);
	uint16_t	crc16 = 0;

	memcpy( &ptr_td.mark, data + TD_mark_offset, 4);
	memcpy( &ptr_td.mark1, data + TD_mark1_offset, 4);
	memcpy( &ptr_td.tp, data + TD_tp_offset, sizeof(time_t));

	ptr_td.mark = PP_NTOHL(ptr_td.mark);
	ptr_td.mark1 = PP_NTOHL(ptr_td.mark1);
	if( ptr_td.mark != (SEND_DATA))
	{
		printf("[%d] rx(0x%x, 0x%x) != tx(0x%x) \n",ptr_td.tp, ptr_td.mark, ptr_td.mark1, SEND_DATA);
	}

	memcpy( &ptr_td.seq, data + TD_seq_offset, 4);
	ptr_td.seq = PP_NTOHL(ptr_td.seq);
	crc16 = crc_ccitt( data, sizeof( struct test_data));
	if(crc16 != *ptrcrc )
	{
		printf("[%d] crc error %x!= %x \n",( ptr_td.tp),  crc16, *ptrcrc);
	}
	if( last_recv_seq[instance] + 1 != ptr_td.seq)
	{
		Dubug_info.recv_err_seq[ instance] ++;
		printf("[%d] recv_err_seq %d+1!= %d \n", ( ptr_td.tp),last_recv_seq[instance], ptr_td.seq);

	}
	last_recv_seq[instance] = ptr_td.seq;
	Recv_count[instance] += p->len - PBUF_LINK_HLEN - 4;		//去除以太网的帧头 14B和帧尾 4B





//	printf("%s Recv %d datas  ", inet->name,  p->len);
//
//
//	for( i = 0; i < 24; i ++)
//	{
//		if( i % 16 == 0)
//			printf("\r\n");
//		printf("0x%02x ", data[i]);
//	}
//	printf("\r\n");
	return 0;

}

static void netif_link_chg(struct netif *netif)
{
	Link_up[ netif->num] = ( netif->state & NETIF_FLAG_LINK_UP)?TRUE:FALSE;
	printf(" %s link change %s \n", netif->name,\
			( netif->state & NETIF_FLAG_LINK_UP)?"up":"down");
}

static void MyHandler( int sig_number )
{
	Running = false;
}

static int set_nonblock_flag( int desc, int value)
{
	int oldflags = fcntl( desc, F_GETFL , 0);
	if( oldflags == -1)
		return -1;

	if( value != 0)
		oldflags |= O_NONBLOCK;
	else
		oldflags &= ~O_NONBLOCK;

	return fcntl( desc, F_SETFL , oldflags);
}


static void show_debug_info( Sys_deginfo *info)
{
	static Sys_deginfo	old_ifo;
	static int count = 0;
	int i = 0;

//	if( Dubug_info.linkdown_count[0] != old_ifo.linkdown_count[0] || Dubug_info.linkdown_count[1] != old_ifo.linkdown_count[1] )
//	{
//		old_ifo.linkdown_count[0] = Dubug_info.linkdown_count[0];
//		old_ifo.linkdown_count[1] = Dubug_info.linkdown_count[1];
//		printf("linkdown_count :");
//		printf( "%d,%d \n", Dubug_info.linkdown_count[0], Dubug_info.linkdown_count[1]);
//	}
//	if( Dubug_info.enc624_wait_busy[0] != old_ifo.enc624_wait_busy[0] || Dubug_info.enc624_wait_busy[1] != old_ifo.enc624_wait_busy[1] )
//	{
//		old_ifo.enc624_wait_busy[0] = Dubug_info.enc624_wait_busy[0];
//		old_ifo.enc624_wait_busy[1] = Dubug_info.enc624_wait_busy[1];
//		printf("enc624_wait_busy :");
//		printf( "%d,%d \n", Dubug_info.enc624_wait_busy[0], Dubug_info.enc624_wait_busy[1]);
//	}
//	if( Dubug_info.enc624_test_thread_run[0] != old_ifo.enc624_test_thread_run[0] \
//			|| Dubug_info.enc624_test_thread_run[1] != old_ifo.enc624_test_thread_run[1] )
//	{
//		old_ifo.enc624_test_thread_run[0] = Dubug_info.enc624_test_thread_run[0];
//		old_ifo.enc624_test_thread_run[1] = Dubug_info.enc624_test_thread_run[1];
//		printf("enc624_test_thread_run :");
//		printf( "%d,%d \n", Dubug_info.enc624_test_thread_run[0], Dubug_info.enc624_test_thread_run[1]);
//	}
//	if( Dubug_info.irq_count[0] != old_ifo.irq_count[0] || Dubug_info.irq_count[1] != old_ifo.irq_count[1] )
//	{
//		old_ifo.irq_count[0] = Dubug_info.irq_count[0];
//		old_ifo.irq_count[1] = Dubug_info.irq_count[1];
//		printf("irq_count :");
//		printf( "%d,%d \n", Dubug_info.irq_count[0], Dubug_info.irq_count[1]);
//	}

//	if( Dubug_info.event_count[0] != old_ifo.event_count[0] || Dubug_info.event_count[1] != old_ifo.event_count[1] )
//	{
//		old_ifo.event_count[0] = Dubug_info.event_count[0];
//		old_ifo.event_count[1] = Dubug_info.event_count[1];
//		printf("event count :");
//		printf( "RX_EVENT %d, TX_EVENT %d \n", Dubug_info.event_count[0], Dubug_info.event_count[1]);
//	}

//	if( Dubug_info.irq_handle_count[0] != old_ifo.irq_handle_count[0]\
//			|| Dubug_info.irq_handle_count[1] != old_ifo.irq_handle_count[1] )
//	{
//		old_ifo.irq_handle_count[0] = Dubug_info.irq_handle_count[0];
//		old_ifo.irq_handle_count[1] = Dubug_info.irq_handle_count[1];
//		printf("irq_handle_count count :");
//		printf( "irq_handle_count %d %d \n", Dubug_info.irq_handle_count[0], Dubug_info.irq_handle_count[1]);
//	}

//	if( Dubug_info.EventHandler[0] != old_ifo.EventHandler[0] \
			|| Dubug_info.EventHandler[1] != old_ifo.EventHandler[1] )
//	{
//		old_ifo.EventHandler[0] = Dubug_info.EventHandler[0];
//		old_ifo.EventHandler[1] = Dubug_info.EventHandler[1];
//		printf("EventHandler count :");
//		printf( "EventHandler %d %d \n", Dubug_info.EventHandler[0], Dubug_info.EventHandler[1]);
//	}
//	if( Dubug_info.irq_set_count != old_ifo.irq_set_count)
//	{
//		old_ifo.irq_set_count = Dubug_info.irq_set_count;
//		printf( "irq_set_count %d\n", Dubug_info.irq_set_count);
//
//	}

	if( Dubug_info.send_count[0] != old_ifo.send_count[0]\
			|| Dubug_info.send_count[1] != old_ifo.send_count[1] )
	{
		old_ifo.send_count[0] = Dubug_info.send_count[0];
		old_ifo.send_count[1] = Dubug_info.send_count[1];
		printf("send_count count :");
		printf( "%d , %d \n", Dubug_info.send_count[0], Dubug_info.send_count[1]);
	}
//	if( Dubug_info.send_busy_count[0] != old_ifo.send_busy_count[0]\
//			|| Dubug_info.send_busy_count[1] != old_ifo.send_busy_count[1] )
//	{
//		old_ifo.send_busy_count[0] = Dubug_info.send_busy_count[0];
//		old_ifo.send_busy_count[1] = Dubug_info.send_busy_count[1];
//		printf("send_busy_count count :");
//		printf( "%d , %d \n", Dubug_info.send_busy_count[0], Dubug_info.send_busy_count[1]);
//	}

	if( Dubug_info.recv_count[0] != old_ifo.recv_count[0]\
			|| Dubug_info.recv_count[1] != old_ifo.recv_count[1] )
	{
		old_ifo.recv_count[0] = Dubug_info.recv_count[0];
		old_ifo.recv_count[1] = Dubug_info.recv_count[1];
		printf("recv_count count :");
		printf( "%d , %d \n", Dubug_info.recv_count[0], Dubug_info.recv_count[1]);
	}
//	else
//	{
//		for( i = 0 ; i < 2; i++)
//		{
//			if( Inet[i] != NULL)
//			{
//				enc624j600_print_reg( Inet[i], ENC624J600_REG_ESTAT);
//				enc624j600_print_reg( Inet[i], ENC624J600_REG_EIR);
//				printf("\r\n");
//			}
//		}
//	}

	if( Dubug_info.recv_err_seq[0] != old_ifo.recv_err_seq[0]\
			|| Dubug_info.recv_err_seq[1] != old_ifo.recv_err_seq[1] )
	{
		old_ifo.recv_err_seq[0] = Dubug_info.recv_err_seq[0];
		old_ifo.recv_err_seq[1] = Dubug_info.recv_err_seq[1];
		printf("recv_err_seq: ");
		printf( "%d,%d ", Dubug_info.recv_err_seq[0], Dubug_info.recv_err_seq[1]);
	}

	if( Dubug_info.enc624_recv_abort[0] != old_ifo.enc624_recv_abort[0]\
				|| Dubug_info.enc624_recv_abort[1] != old_ifo.enc624_recv_abort[1] )
	{
		old_ifo.enc624_recv_abort[0] = Dubug_info.enc624_recv_abort[0];
		old_ifo.enc624_recv_abort[1] = Dubug_info.enc624_recv_abort[1];
		printf("enc624_recv_abort: ");
		printf( "%d,%d ", Dubug_info.enc624_recv_abort[0], Dubug_info.enc624_recv_abort[1]);
	}

	printf("\n");
//	for( i = 0 ; i < 2; i++)
//	{
//		if( Inet[i] != NULL)
//		{
//			enc624j600_print_reg( Inet[i], ENC624J600_REG_ESTAT);
//			enc624j600_print_reg( Inet[i], ENC624J600_REG_EIR);
//		}
//	}


}


static void	linkstate_change_callback( NetAppObj *obj)
{
//	printf(" linkstate_change_callback \n");
	bool state = obj->get_linkstate( obj);
	if( state)
		sem_post(  &sme_linkup[ obj->num]);
	else
	{
		Net_linkdown[obj->num] = true;
	}


}

