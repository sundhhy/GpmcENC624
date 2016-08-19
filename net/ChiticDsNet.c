/**
* @file 	ChiticDsNet.c
* @brief	��̩����ͬ������Э��.
* @details
* @author	sundh
* @date		2016-08-02
* @version	0.1.1
* @par Copyright (c):
* 		��̩����
* @par History:
*	version: author, date, desc\n
*	1.1.1:sundh,2016-08-02,����
*
*/
#include "netif.h"
#include "net.h"
#include "ChiticDsNet.h"
#include "hal_config.h"
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "debug.h"
/**
 * chitic Ӧ�ò��������������˽�г�Ա
 *
 */
struct CDSNetobjPrivate{
	uint8_t	mac_addr[NETIF_MAX_HWADDR_LEN];
	char	netname[NET_NAME_LEN];
	struct netif*	netif;
}static  NoPrvt[NET_INSTANCE_NUM];

static 	NetAppObj*			NetObj[DS_HANDLE_NUM] = {NULL};		///< ʹ�þ���������������
static	uint16_t			TagtNetID[DS_HANDLE_NUM] = {0};		///< ʹ�þ��������Ŀ������ID
static	uint16_t			ArpCacheIdx[DS_HANDLE_NUM] = {0};		///< ʹ�þ��������arp����

static struct pbuf			*RxPrcsHd;			///< �������ݴ������ͷ
static	uint16_t			SendCount[NET_INSTANCE_NUM];

static pthread_cond_t Recv_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t Recv_mutex =  PTHREAD_MUTEX_INITIALIZER;

