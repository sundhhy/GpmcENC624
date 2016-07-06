#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <err.h>
#include "net.h"
#include "enc624j600.h"

static err_t create_net_interface( NetInterface **Inet);

NetInterface	*Inet1;
int main(int argc, char *argv[])
{
	err_t ret = 0;

	printf("Drive of ENC624\n");

	ret = create_net_interface( &Inet1);
	if( ret)
	{
		printf("create_net_interface fail, exit\n");
		return EXIT_FAILURE;
	}


	enc624j600Driver.Init( Inet1);

	return EXIT_SUCCESS;
}

static err_t create_net_interface( NetInterface **Inet)
{
	*Inet = malloc( sizeof(NetInterface));
	if( Inet == NULL)
		goto err1;

	memset( *Inet, 0, sizeof(NetInterface));
	(*Inet)->nicContext = malloc( sizeof(Enc624j600Context));
	if( (*Inet)->nicContext == NULL)
			goto err2;

	(*Inet)->instance = 0;
	memset( &(*Inet)->macAddr, 0, sizeof(MacAddr_u16));

//	(*Inet)->macAddr.w[0] = 0x1234;
//	(*Inet)->macAddr.w[1] = 0x5678;
//	(*Inet)->macAddr.w[2] = 0x9abc;

	return EXIT_SUCCESS;

err2:
	free(*Inet);
err1:
	return EXIT_FAILURE;
}


