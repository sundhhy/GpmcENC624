/**
* @file 	ChiticDsNet.c
* @brief	正泰中自同步网络协议.
* @details
* @author	sundh
* @date		2016-08-02
* @version	0.1.1
* @par Copyright (c):
* 		正泰中自
* @par History:
*	version: author, date, desc\n
*	1.1.1:sundh,2016-08-02,创建
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
 * chitic 应用层的网络操作对象的私有成员
 *
 */
struct CDSNetobjPrivate{
	uint8_t	mac_addr[NETIF_MAX_HWADDR_LEN];
	char	netname[NET_NAME_LEN];
	struct netif*	netif;
}static  NoPrvt[NET_INSTANCE_NUM];

static 	NetAppObj*			NetObj[DS_HANDLE_NUM] = {NULL};		///< 使用句柄来索引网络对象
static	uint16_t			TagtNetID[DS_HANDLE_NUM] = {0};		///< 使用句柄来索引目标网络ID
static	uint16_t			ArpCacheIdx[DS_HANDLE_NUM] = {0};		///< 使用句柄来索引arp缓存

static struct pbuf			*RxPrcsHd;			///< 接受数据处理队列头
static	uint16_t			SendCount[NET_INSTANCE_NUM];

static pthread_cond_t Recv_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t Recv_mutex =  PTHREAD_MUTEX_INITIALIZER;

static err_t input_fn(struct pbuf *p, struct netif *inp);
static void netif_link_chg(struct netif *netif);
static uint16_t check_sum(uint16_t *buf, int len);
/**
 * @brief 初始化网络芯片的接口管理模块.
 *
 * @details 创建网络接口模块，并将使用预设的MAC地址和名称来初始化它。
 *
 * @param[in]	cthis 指向操作实体的指针.
 * @retval	ERR_OK	成功
 * @retval	ERR_MEM	内存不足
 * @retval	ERR_BAD_PARAMETER	传入的初始化对象是错误的
 * @par 修改日志
 * 		sundh 于2016-08-03创建
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
 * @brief 销毁网络接口管理模块
 *
 * @details 关闭服务线程，并释放资源
 *
 * @param[in]	cthis 指向操作实体的指针.
 * @retval	ERR_OK	成功
 * @par 修改日志
 * 		sundh 于2016-08-03创建
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
 * @brief 获取网络MAC地址
 *
 * @details
 *
 * @param[in]	cthis 指向操作实体的指针.
 * @retval	uint8_t* 指向MAC数据的指针
 * @par 修改日志
 * 		sundh 于2016-08-03创建
 */
uint8_t* get_mac(NetAppObj *cthis)
{
	return NoPrvt[ cthis->num].mac_addr;
}

/**
 * @brief 设置网络MAC地址
 *
 * @details  这里修改的mac地址只会在软件上起作用。
 * 要想修改到芯片上，这个函数应该在init之前调用。
 *
 * @param[in]	cthis 指向操作实体的指针.
 * @param[in]	*mac 修改的数据源
 * @par 修改日志
 * 		sundh 于2016-08-03创建
 */
void set_mac(NetAppObj *cthis, uint8_t *mac)
{
	assert( mac != NULL);
	if( mac == NULL)
		return ERR_BAD_PARAMETER;
	memcpy( NoPrvt[ cthis->num].mac_addr, mac, NETIF_MAX_HWADDR_LEN);
}

/**
 * @brief 获取网络名称
 *
 * @details
 *
 * @param[in]	cthis 指向操作实体的指针.
 * @retval	char* 指向网络名称的指针
 * @par 修改日志
 * 		sundh 于2016-08-03创建
 */
char* get_name(NetAppObj *cthis)
{
	return NoPrvt[ cthis->num].netname;
}

/**
 * @brief 设置网络名称
 *
 * @details
 *
 * @param[in]	cthis 指向操作实体的指针.
 * @param[in]	*name 修改的数据源
 * @par 修改日志
 * 		sundh 于2016-08-03创建
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
 * @brief 获取网络连接状态
 *
 * @details
 *
 * @param[in]	cthis 指向操作实体的指针.
 * @retval	true:网络芯片已经连接
 * @retval	false:网络芯片断开连接
 * @par 修改日志
 * 		sundh 于2016-08-03创建
 */
