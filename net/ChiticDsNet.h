/**
* @file 	ChiticDs.h
* @brief	��̩����ͬ������Э���ͷ�ļ�.
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
#ifndef CHITICDSNET_H_
#define CHITICDSNET_H_
#include <stdbool.h>
#include "error_head.h"
#include "lw_oopc.h"
#define __LITTLE_ENDIAN_BITFIELD	1
#define 	DS_PRTC_VER		01	///< chitic ����ͬ��Э��汾��
#define 	DS_PRTC_TYPE	11	///< chitic ����ͬ��Э��������
#define		DS_HANDLE_NUM	16	///< ���֧�ֵľ���������ݶ�16����
								///< �ڱ�ϵͳ�У���int�����洢�����������DS_HANDLE_NUM ҪС��31������ᳬ��int�ķ�Χ��
#define		DS_PRTC_HDRLEN	12	///< chitic ����ͬ��Э��ͷ����

#define		USER_DATA_MAX(mtu)	( mtu- PBUF_LINK_HLEN - 4 - DS_PRTC_HDRLEN)

/**
 * chitic ����ͬ��Э��ͷ�ṹ
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
 * Ӧ�ò�������������.
 *
 */
CLASS(NetAppObj)
{
	///<��ţ�����ָ������оƬ��Ӧ����0��1
	uint16_t	num;
	///<Ψһ�������ַ
	uint16_t	netaddr;

	err_t	(*init)( NetAppObj *);
	err_t	(*destroy)( NetAppObj *);

	///< ��ȡ/���� ����MAC��ַ
	uint8_t*	(*get_mac)(NetAppObj *);
	void 	(*set_mac)(NetAppObj *, uint8_t *);

	///< ��ȡ/���� ��������
	char*	(*get_name)(NetAppObj *);
	void 	(*set_name)(NetAppObj *, char *);

	///< ��ȡ����оƬ������䵥Ԫ
	uint16_t	(*get_mtu)(NetAppObj *);
	///< ��ȡ����оƬ������״̬
	bool 	(*get_linkstate)(NetAppObj *);
	///< ������оƬ������״̬�����仯��ʱ���Զ����øûص�����
	void	(*linkstate_change_callback)( NetAppObj *);

};

/**
 *  @brief ��ָ��������Ž�������
 */
int DS_Connect( NetAppObj *dsnet, uint16_t targetid);

/**
 *  @brief �Ͽ�ָ��������
 */
err_t DS_DisConnect( int hd_bitmap);

/**
 *  @brief ��ָ���ľ����ȡ���ݣ��������ݷ��ͷ���id���õ�fromid��
 */
int DSRecvFrom( int hd_bitmap, void *buff, int len, uint16_t *fromid);
/**
 *  @brief ��ָ���ľ����������
 */
int DSSendTo( int hd_bitmap, void *buff, int len);



#endif /* CHITICDSNET_H_ */
