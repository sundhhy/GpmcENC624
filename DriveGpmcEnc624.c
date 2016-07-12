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
	int i = 0, count = 0;
	char opt;
	uint8_t 	instance;

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

		show_debug_info( &Dubug_info);
		if( count %40 == 0)
		{
//
////
			printf(" run ... \n");
//			enc624j600_print_reg( Inet[ 0], ENC624J600_REG_ESTAT);
//			enc624j600_print_reg( Inet[ 0], ENC624J600_REG_EIR);
//			enc624j600_print_reg( Inet[ 0], ENC624J600_REG_EIE);
//
//
//
//			//Display actual speed and duplex mode
//			TRACE_DEBUG("%s %s\r\n",
//					Inet[ 0]->speed100 ? "100BASE-TX" : "10BASE-T",
//							Inet[ 0]->fullDuplex ? "Full-Duplex" : "Half-Duplex");
//
//
////			enc624j600DumpReg(Inet[ 0]);
////			enc624j600DumpPhyReg(Inet[ 0]);
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

static err_t create_net_interface( int  instance)
{
	Inet[ instance] = malloc( sizeof(NetInterface));
	if( Inet[ instance] == NULL)
		goto err1;


	memset( Inet[ instance], 0, sizeof(NetInterface));
	Inet[ instance]->nicContext = malloc( sizeof(Enc624j600Context));
	if( Inet[ instance]->nicContext == NULL)
			goto err2;

	Inet[ instance]->ethFrame = malloc( 2*1024);		//24k
	if( Inet[ instance]->ethFrame == NULL)
				goto err3;


	Inet[ instance]->instance = instance;
	memset( &Inet[ instance]->macAddr, 0, sizeof( MacAddr_u16));

	Inet[ instance]->macAddr.w[0] = 0xd0ff;
	Inet[ instance]->macAddr.w[1] = 0x5087;
	Inet[ instance]->macAddr.w[2] = 0xf80e + instance;

	Inet[ instance]->nicRxEvent = RX_EVENT;
	Inet[ instance]->nicTxEvent = TX_EVENT;

	sprintf( Inet[ instance]->name, "eth%d", instance);

	return EXIT_SUCCESS;

err3:
	free( Inet[ instance]->nicContext);
err2:
	free( Inet[ instance]);
err1:
	return EXIT_FAILURE;
}


static void cleanup(void *arg)
{
	NetInterface *inet = ( NetInterface *)arg;
	printf("net drive %s thread exit \n", inet->name );

	enc624j600Driver.destory( inet);
	destory_net_interface( inet->instance);

}

static void* enc_drive_main(void *arg)
{
	NetInterface *inet = ( NetInterface *)arg;
	err_t	ret = 0;



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
			enc624j600Driver.EventHandler( inet);

		}
		else if(  inet->isr_status & ISR_TX_EVENT)
		{
			atomic_clr( &inet->isr_status, ISR_TX_EVENT);
			enc624j600Driver.EventHandler( inet);

		}
		else if(  inet->isr_status & ISR_LINK_STATUS_CHG)
		{
			atomic_clr( &inet->isr_status, ISR_LINK_STATUS_CHG);
			enc624j600Driver.EventHandler( inet);

		}
		else if(  inet->isr_status & ISR_ERROR)
		{
			atomic_clr( &inet->isr_status, ISR_ERROR);
			printf(" ISR_ERROR happened \n");
		}

		pthread_cleanup_pop(0);
	}





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