static err_t input_fn(struct pbuf *p, struct netif *inp);
static void netif_link_chg(struct netif *netif);
static uint16_t check_sum(uint16_t *buf, int len);
/**
 * @brief ��ʼ������оƬ�Ľӿڹ���ģ��.
 *
 * @details ��������ӿ�ģ�飬����ʹ��Ԥ���MAC��ַ����������ʼ������
 *
 * @param[in]	cthis ָ�����ʵ���ָ��.
 * @retval	ERR_OK	�ɹ�
 * @retval	ERR_MEM	�ڴ治��
 * @retval	ERR_BAD_PARAMETER	����ĳ�ʼ�������Ǵ����
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
err_t net_init( NetAppObj *cthis)
{
	uint16_t net_num = cthis->num;
	static char fist_run = 1;
	assert( net_num < NET_INSTANCE_NUM);
	if( net_num > NET_INSTANCE_NUM)
		return ERR_BAD_PARAMETER;

	if( fist_run)
	{
		fist_run = 0;
		pbuf_init();
	}

	NoPrvt[ net_num].netif = malloc( sizeof( struct netif));
	if( NoPrvt[ net_num].netif == NULL)
		return ERR_MEM;

	NoPrvt[ net_num].netif->uplayer = cthis;
	NoPrvt[ net_num].netif->hwaddr = NoPrvt[ net_num].mac_addr;
	NoPrvt[ net_num].netif->name = NoPrvt[ net_num].netname;
	NoPrvt[ net_num].netif->num = net_num;
	NoPrvt[ net_num].netif->input = input_fn;
	NoPrvt[ net_num].netif->link_callback = netif_link_chg;
	NoPrvt[ net_num].netif->myid = cthis->netaddr;
	netif_init( NoPrvt[ net_num].netif);


	return ERR_OK;
}

/**
 * @brief ��������ӿڹ���ģ��
 *
 * @details �رշ����̣߳����ͷ���Դ
 *
 * @param[in]	cthis ָ�����ʵ���ָ��.
 * @retval	ERR_OK	�ɹ�
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
err_t net_destory( NetAppObj *cthis)
{
	uint16_t net_num = cthis->num;
	assert( net_num < NET_INSTANCE_NUM);
	if( net_num > NET_INSTANCE_NUM)
			return ERR_BAD_PARAMETER;

	netif_remove( NoPrvt[ net_num].netif);
	free( NoPrvt[ net_num].netif);

	return ERR_OK;
}

/**
 * @brief ��ȡ����MAC��ַ
 *
 * @details
 *
 * @param[in]	cthis ָ�����ʵ���ָ��.
 * @retval	uint8_t* ָ��MAC���ݵ�ָ��
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
uint8_t* get_mac(NetAppObj *cthis)
{
	return NoPrvt[ cthis->num].mac_addr;
}

/**
 * @brief ��������MAC��ַ
 *
 * @details  �����޸ĵ�mac��ַֻ��������������á�
 * Ҫ���޸ĵ�оƬ�ϣ��������Ӧ����init֮ǰ���á�
 *
 * @param[in]	cthis ָ�����ʵ���ָ��.
 * @param[in]	*mac �޸ĵ�����Դ
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
void set_mac(NetAppObj *cthis, uint8_t *mac)
{
	assert( mac != NULL);
	if( mac == NULL)
		return ERR_BAD_PARAMETER;
	memcpy( NoPrvt[ cthis->num].mac_addr, mac, NETIF_MAX_HWADDR_LEN);
}

/**
 * @brief ��ȡ��������
 *
 * @details
 *
 * @param[in]	cthis ָ�����ʵ���ָ��.
 * @retval	char* ָ���������Ƶ�ָ��
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
char* get_name(NetAppObj *cthis)
{
	return NoPrvt[ cthis->num].netname;
}

/**
 * @brief ������������
 *
 * @details
 *
 * @param[in]	cthis ָ�����ʵ���ָ��.
 * @param[in]	*name �޸ĵ�����Դ
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
void set_name(NetAppObj *cthis, char *name)
{
	int i = 0;
	assert( name != NULL);
	if( name == NULL)
		return ERR_BAD_PARAMETER;
	for( i = 0; i < NET_NAME_LEN; i ++)
	{
		if( name[i] == '\0')
			break;
		NoPrvt[ cthis->num].netname[i] = name[i];
	}
}

/**
 * @brief ��ȡ��������״̬
 *
 * @details
 *
 * @param[in]	cthis ָ�����ʵ���ָ��.
 * @retval	true:����оƬ�Ѿ�����
 * @retval	false:����оƬ�Ͽ�����
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
bool get_linkstate(NetAppObj *cthis)
{
	NetInterface *inet = ( NetInterface *)NoPrvt[ cthis->num].netif->ll_netif;


	return inet->linkState;
}
/**
 * @brief ��ȡ��·���һ�ε�����䵥Ԫ
 *
 * @details
 *
 * @param[in]	cthis ָ�����ʵ���ָ��.
 * @retval	��·���һ�ε�����䵥Ԫ
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
uint16_t	get_mtu(NetAppObj *cthis)
{
	return NoPrvt[ cthis->num].netif->mtu;
}

CTOR(NetAppObj)

FUNCTION_SETTING(init, net_init);
FUNCTION_SETTING(destroy, net_destory);
FUNCTION_SETTING(get_mac, get_mac);
FUNCTION_SETTING(set_mac, set_mac);
FUNCTION_SETTING(get_name, get_name);
FUNCTION_SETTING(set_name, set_name);
FUNCTION_SETTING(get_linkstate, get_linkstate);
FUNCTION_SETTING(get_mtu, get_mtu);
END_CTOR


/**
 * @brief ��ָ��������Ž�������
 *
 * @details ���ô˽ӿں�dsnet�������·����оƬ���ⷢ��chitic arp������� ������ͨ������ֵ��������ӵĽ�չ��
 * ������ֵ�ǡ�ERR_UNKOWN��ʱ��˵���������ڴ�������У�������Ҫ�������á�
 * ����ֵΪ���ڵ���0���������ʾ�ɹ�����������������Ҫ�����������ֵ��Ϊ���������ľ����
 * ����ֵΪ��ERR_TIMEOUT���������˴����ӳ�ʱʧ�ܡ�������Ҫ���г�ʱ����
 * ����ֵΪERR_UNAVAILABLE��˵�������Դ�Ѿ����þ���������Ҫ�Ѳ��õľ��ͨ��'DS_DisConnect'�ͷŵ���
 *
 * @see DS_DisConnect
 * @param[in]	dsnet ������Ҫ�������ӵ�����.
 * @param[in]	targetid Ŀ��������id��.
 * @retval	���ڵ���0������ �� �����Ѿ��ɹ�����,���ز������.
 * @retval	ERR_UNKOWN: ���ӹ��̻�δ��ɡ����ӻ�������.
 * @retval  ERR_TIMEOUT : ����������ʱ�˳�.
 * @retval  ERR_UNAVAILABLE: �����Դ����
 * @attention ���صľ����ʵ�Ǹ�λͼֵ���ڱ�ϵͳ�У���int�����洢�����������DS_HANDLE_NUM ҪС��31������ᳬ��int�ķ�Χ��
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
int DS_Connect( NetAppObj *dsnet, uint16_t targetid)
{
	int handle = 0;
	int ret = netif_connect( NoPrvt[dsnet->num].netif, targetid);



	//�������͹����г�����Դ���㣬�����ӳ�һ��ʱ��
	if( ret == ERR_UNAVAILABLE)
	{
		delay(10);
		return ERR_UNKOWN;
	}
	else if( ret < 0)
		return ret;

	//�ҵ���һ��δʹ�õľ��
	for( handle = 0; handle < DS_HANDLE_NUM; handle ++)
	{
		if( NetObj[ handle] == NULL)
		{
			NetObj[ handle] = dsnet;
			ArpCacheIdx[ handle] = ret;
			TagtNetID[handle] = targetid;

			return (1 << handle);
		}
	}
	return ERR_UNAVAILABLE;
}

/**
 * @brief �Ͽ����ָ������ӡ�
 *
 * @details �Ͽ����ӣ����ͷž����Դ
 *
 * @param[in]	hd_bitmap Ҫ�رյ�����.
 * @retval	ERR_OK �� �ɹ�.
 * @retval	ERR_BAD_PARAMETER �� ����ľ������.
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
err_t DS_DisConnect( int hd_bitmap)
{
	int  i ;
	if( hd_bitmap < 0)
		return ERR_BAD_PARAMETER;
	for( i = 0; i < DS_HANDLE_NUM; i++)
	{
		if( hd_bitmap & ( 1 << i))
		{
			if( NetObj[ i] == NULL)
				continue;
			netif_disconnect( NoPrvt[ NetObj[ i]->num].netif, ArpCacheIdx[ i] );
			NetObj[ i] = NULL;
		}
	}

	return ERR_OK;
}

/**
 * @brief ��ָ���ľ����ȡ���ݣ��������ݷ��ͷ���id���õ�fromid��.����֧�ֶ�����ͬʱ���ա�
 *
 * @details ����ʱ�����û�����ݣ��������������ߡ����ݵ�����ʱ��Ҫ�����´���
 * 1 ����handle���ж������Ƿ��Ƿ��򱾻��ġ�������ǾͶ��������ش����롣
 * 2 �ж�Э�����ͣ��롮DS_PRTC_TYPE�����������������ش����롣
 * 3 ��ͷ������У��ͼ��㣬�����������ȫΪ1��У����󣬶��������ش����롣
 * 4 �����ݷ���buff���� len ���� ͷ���ܳ��� - ͷ�����ȣ��� pbun->len - ͷ������,  ������֮��С���Ǹ���Ϊ���ճ��ȡ�
 * 5 ���fromidָ�벻ΪNULL����Դ�����ַ��ֵ������
 *
 * @param[in]	handle ���յ�����.
 * @param[in] 	buff   �������ݻ�����.
 * @param[in]	len		�������ݳ���.
 * @param[in]	fromid	����Դ��ַ
 * @retval	���ܵ����ݳ���.
 * @retval	ERR_VAL�����������
 * @retval	ERR_BAD_PARAMETER������ľ����һ���������δ�������ӵľ����ȡ��ʱ��
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
int DSRecvFrom( int hd_bitmap, void *buff, int len, uint16_t *fromid)
{
	NetAppObj *netobj;
	ChiticDSHdr *ds_hdr = NULL;
	struct	pbuf	*rx_in_handle;	///< �����еĻ���
	uint16_t 	check = 0;
	uint16_t	pbuf_left;

	uint16_t	*hd_data;
	char	*p = NULL;
	short	recv_len, i;
	short	hit = 0;
	assert( hd_bitmap > 0);
	if( hd_bitmap < 1)
		return ERR_BAD_PARAMETER;

	if( hd_bitmap < 0)
		return ERR_BAD_PARAMETER;
	recv_len = -1;

	///< �Ӵ������ͷ��ȡ����
	pthread_mutex_lock( &Recv_mutex );
	while( RxPrcsHd == NULL)
		pthread_cond_wait( &Recv_cond, &Recv_mutex);
	rx_in_handle = RxPrcsHd;
	RxPrcsHd = RxPrcsHd->next;
	pthread_mutex_unlock( &Recv_mutex);


	ds_hdr = ( ChiticDSHdr *)( rx_in_handle->payload + PBUF_LINK_HLEN);

	if( ds_hdr->ihl != DS_PRTC_TYPE)
		goto recv_exit;
	hd_data = (uint16_t *)ds_hdr;
	check = check_sum( hd_data, ds_hdr->head_len/2);
	if( check != 0xffff)
		goto recv_exit;
	ds_hdr->src_addr = PP_NTOHS( ds_hdr->src_addr);
	ds_hdr->dst_addr = PP_NTOHS( ds_hdr->dst_addr);
	ds_hdr->total_len = PP_NTOHS( ds_hdr->total_len);
	for( i = 0; i < DS_HANDLE_NUM; i++)
	{
		if( hd_bitmap & ( 1 << i))
		{
			netobj = NetObj[ i];
			if( netobj)
			{
				if( ds_hdr->dst_addr ==  netobj->netaddr )
				{
					hit =1;
					break;
				}
			}

		}
	}
	if( hit == 0)
		goto recv_exit;
	recv_len = ds_hdr->total_len - ds_hdr->head_len;
	pbuf_left = rx_in_handle->len - PBUF_LINK_HLEN - 4 - ds_hdr->head_len;
	recv_len = (recv_len < pbuf_left) ? recv_len : pbuf_left;
	recv_len = (recv_len < len) ? recv_len : len;

	p = (char *)ds_hdr + ds_hdr->head_len;
	memcpy( buff, p, recv_len);
	if( fromid)
		*fromid = ds_hdr->src_addr;


recv_exit:
	Dubug_info.pbuf_free_local = 1;
	pbuf_free( rx_in_handle);


	if( recv_len >= 0)
		return recv_len;
	return ERR_VAL;

}
/**
 * @brief ��ָ���ľ����������
 *
 * @details ��������ʱ��������pbuf�ڴ档�������ʧ�����²�ķ��ͽӿڷ���NULL���ݣ�֪ͨ�²���������ˢ��pbuf�ڴ������
 * Ȼ���ӳ�1ms����ERR_UNAVAILABLE�ķ���ֵ�˳���
 *
 * @param[in]	hd_bitmap ���͵�����.
 * @param[in] 	buff   �������ݻ�����.
 * @param[in]	len		�������ݳ���.
 * @retval	���͵����ݳ���.
 * @retval	ERR_UNAVAILABLE: ����ʧ��
 * @attention	������а���������ӵ�ʱ��ֻ�����ҵ��ĵ�һ�����͡�ʣ�µĲ���ȥ���͡�
 * @par �޸���־
 * 		sundh ��2016-08-03����
 */
