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



const  uint8_t enc624_mac[2][6] = { { 0x01,0x01,0x01,0x01,0x01,0x01}, { 0x02,0x02,0x02,0x02,0x02,0x02}};

static void MyHandler( int sig_number );
static void show_debug_info( Sys_deginfo *info);
static int set_nonblock_flag( int desc, int value);
static void netif_link_chg(struct netif *netif);

int Net_connect_handle[2] = {-1};
err_t input_fn(struct pbuf *p, struct netif *inp);

NetInterface	*Inet[2];
bool	Running = true;


Sys_deginfo		Dubug_info;


struct netif	*NetIf[2];


int main(int argc, char *argv[])
{

	uint32_t i = 0, count = 0;
	uint32_t *p_u32;
	char opt;
	uint8_t 	instance;



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
	printf(" enter instance 1/2 or 3 for both \r\n");
	instance = getchar() - '0' ;
	assert( instance < 4);
	pbuf_init();
	for( i = 0; i < 2; i++)
	{
		if( instance & ( 1 << i))
		{
			NetIf[i] = malloc( sizeof( struct netif));
//			memcpy( NetIf[i]->hwaddr, enc624_mac[i], 6);
			NetIf[i]->num = i;
			NetIf[i]->input = input_fn;
			NetIf[i]->link_callback = netif_link_chg;
			netif_init( NetIf[i]);
			Inet[i] = NetIf[i]->ll_netif;
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
		count ++;
		if( count %40 == 0)
			printf(" run %04x ... \n", count );
		delay(100);

		if( Net_connect_handle[0] < 0)
		{
			Net_connect_handle[ 0] = netif_connect( NetIf[0], (u8_t *)&ethbroadcast);
			continue;
		}



		if( count %40 == 0)
		{
			p_txbuf = pbuf_alloc( PBUF_LINK, 1200, PBUF_POOL);

			if( p_txbuf)
			{
				p_u32 = ( uint32_t *)p_txbuf->payload;
				*p_u32 = count;
				p_txbuf->len = 4;
				NetIf[0]->upperlayer_output( NetIf[0], p_txbuf, Net_connect_handle[0]);
				pbuf_free( p_txbuf);
			}

		}


	}

	for( i = 0; i < 2; i++)
	{
		if( instance & ( 1 << i))
		{
			netif_remove( NetIf[i]);
			free( NetIf[i]);
		}
	}



	printf(" program exit ! \n");

	return EXIT_SUCCESS;
}


err_t input_fn(struct pbuf *p, struct netif *inp)
{
	int	i = 0;
	NetInterface *inet = ( NetInterface *)inp->ll_netif;
	uint8_t		*data = (uint8_t *) p->payload;
	TRACE_INFO("%s Recv %d datas  ", inet->name,  p->len);

	for( i = 0; i < p->len; i ++)
	{
		if( i % 16 == 0)
			TRACE_INFO("\r\n");
		TRACE_INFO("0x%02x ", data[i]);
	}
	TRACE_INFO("\r\n");
//	pbuf_free( p);
	return 0;
}

static void netif_link_chg(struct netif *netif)
{
	NetInterface *inet = ( NetInterface *)netif->ll_netif;

//	if( inet->linkState)
//	{
//		if( Net_connect_handle[ inet->instance] < 0)
//			Net_connect_handle[ inet->instance] = netif_connect( netif, (u8_t *)&ethbroadcast);
//	}
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




