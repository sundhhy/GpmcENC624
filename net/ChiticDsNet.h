/**
* @file 	ChiticDs.h
* @brief	正泰中自同步网络协议的头文件.
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
#ifndef CHITICDSNET_H_
#define CHITICDSNET_H_
#include <stdbool.h>
#include "error_head.h"
#include "lw_oopc.h"
#define __LITTLE_ENDIAN_BITFIELD	1
#define 	DS_PRTC_VER		01	///< chitic 数据同步协议版本号
#define 	DS_PRTC_TYPE	11	///< chitic 数据同步协议类型码
#define		DS_HANDLE_NUM	16	///< 最大支持的句柄数量，暂定16个。
								///< 在本系统中，用int型来存储这个句柄，因此DS_HANDLE_NUM 要小于31。否则会超出int的范围。
#define		DS_PRTC_HDRLEN	12	///< chitic 数据同步协议头长度

#define		USER_DATA_MAX(mtu)	( mtu- PBUF_LINK_HLEN - 4 - DS_PRTC_HDRLEN)

/**
 * chitic 数据同步协议头结构
 *
 */
typedef struct {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	uint8_t		ihl:4,
				version:4;
#elif defined (__BIG_ENDIAN_BITFIELD)
	__u8	version:4,
  		ihl:4;
#endif
	uint8_t		head_len;
	uint16_t	total_len;
	uint16_t	count;
	uint16_t	src_addr;
	uint16_t	dst_addr;
	uint16_t	check;
}ChiticDSHdr;




/**
 * 应用层的网络操作对象.
 *
 */
CLASS(NetAppObj)
{
	///<序号，用于指定网络芯片，应该是0或1
	uint16_t	num;
	///<唯一的网络地址
	uint16_t	netaddr;

	err_t	(*init)( NetAppObj *);
	err_t	(*destroy)( NetAppObj *);

	///< 获取/设置 网络MAC地址
	uint8_t*	(*get_mac)(NetAppObj *);
	void 	(*set_mac)(NetAppObj *, uint8_t *);

	///< 获取/设置 网络名称
	char*	(*get_name)(NetAppObj *);
	void 	(*set_name)(NetAppObj *, char *);

	///< 获取网络芯片的最大传输单元
	uint16_t	(*get_mtu)(NetAppObj *);
	///< 获取网络芯片的连接状态
	bool 	(*get_linkstate)(NetAppObj *);
	///< 当网络芯片的连接状态发生变化的时候，自动调用该回调函数
	void	(*linkstate_change_callback)( NetAppObj *);

};

/**
 *  @brief 向指定的网络号进行连接
 */
int DS_Connect( NetAppObj *dsnet, uint16_t targetid);

/**
 *  @brief 断开指定的连接
 */
err_t DS_DisConnect( int hd_bitmap);

/**
 *  @brief 向指定的句柄收取数据，并将数据发送方的id放置到fromid上
 */
int DSRecvFrom( int hd_bitmap, void *buff, int len, uint16_t *fromid);
/**
 *  @brief 向指定的句柄发送数据
 */
int DSSendTo( int hd_bitmap, void *buff, int len);



#endif /* CHITICDSNET_H_ */