int DSSendTo( int hd_bitmap, void *buff, int len)
{
	short num, i;
	int  handle = -1;
	struct pbuf *p_txbuf;

	ChiticDSHdr *ds_hdr = NULL;
	assert( hd_bitmap > 0);
	if( hd_bitmap < 1)
		return ERR_BAD_PARAMETER;
	for( i = 0; i < DS_HANDLE_NUM; i++)
	{
		if( hd_bitmap & ( 1 << i))
		{

			if( NetObj[ handle])
			{
				handle = i;
				break;
			}
		}

	}
	if( handle < 0)
		return ERR_BAD_PARAMETER;

	num = NetObj[ handle]->num;

	assert( len <=  (  NoPrvt[ NetObj[ handle]->num].netif->mtu - PBUF_LINK_HLEN - 4 - DS_PRTC_HDRLEN));
	if( len > (  NoPrvt[ NetObj[ handle]->num].netif->mtu - PBUF_LINK_HLEN - 4 - DS_PRTC_HDRLEN))
	{
		printf("send data %d > %d  \n ", len, \
				(NoPrvt[ NetObj[ handle]->num].netif->mtu - PBUF_LINK_HLEN - 4 - DS_PRTC_HDRLEN));
		return ERR_BAD_PARAMETER;
	}
	p_txbuf = pbuf_alloc( PBUF_LINK, len + DS_PRTC_HDRLEN, PBUF_TX_POOL);
	if( p_txbuf != NULL)
	{
		ds_hdr = ( ChiticDSHdr *)p_txbuf->payload;
		ds_hdr->ihl = DS_PRTC_TYPE;
		ds_hdr->version = DS_PRTC_VER;
		ds_hdr->src_addr = PP_HTONS(NetObj[ handle]->netaddr);
		ds_hdr->dst_addr = PP_HTONS(TagtNetID[handle]);
		ds_hdr->head_len = DS_PRTC_HDRLEN;
		ds_hdr->total_len = PP_HTONS( len + DS_PRTC_HDRLEN);
		ds_hdr->count = PP_HTONS(SendCount[num]);


		ds_hdr->check  = check_sum( (uint16_t *)ds_hdr, ds_hdr->head_len/2 - 1);
//		ds_hdr->check = PP_HTONS(SendCount[num]);
		SendCount[num] ++;

		memcpy( p_txbuf->payload + DS_PRTC_HDRLEN, buff, len);

		if( NoPrvt[ NetObj[ handle]->num].netif->upperlayer_output( NoPrvt[ NetObj[ handle]->num].netif, p_txbuf, ArpCacheIdx[handle]) != ERR_OK)
		{
			Dubug_info.pbuf_free_local = 2;
			pbuf_free( p_txbuf);
			return ERR_UNAVAILABLE;
		}
		return len;
	}
	for( i = 0 ; i < NET_INSTANCE_NUM; i++)
		NoPrvt[ i].netif->upperlayer_output( NoPrvt[ i].netif, NULL, -1);
	delay(1);
	return ERR_UNAVAILABLE;


}