bool get_linkstate(NetAppObj *cthis)
{
	NetInterface *inet = ( NetInterface *)NoPrvt[ cthis->num].netif->ll_netif;


	return inet->linkState;
}
/**
 * @brief 获取链路层的一次的最大传输单元
 *
 * @details
 *
 * @param[in]	cthis 指向操作实体的指针.
 * @retval	链路层的一次的最大传输单元
 * @par 修改日志
 * 		sundh 于2016-08-03创建
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
 * @brief 向指定的网络号进行连接
 *
 * @details 调用此接口后，dsnet代表的那路网络芯片向外发送chitic arp请求包。 调用着通过返回值来获得连接的进展。
 * 当返回值是‘ERR_UNKOWN’时，说明连接正在处理过程中，调用着要继续调用。
 * 返回值为大于等于0的数，这表示成功建立建立。调用着要保存这个返回值作为后续操作的句柄。
 * 返回值为‘ERR_TIMEOUT’，表明此次连接超时失败。调用着要进行超时处理。
 * 返回值为ERR_UNAVAILABLE，说明句柄资源已经被用尽，调用者要把不用的句柄通过'DS_DisConnect'释放掉。
 *
 * @see DS_DisConnect
 * @param[in]	dsnet 代表需要进行连接的网络.
 * @param[in]	targetid 目标主机的id号.
 * @retval	大于等于0的整数 ： 连接已经成功建立,返回操作句柄.
 * @retval	ERR_UNKOWN: 连接过程还未完成。连接还不可用.
 * @retval  ERR_TIMEOUT : 建立建立超时退出.
 * @retval  ERR_UNAVAILABLE: 句柄资源不足
 * @attention 返回的句柄其实是个位图值。在本系统中，用int型来存储这个句柄，因此DS_HANDLE_NUM 要小于31。否则会超出int的范围。
 * @par 修改日志
 * 		sundh 于2016-08-03创建
 */
int DS_Connect( NetAppObj *dsnet, uint16_t targetid)
{
	int handle = 0;
	int ret = netif_connect( NoPrvt[dsnet->num].netif, targetid);



	//驱动发送过程中出现资源不足，就先延迟一段时间
	if( ret == ERR_UNAVAILABLE)
	{
		delay(10);
		return ERR_UNKOWN;
	}
	else if( ret < 0)
		return ret;

	//找到第一个未使用的句柄
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
 * @brief 断开句柄指向的连接。
 *
 * @details 断开连接，并释放句柄资源
 *
 * @param[in]	hd_bitmap 要关闭的连接.
 * @retval	ERR_OK ： 成功.
 * @retval	ERR_BAD_PARAMETER ： 传入的句柄有误.
 * @par 修改日志
 * 		sundh 于2016-08-03创建
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
 * @brief 向指定的句柄收取数据，并将数据发送方的id放置到fromid上.可以支持多个句柄同时接收。
 *
 * @details 接收时，如果没有数据，将会阻塞调用者。数据到来的时候，要做如下处理：
 * 1 根据handle来判断数据是否是发向本机的。如果不是就丢弃。返回错误码。
 * 2 判断协议类型，与‘DS_PRTC_TYPE’不符，丢弃。返回错误码。
 * 3 对头部进行校验和计算，如果计算结果不全为1，校验错误，丢弃。返回错误码。
 * 4 将数据放入buff，在 len ，（ 头部总长度 - 头部长度）， pbun->len - 头部长度,  这三者之间小的那个作为接收长度。
 * 5 如果fromid指针不为NULL，将源网络地址赋值给它。
 *
 * @param[in]	handle 接收的连接.
 * @param[in] 	buff   接受数据缓存区.
 * @param[in]	len		接受数据长度.
 * @param[in]	fromid	数据源地址
 * @retval	接受的数据长度.
 * @retval	ERR_VAL：错误的数据
 * @retval	ERR_BAD_PARAMETER：错误的句柄，一般出现在向未建立连接的句柄读取的时候
 * @par 修改日志
 * 		sundh 于2016-08-03创建
 */
int DSRecvFrom( int hd_bitmap, void *buff, int len, uint16_t *fromid)
{
	NetAppObj *netobj;
	ChiticDSHdr *ds_hdr = NULL;
	struct	pbuf	*rx_in_handle;	///< 处理中的缓存
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

	///< 从处理队列头获取数据
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
 * @brief 向指定的句柄发送数据
 *
 * @details 发送数据时，先申请pbuf内存。如果申请失败向下层的发送接口发送NULL数据，通知下层驱动进行刷新pbuf内存操作。
 * 然后延迟1ms后，以ERR_UNAVAILABLE的返回值退出。
 *
 * @param[in]	hd_bitmap 发送的连接.
 * @param[in] 	buff   发送数据缓存区.
 * @param[in]	len		发送数据长度.
 * @retval	发送的数据长度.
 * @retval	ERR_UNAVAILABLE: 发送失败
 * @attention	当句柄中包含多个连接的时候，只会像找到的第一个发送。剩下的不会去发送。
 * @par 修改日志
 * 		sundh 于2016-08-03创建
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
			/* 未处理链表插入到处理链表	*/
			if( inet->rxUnprocessed)
			{
				//将pbuf的标志改写为处理中
				p_iterator = inet->rxUnprocessed;
				while( p_iterator->next != NULL)
				{

					p_iterator->flags = PBUFFLAG_DEALING;
					p_iterator = p_iterator->next;
				}
				ret = insert_node_to_listtail( (void **)&RxPrcsHd, inet->rxUnprocessed);
				inet->rxUnprocessed = NULL;
			}

			/* 新的数据插入到处理链表	*/
			insert_node_to_listtail( (void **)&RxPrcsHd, p);

		}
		else
		{
			//释放缓存
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
		///< 将数据缓存放入待处理队列中.这里不需要上锁
		///< inet->rxUnprocessed 只能在这里修改
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

