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


static void cleanup(void *arg);
static err_t create_net_interface( int  instance);
static err_t destory_net_interface( int  instance);
static void MyHandler( int sig_number );
static void show_debug_info( Sys_deginfo *info);
static int set_nonblock_flag( int desc, int value);
static void* enc_drive_main(void *arg);

NetInterface	*Inet[2];
bool	Running = true;


Sys_deginfo		Dubug_info;





int main(int argc, char *argv[])
{
	err_t ret = 0;
	uint32_t i = 0, count = 0;
	uint32_t *p_u32;
	char opt;
	uint8_t 	instance;
	NetBuffer		send_buffer;
	struct pbuf *p;
	if(ThreadCtl(_NTO_TCTL_IO, 0) != EOK)
	{
		printf("You must be root.");
		// We will not return
	}
	printf("Drive of ENC624\n");
	printf(" enter instance 1/2 or 3 for both \r\n");
	instance = getchar() - '0' ;
	assert( instance < 4);

	signal(SIGCHLD,SIG_IGN);
	signal(SIGUSR1,MyHandler);
	signal(SIGTERM,MyHandler);


	memp_init();

	for( i = 0; i < 2; i++)
	{
		if( instance & ( 1 << i))
		{

			ret = create_net_interface( i);
			if( ret)
			{
				printf("create_net_interface fail, exit\n");
				return EXIT_FAILURE;
			}


			ret = pthread_create ( &Inet[ i]->tid, NULL, enc_drive_main, Inet[ i]);
			CHECK_ERROR( ret, " pthread_create()");



		}
	}



	printf("press q to exit \n");
	set_nonblock_flag( 0, 1);
	while( Running)
	{



		opt = getchar();
		if( opt == 'q')
		{


			break;
		}


		count ++;

//		show_debug_info( &Dubug_info);
		if( count %40 == 0)
		{

			if( Inet[0]->linkState)
			{
				p = (struct pbuf *)Inet[0]->txbuf;
				send_buffer.data = p->payload;
				memset( send_buffer.data, 0xff, 6);
				memcpy( send_buffer.data + 6, &Inet[0]->macAddr, 6);
				p_u32 = ( uint32_t *)(send_buffer.data + 16);
				*p_u32 = count;

				send_buffer.len = 1514;
				enc624j600Driver.SendPacket( Inet[0], &send_buffer, 0);
			}

			printf(" run %04x ... \n", *p_u32 );
		}

		delay(100);
	}


	for( i = 0; i < 2; i++)
	{
		if( instance & ( 1 << i))
		{

			pthread_cancel( Inet[ i]->tid);
			pthread_join( Inet[ i]->tid, NULL);
		}
	}

	printf(" program exit ! \n");

	return EXIT_SUCCESS;
}


static void* enc_drive_main(void *arg)
{
	NetInterface *inet = ( NetInterface *)arg;
	err_t	ret = 0;
	NetBuffer		send_buffer;
	struct pbuf *p_rxbuf;
	struct pbuf *p_txbuf = pbuf_alloc( PBUF_RAW, ETH_MTU, PBUF_POOL);
	inet->txbuf = (void *)p_txbuf;
	ret = enc624j600Driver.Init( inet);
	if( ret)
		return NULL;

	while(1)
	{
		enc624j600Driver.EnableIrq( inet);
		pthread_cleanup_push(cleanup, (void *)inet);

		InterruptWait_r(NULL, NULL);
		enc624j600Driver.DisableIrq( inet);
//		printf(" isr happened \n");
		if( inet->isr_status & ISR_RX_EVENT)
		{
			atomic_clr( &inet->isr_status, ISR_RX_EVENT);
			p_rxbuf = pbuf_alloc( PBUF_RAW, ETH_MTU, PBUF_POOL);
			if( p_rxbuf != NULL)
			{
				delay(1);
				inet->ethFrame = p_rxbuf->payload;
				enc624j600Driver.EventHandler( inet);


				send_buffer.data = p_rxbuf->payload;
				send_buffer.len = p_rxbuf->len;

				enc624j600Driver.SendPacket( inet, &send_buffer, 0);
				pbuf_free( p_rxbuf);

			}
			else
			{
				TRACE_INFO("pbuf_alloc fail \r\n");

			}


		}
		if(  inet->isr_status & ISR_TX_EVENT)
		{
			atomic_clr( &inet->isr_status, ISR_TX_EVENT);
			enc624j600Driver.EventHandler( inet);

		}
		if(  inet->isr_status & ISR_LINK_STATUS_CHG)
		{
			atomic_clr( &inet->isr_status, ISR_LINK_STATUS_CHG);
			enc624j600Driver.EventHandler( inet);



		}
		if(  inet->isr_status & ISR_ERROR)
		{
			atomic_clr( &inet->isr_status, ISR_ERROR);
			printf(" ISR_ERROR happened \n");
		}

		pthread_cleanup_pop(0);
	}





}

static err_t create_net_interface( int  instance)
{
	Inet[ instance] = malloc( sizeof(NetInterface));
	if( Inet[ instance] == NULL)
		goto err1;


	memset( Inet[ instance], 0, sizeof(NetInterface));
	Inet[ instance]->nicContext = malloc( sizeof(Enc624j600Context));
	if( Inet[ instance]->nicContext == NULL)
			goto err2;




	Inet[ instance]->instance = instance;
	memset( &Inet[ instance]->macAddr, 0, sizeof( MacAddr_u16));

	Inet[ instance]->macAddr.w[0] = 0xd0ff;
	Inet[ instance]->macAddr.w[1] = 0x5087;
	Inet[ instance]->macAddr.w[2] = 0xf80e + instance;

	Inet[ instance]->nicRxEvent = RX_EVENT;
	Inet[ instance]->nicTxEvent = TX_EVENT;

	sprintf( Inet[ instance]->name, "eth%d", instance);


	return EXIT_SUCCESS;


err2:
	free( Inet[ instance]);
err1:
	return EXIT_FAILURE;
}


static void cleanup(void *arg)
{

	NetInterface *inet = ( NetInterface *)arg;
	struct pbuf *p_txbuf = (struct pbuf *)inet->txbuf;
	printf("net drive %s thread exit \n", inet->name );

	pbuf_free( p_txbuf);
	enc624j600Driver.destory( inet);
	destory_net_interface( inet->instance);

}



static err_t destory_net_interface( int  instance)
{


	free( Inet[ instance]->nicContext);
	free( Inet[ instance]);

	return EXIT_SUCCESS;


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

	if( Dubug_info.irq_count[0] != old_ifo.irq_count[0] || Dubug_info.irq_count[1] != old_ifo.irq_count[1] )
	{
		old_ifo.irq_count[0] = Dubug_info.irq_count[0];
		old_ifo.irq_count[1] = Dubug_info.irq_count[1];
		printf("irq_count :");
		printf( "%d,%d \n", Dubug_info.irq_count[0], Dubug_info.irq_count[1]);
	}

	if( Dubug_info.event_count[0] != old_ifo.event_count[0] || Dubug_info.event_count[1] != old_ifo.event_count[1] )
	{
		old_ifo.event_count[0] = Dubug_info.event_count[0];
		old_ifo.event_count[1] = Dubug_info.event_count[1];
		printf("event count :");
		printf( "RX_EVENT %d, TX_EVENT %d \n", Dubug_info.event_count[0], Dubug_info.event_count[1]);
	}

	if( Dubug_info.event_handle_count != old_ifo.event_handle_count)
	{
		old_ifo.event_handle_count = Dubug_info.event_handle_count;
		printf( "event_handle_count %d\n", Dubug_info.event_handle_count);

	}

}