static err_t input_fn(struct pbuf *p, struct netif *inp)
{
	NetInterface *inet = ( NetInterface *)inp->ll_netif;
	struct pbuf  *p_iterator;
	err_t ret ;


	if( pthread_mutex_trylock( &Recv_mutex) == EOK )
	{
		if( p)
		{
			p->flags = PBUFFLAG_DEALING;
			/* δ����������뵽��������	*/
			if( inet->rxUnprocessed)
			{
				//��pbuf�ı�־��дΪ������
				p_iterator = inet->rxUnprocessed;
				while( p_iterator->next != NULL)
				{

					p_iterator->flags = PBUFFLAG_DEALING;
					p_iterator = p_iterator->next;
				}
				ret = insert_node_to_listtail( (void **)&RxPrcsHd, inet->rxUnprocessed);
				inet->rxUnprocessed = NULL;
			}

			/* �µ����ݲ��뵽��������	*/
			insert_node_to_listtail( (void **)&RxPrcsHd, p);

		}
		else
		{
			//�ͷŻ���
			if( RxPrcsHd)
			{
				p_iterator = RxPrcsHd;
				RxPrcsHd = RxPrcsHd->next;
				p_iterator->next = NULL;
				Dubug_info.pbuf_free_local = 7;
				pbuf_free(p_iterator);
			}

		}
		pthread_cond_signal( &Recv_cond);
		pthread_mutex_unlock( &Recv_mutex );
		return ERR_OK;
	}
	else
	{
		///< �����ݻ����������������.���ﲻ��Ҫ����
		///< inet->rxUnprocessed ֻ���������޸�
		if( p)
		{
			p->flags = PBUFFLAG_UNPROCESS;
			insert_node_to_listtail( (void **)&inet->rxUnprocessed, p);
		}

	}

	return ERR_OK;

}

static void netif_link_chg(struct netif *netif)
{
	NetAppObj *netobj = netif->uplayer;
	if( netobj->linkstate_change_callback)
		netobj->linkstate_change_callback(netobj);
}

static uint16_t check_sum(uint16_t *buf, int len)
{
	int i ;
	uint16_t check = 0;
	for( i = 0; i < len; i ++)
	{
		check += ~buf[i];
	}
	return check;

}

