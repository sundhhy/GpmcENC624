/**
 * @file enc624j600.c
 * @brief ENC624J600/ENC424J600 Ethernet controller
 *
 * @section License
 *
 * Copyright (C) 2010-2015 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 1.6.0
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL NIC_TRACE_LEVEL

//Dependencies
//#include "core/net.h"
#include "enc624j600.h"
#include "hal_config.h"
#include "debug.h"
#include <string.h>
#include <unistd.h>
#include "osAbstraction.h"

//为两种接口形式分别提供读写寄存器的方法
void ( *WriteReg[2])(NetInterface *interface, uint8_t address, uint16_t data);
void ( *SetBit[2])(NetInterface *interface, uint8_t address, uint16_t mask);
void ( *ClearBit[2])(NetInterface *interface, uint8_t address, uint16_t mask);
uint16_t ( *ReadReg[2])(NetInterface *interface, uint8_t address);

int I_type = 0;		//接口类型

/**
 * @brief ENC624J600 driver
 **/

const NicDriver enc624j600Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   enc624j600Init,
   enc624j600Tick,
   enc624j600EnableIrq,
   enc624j600DisableIrq,
   enc624j600EventHandler,
   enc624j600SetMacFilter,
   enc624j600SendPacket,
   enc624j600destory,
   NULL,
   TRUE,
   TRUE,
   TRUE
};

MacAddr_u16	MAC_UNSPECIFIED_ADDR = { { 0, 0, 0}};


Drive_Gpmc			*Drive_PSPDriver[2];
Drive_Gpio			*Drive_extIntDriver[2];

/**
 * @brief ENC624J600 controller initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

err_t enc624j600Init(NetInterface *interface)
{
   Enc624j600Context *context;


   //Debug message
   TRACE_INFO("Initializing ENC624J600 Ethernet controller...\r\n");

   //ENC接口连接到GPMC总线和GPIO中断引脚

#ifndef DEBUG_ONLY_GPIO_INIT
   if( Hal_enc_cfg.interface_type == ENC_INTERFACE_PSP)
   {
	   Drive_PSPDriver[ interface->instance] = Drive_Gpmc_new();
	   if( Drive_PSPDriver[ interface->instance] == NULL)
		   return ERROR_T(init_memory_invalid);
//	   interface->busDriver = SUPER_PTR(Drive_PSPDriver, IBusDrive);
	   interface->busDriver = Drive_PSPDriver[ interface->instance];

	   WriteReg[ ENC_INTERFACE_PSP] = enc624_PSP_WriteReg;
	   ReadReg[ ENC_INTERFACE_PSP] = enc624_PSP_ReadReg;
	   SetBit[ ENC_INTERFACE_PSP] = enc624_PSP_SetBit;
	   ClearBit[ ENC_INTERFACE_PSP] = enc624_PSP_ClearBit;


   }
   else {
	   return ERROR_T(invalid_bus_type);
   }
   I_type = Hal_enc_cfg.interface_type;

#endif
   Drive_extIntDriver[ interface->instance] = Drive_Gpio_new();
   if( Drive_extIntDriver[ interface->instance] == NULL)
	   goto err_extInt;
//   interface->extIntDriver =  SUPER_PTR(Drive_extIntDriver, IExternIntr);
   interface->extIntDriver = Drive_extIntDriver[ interface->instance];
   interface->extIntDriver->irq_handle = enc624j600IrqHandler;
   interface->extIntDriver->irq_handle_arg = interface;

   //Initialize external interrupt line
   interface->extIntDriver->config = Hal_enc_cfg.extInt_cfg[ interface->instance];

#ifdef  DEBUG_ONLY_GPIO_INIT
   interface->extIntDriver->init( interface->extIntDriver);
#endif

#ifndef DEBUG_ONLY_GPIO_INIT

   //Initialize SPI
   interface->busDriver->init( interface->busDriver, &interface->instance);
   //Initialize external interrupt line
   interface->extIntDriver->init( interface->extIntDriver);

   //Point to the driver context
   context = (Enc624j600Context *) interface->nicContext;
   //Initialize driver specific variables
   context->nextPacketPointer = ENC624J600_RX_BUFFER_START;

   //First, issue a system reset
   enc624j600SoftReset(interface);

   //Disable CLKOUT output
   WriteReg[I_type](interface, ENC624J600_REG_ECON2, ECON2_ETHEN | ECON2_STRCH);

   //Optionally set the station MAC address
   if(macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR) == 0)
   {
      //Use the factory preprogrammed station address
      interface->macAddr.w[0] = ReadReg[ I_type](interface, ENC624J600_REG_MAADR1);
      interface->macAddr.w[1] = ReadReg[ I_type](interface, ENC624J600_REG_MAADR2);
      interface->macAddr.w[2] = ReadReg[ I_type](interface, ENC624J600_REG_MAADR3);
   }
   else
   {
      //Override the factory preprogrammed address
      WriteReg[I_type](interface, ENC624J600_REG_MAADR1, interface->macAddr.w[0]);
      WriteReg[I_type](interface, ENC624J600_REG_MAADR2, interface->macAddr.w[1]);
      WriteReg[I_type](interface, ENC624J600_REG_MAADR3, interface->macAddr.w[2]);
   }

   //Set receive buffer location
   WriteReg[I_type](interface, ENC624J600_REG_ERXST, ENC624J600_RX_BUFFER_START);
   //Program the tail pointer ERXTAIL to the last even address of the buffer
   WriteReg[I_type](interface, ENC624J600_REG_ERXTAIL, ENC624J600_RX_BUFFER_STOP);

   //Configure the receive filters
   WriteReg[I_type](interface, ENC624J600_REG_ERXFCON, ERXFCON_HTEN |
      ERXFCON_CRCEN | ERXFCON_RUNTEN | ERXFCON_UCEN | ERXFCON_BCEN);

   //Initialize the hash table
   WriteReg[I_type](interface, ENC624J600_REG_EHT1, 0x0000);
   WriteReg[I_type](interface, ENC624J600_REG_EHT2, 0x0000);
   WriteReg[I_type](interface, ENC624J600_REG_EHT3, 0x0000);
   WriteReg[I_type](interface, ENC624J600_REG_EHT4, 0x0000);

   //All short frames will be zero-padded to 60 bytes and a valid CRC is then appended
   WriteReg[I_type](interface, ENC624J600_REG_MACON2,

      MACON2_DEFER | MACON2_PADCFG0 | MACON2_TXCRCEN | MACON2_R1 );

   //Program the MAMXFL register with the maximum frame length to be accepted
   WriteReg[I_type](interface, ENC624J600_REG_MAMXFL, 1518);

   //PHY initialization
   enc624j600WritePhyReg(interface, ENC624J600_PHY_REG_PHANA, PHANA_ADPAUS0 |
      PHANA_AD100FD | PHANA_AD100 | PHANA_AD10FD | PHANA_AD10 | PHANA_ADIEEE0);

   //sundh add
   enc624j600WritePhyReg(interface, ENC624J600_PHY_REG_PHCON1, PHCON1_ANEN | PHCON1_RENEG );

   //Clear interrupt flags
   WriteReg[I_type](interface, ENC624J600_REG_EIR, 0x0000);

   //Configure interrupts as desired
   WriteReg[I_type](interface, ENC624J600_REG_EIE, EIE_INTIE |

  EIE_LINKIE | EIE_PKTIE | EIE_TXIE | EIE_TXABTIE | EIE_PCFULIE | EIE_RXABTIE);		//sundh add 'EIE_RXABTIE'


   //Set RXEN to enable reception
   SetBit[ I_type](interface, ENC624J600_REG_ECON1, ECON1_RXEN);

   //Dump registers for debugging purpose
   enc624j600DumpReg(interface);
   enc624j600DumpPhyReg(interface);

   //Force the TCP/IP stack to check the link state
   osSetEvent(&interface->nicRxEvent);
   //ENC624J600 transmitter is now ready to send
   osSetEvent(&interface->nicTxEvent);

#endif
   //Successful initialization
   return NO_ERROR;

err_extInt :
	interface->busDriver = NULL;

	free(Drive_PSPDriver[ interface->instance] );
	return ERROR_T(init_createclass_fail);

}

err_t enc624j600destory(NetInterface *interface)
{
	interface->extIntDriver->deatory( interface->extIntDriver);
#ifndef DEBUG_ONLY_GPIO_INIT
	interface->busDriver->destory( interface->busDriver);
	interface->busDriver = NULL;
#endif
	interface->extIntDriver = NULL;
	return NO_ERROR;

}


/**
 * @brief ENC624J600 controller reset
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

//#define DEBUG_PSP_WR

err_t enc624j600SoftReset(NetInterface *interface)
{
#define EUDAST_TEST_VAL 0x1234
   //Wait for the SPI interface to be ready
   do
   {
      //Write 0x1234 to EUDAST

      WriteReg[I_type](interface, ENC624J600_REG_EUDAST, EUDAST_TEST_VAL);
      //Read back register and check contents
#ifdef DEBUG_PSP_WR
      delay(1);
	}while(1);
#else
   } while(ReadReg[ I_type](interface, ENC624J600_REG_EUDAST) != EUDAST_TEST_VAL);
#endif


   //Poll CLKRDY and wait for it to become set
   while(!( ReadReg[ I_type](interface, ENC624J600_REG_ESTAT) & ESTAT_CLKRDY));


   //Issue a system reset command by setting ETHRST
   SetBit[ I_type](interface, ENC624J600_REG_ECON2, ECON2_ETHRST);
   //Wait at least 25us for the reset to take place
   sleep(1);

   //Read EUDAST to confirm that the system reset took place.
   //EUDAST should have reverted back to its reset default
   if(ReadReg[ I_type](interface, ENC624J600_REG_EUDAST) != 0x0000)
      return EXIT_FAILURE;

   //Wait at least 256us for the PHY registers and PHY
   //status bits to become available
   sleep(1);

   //The controller is now ready to accept further commands
   return EXIT_SUCCESS;
}


/**
 * @brief ENC624J600 timer handler
 * @param[in] interface Underlying network interface
 **/

void enc624j600Tick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void enc624j600EnableIrq(NetInterface *interface)
{
   //Enable interrupts
   interface->extIntDriver->enableIrq( interface->extIntDriver);
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void enc624j600DisableIrq(NetInterface *interface)
{
   //Disable interrupts
   interface->extIntDriver->disableIrq( interface->extIntDriver);
}


/**
 * @brief ENC624J600 interrupt service routine
 * @param[in] interface Underlying network interface
 * @return TRUE if a higher priority task must be woken. Else FALSE is returned
 **/


bool enc624j600IrqHandler(void *arg)
{
   bool flag;
   uint16_t status;
   NetInterface *interface = ( NetInterface *)arg;
   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Clear the INTIE bit, immediately after an interrupt event
   ClearBit[ I_type](interface, ENC624J600_REG_EIE, EIE_INTIE);

   //Read interrupt status register
   status = ReadReg[ I_type](interface, ENC624J600_REG_EIR);

   //Link status change?
   if(status & EIR_LINKIF)
   {
      //Disable LINKIE interrupt
      ClearBit[ I_type](interface, ENC624J600_REG_EIE, EIE_LINKIE);
      //Notify the user that the link state has changed
      flag |= osSetEventFromIsr(&interface->nicRxEvent);
   }
   //Packet received?
   if(status & EIR_PKTIF)
   {
      //Disable PKTIE interrupt
      ClearBit[ I_type](interface, ENC624J600_REG_EIE, EIE_PKTIE);
      //Notify the user that a packet has been received
      flag |= osSetEventFromIsr(&interface->nicRxEvent);
   }
   //Packet transmission complete?
   if(status & (EIR_TXIF | EIR_TXABTIF))
   {
      //Notify the user that the transmitter is ready to send
      flag |= osSetEventFromIsr(&interface->nicTxEvent);
      //Clear interrupt flag
      ClearBit[ I_type](interface, ENC624J600_REG_EIR, EIR_TXIF | EIR_TXABTIF);
   }

   //Once the interrupt has been serviced, the INTIE bit
   //is set again to re-enable interrupts
   SetBit[ I_type](interface, ENC624J600_REG_EIE, EIE_INTIE);

   //A higher priority task must be woken?
   return flag;
}


/**
 * @brief ENC624J600 event handler
 * @param[in] interface Underlying network interface
 **/

void enc624j600EventHandler(NetInterface *interface)
{
   err_t error;
   uint16_t status;
   size_t length;




   //Read interrupt status register
   status = ReadReg[ I_type](interface, ENC624J600_REG_EIR);

   //Check whether the link state has changed
   if(status & EIR_LINKIF)
   {
      //Clear interrupt flag
      ClearBit[ I_type](interface, ENC624J600_REG_EIR, EIR_LINKIF);
      //Read Ethernet status register
      status = ReadReg[ I_type](interface, ENC624J600_REG_ESTAT);

      //Check link state
      if(status & ESTAT_PHYLNK)
      {
         //Link is up
         interface->linkState = TRUE;

         //Read PHY status register 3

         status = enc624j600ReadPhyReg(interface, ENC624J600_PHY_REG_PHSTAT3);
         //Get current speed
         interface->speed100 = (status & PHSTAT3_SPDDPX1) ? TRUE : FALSE;
         //Determine the new duplex mode
         interface->fullDuplex = (status & PHSTAT3_SPDDPX2) ? TRUE : FALSE;

         //Configure MAC duplex mode for proper operation
         enc624j600ConfigureDuplexMode(interface);

         //Display link state
         TRACE_INFO("Link is up (%s)... ", interface->name);

         //Display actual speed and duplex mode
         TRACE_INFO("%s %s\r\n",
            interface->speed100 ? "100BASE-TX" : "10BASE-T",
            interface->fullDuplex ? "Full-Duplex" : "Half-Duplex");
      }
      else
      {
         //Link is down
         interface->linkState = FALSE;
         //Display link state
         TRACE_INFO("Link is down (%s)...\r\n", interface->name);
      }

      //Process link state change event
      nicNotifyLinkChange(interface);			//sundh: delay
   }


   //Check whether a packet has been received?
   if(status & EIR_PKTIF)
   {
      //Clear interrupt flag
      ClearBit[ I_type](interface, ENC624J600_REG_EIR, EIR_PKTIF);

      //Process all pending packets
      do
      {
         //Read incoming packet
    	  if( interface->ethFrame == NULL)
    		  break;
         error = enc624_PSP_ReceivePacket(interface,
            interface->ethFrame, ETH_MAX_FRAME_SIZE, &length);

         //Check whether a valid packet has been received
         if(!error)
         {
            //Pass the packet to the upper layer
            nicProcessPacket(interface, interface->ethFrame, length);
         }

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable LINKIE and PKTIE interrupts
   SetBit[ I_type](interface, ENC624J600_REG_EIE, EIE_LINKIE | EIE_PKTIE);
}


/**
 * @brief Configure multicast MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

err_t enc624j600SetMacFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint16_t hashTable[4];

   //Debug message
   TRACE_INFO("Updating ENC624J600 hash table...\r\n");

   //Clear hash table
   memset(hashTable, 0, sizeof(hashTable));

   //The MAC filter table contains the multicast MAC addresses
   //to accept when receiving an Ethernet frame
   for(i = 0; i < interface->macFilterSize; i++)
   {
      //Compute CRC over the current MAC address
      crc = enc624j600CalcCrc(&interface->macFilter[i].addr, sizeof(MacAddr));
      //Calculate the corresponding index in the table
      k = (crc >> 23) & 0x3F;
      //Update hash table contents
      hashTable[k / 16] |= (1 << (k % 16));
   }

   //Write the hash table to the ENC624J600 controller
   WriteReg[I_type](interface, ENC624J600_REG_EHT1, hashTable[0]);
   WriteReg[I_type](interface, ENC624J600_REG_EHT2, hashTable[1]);
   WriteReg[I_type](interface, ENC624J600_REG_EHT3, hashTable[2]);
   WriteReg[I_type](interface, ENC624J600_REG_EHT4, hashTable[3]);

   //Debug message
   TRACE_INFO("  EHT1 = %04x \" PRIX16 \"\r\n", ReadReg[ I_type](interface, ENC624J600_REG_EHT1));
   TRACE_INFO("  EHT2 = %04x \" PRIX16 \"\r\n", ReadReg[ I_type](interface, ENC624J600_REG_EHT2));
   TRACE_INFO("  EHT3 = %04x \" PRIX16 \"\r\n", ReadReg[ I_type](interface, ENC624J600_REG_EHT3));
   TRACE_INFO("  EHT4 = %04x \" PRIX16 \"\r\n", ReadReg[ I_type](interface, ENC624J600_REG_EHT4));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send a packet
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the data to send
 * @param[in] offset Offset to the first data byte
 * @return Error code
 **/

err_t enc624j600SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset)
{
   size_t length;
   uint16_t			point;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > 1536)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_T( ERROR_INVALID_LENGTH);
   }

   //Make sure the link is up before transmitting the frame
   if(!interface->linkState)
   {
      //The transmitter can accept another packet
      osSetEventFromIsr(&interface->nicTxEvent);
      //Drop current packet
      return NO_ERROR;
   }

   //Ensure that the transmitter is ready to send
   if(ReadReg[ I_type](interface, ENC624J600_REG_ECON1) & ECON1_TXRTS)
      return ERROR_FAILURE;


   point = ENC624J600_TX_BUFFER_START;
   enc624_PSP_WriteSram(interface, &point, buffer, offset);

   //Point to the SRAM buffer
//   WriteReg[I_type](interface, ENC624J600_REG_EGPWRPT, ENC624J600_TX_BUFFER_START);
   //Copy the packet to the SRAM buffer
//   enc624j600WriteBuffer(interface, ENC624J600_CMD_WGPDATA, buffer, offset);


   //Program ETXST to the start address of the packet
   WriteReg[I_type](interface, ENC624J600_REG_ETXST, ENC624J600_TX_BUFFER_START);
   //Program ETXLEN with the length of data copied to the memory
   WriteReg[I_type](interface, ENC624J600_REG_ETXLEN, length);

   //Clear TXIF and TXABTIF interrupt flags
   ClearBit[ I_type](interface, ENC624J600_REG_EIR, EIR_TXIF | EIR_TXABTIF);
   //Set the TXRTS bit to initiate transmission
   SetBit[ I_type](interface, ENC624J600_REG_ECON1, ECON1_TXRTS);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @param[out] buffer Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] length Number of bytes that have been received
 * @return Error code
 **/
err_t enc624_PSP_ReceivePacket(NetInterface *interface,
   uint8_t *buffer, size_t size, size_t *length)
{
	err_t error;


	Enc624j600Context *context;
	uint16_t			point;

	//Point to the driver context
	context = (Enc624j600Context *) interface->nicContext;
	point = context->nextPacketPointer;
	//Verify that a packet is waiting by ensuring that PKTCNT is non-zero
	if(ReadReg[ I_type](interface, ENC624J600_REG_ESTAT) & ESTAT_PKTCNT)
	{
		struct rsv rsv;
//		uint16_t n;
//		uint32_t status;
		enc624_PSP_ReadSram( interface, &point, (uint8_t *)&rsv, sizeof(rsv));
		//Read the first two bytes, which are the address of the next packet
//		enc624_PSP_ReadSram( interface, &point , (uint8_t *)&context->nextPacketPointer, 2);
//		//Get the length of the received frame in bytes
//		enc624_PSP_ReadSram(interface, &point , (uint8_t *) &n, 2);
//		//Read the receive status vector (RSV)
//		enc624_PSP_ReadSram(interface,  &point, (uint8_t *) &status, 4);

	  //Make sure no error occurred
	  if( rsv.rxstat & RSV_RECEIVED_OK && rsv.len <= enc624j600Driver.mtu)
	  {
		 //Limit the number of data to read
		 size = MIN(size, rsv.len);
		 //Read the Ethernet frame
		 enc624_PSP_ReadSram(interface, &point, buffer, size);

		 //Total number of bytes that have been received
		 *length = size;
		 //Packet successfully received
		 error = NO_ERROR;
	  }
	  else
	  {
		 //The received packet contains an error
		 error = ERROR_INVALID_PACKET;
	  }
	  context->nextPacketPointer = rsv.next_packet;
	  //Update the ERXTAIL pointer value to the point where the packet
	  //has been processed, taking care to wrap back at the end of the
	  //received memory buffer
	  if( rsv.next_packet == ENC624J600_RX_BUFFER_START)
		 WriteReg[I_type](interface, ENC624J600_REG_ERXTAIL, ENC624J600_RX_BUFFER_STOP);
	  else
		 WriteReg[I_type](interface, ENC624J600_REG_ERXTAIL, rsv.next_packet - 2);


	  //Set PKTDEC to decrement the PKTCNT bits
	  SetBit[ I_type](interface, ENC624J600_REG_ECON1, ECON1_PKTDEC);
	}
	else
	{
	  //No more data in the receive buffer
	  error = ERROR_BUFFER_EMPTY;
	}

	//Return status code
	return error;

}

err_t enc624j600ReceivePacket(NetInterface *interface,
   uint8_t *buffer, size_t size, size_t *length)
{
   err_t error;
   uint16_t n;
   uint32_t status;
   Enc624j600Context *context;

   //Point to the driver context
   context = (Enc624j600Context *) interface->nicContext;

   //Verify that a packet is waiting by ensuring that PKTCNT is non-zero
   if(ReadReg[ I_type](interface, ENC624J600_REG_ESTAT) & ESTAT_PKTCNT)
   {
      //Point to the next packet
      WriteReg[I_type](interface, ENC624J600_REG_ERXRDPT, context->nextPacketPointer);
      //Read the first two bytes, which are the address of the next packet
      enc624j600ReadBuffer(interface, ENC624J600_CMD_RRXDATA, (uint8_t *) &context->nextPacketPointer, 2);
      //Get the length of the received frame in bytes
      enc624j600ReadBuffer(interface, ENC624J600_CMD_RRXDATA, (uint8_t *) &n, 2);
      //Read the receive status vector (RSV)
      enc624j600ReadBuffer(interface, ENC624J600_CMD_RRXDATA, (uint8_t *) &status, 4);

      //Make sure no error occurred
      if(status & RSV_RECEIVED_OK)
      {
         //Limit the number of data to read
         size = MIN(size, n);
         //Read the Ethernet frame
         enc624j600ReadBuffer(interface, ENC624J600_CMD_RRXDATA, buffer, size);

         //Total number of bytes that have been received
         *length = size;
         //Packet successfully received
         error = NO_ERROR;
      }
      else
      {
         //The received packet contains an error
         error = ERROR_INVALID_PACKET;
      }

      //Update the ERXTAIL pointer value to the point where the packet
      //has been processed, taking care to wrap back at the end of the
      //received memory buffer
      if(context->nextPacketPointer == ENC624J600_RX_BUFFER_START)
         WriteReg[I_type](interface, ENC624J600_REG_ERXTAIL, ENC624J600_RX_BUFFER_STOP);
      else
         WriteReg[I_type](interface, ENC624J600_REG_ERXTAIL, context->nextPacketPointer - 2);

      //Set PKTDEC to decrement the PKTCNT bits
      SetBit[ I_type](interface, ENC624J600_REG_ECON1, ECON1_PKTDEC);
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Return status code
   return error;
}


/**
 * @brief Configure duplex mode
 * @param[in] interface Underlying network interface
 **/

void enc624j600ConfigureDuplexMode(NetInterface *interface)
{
   uint16_t duplexMode;

   //Determine the new duplex mode by reading the PHYDPX bit
   duplexMode = ReadReg[ I_type](interface, ENC624J600_REG_ESTAT) & ESTAT_PHYDPX;

   //Full-duplex mode?
   if(duplexMode)
   {
      //Configure the FULDPX bit to match the current duplex mode
      WriteReg[I_type](interface, ENC624J600_REG_MACON2, MACON2_DEFER |
         MACON2_PADCFG2 | MACON2_PADCFG0 | MACON2_TXCRCEN | MACON2_R1 | MACON2_FULDPX);
      //Configure the Back-to-Back Inter-Packet Gap register
      WriteReg[I_type](interface, ENC624J600_REG_MABBIPG, 0x15);
   }
   //Half-duplex mode?
   else
   {
      //Configure the FULDPX bit to match the current duplex mode
      WriteReg[I_type](interface, ENC624J600_REG_MACON2, MACON2_DEFER |
         MACON2_PADCFG2 | MACON2_PADCFG0 | MACON2_TXCRCEN | MACON2_R1);
      //Configure the Back-to-Back Inter-Packet Gap register
      WriteReg[I_type](interface, ENC624J600_REG_MABBIPG, 0x12);
   }
}


/**
 * @brief Write ENC624J600 register
 * @param[in] interface Underlying network interface
 * @param[in] address Register address
 * @param[in] data Register value
 **/

void enc624_PSP_WriteReg(NetInterface *interface, uint8_t address, uint16_t data)
{
	interface->busDriver->write_u8( interface->busDriver, address + 0x7e00, LSB(data));
	interface->busDriver->write_u8( interface->busDriver, address + 0x7e00 + 1 , MSB(data));
}
void enc624j600WriteReg(NetInterface *interface, uint8_t address, uint16_t data)
{
	//todo SPI接口根据后期需要再去实现
   //Pull the CS pin low
//   interface->busDriver->assertCs(interface->busDriver);
//
//   //Write opcode
//   interface->busDriver->write_u8( interface->busDriver, ENC624J600_CMD_WCRU);
//   //Write register address
//   interface->busDriver->write_u8( interface->busDriver, address);
//
//   //Write register value
//   interface->busDriver->write_u8( interface->busDriver, LSB(data));
//   interface->busDriver->write_u8( interface->busDriver, MSB(data));
//
//
//   //Terminate the operation by raising the CS pin
//   interface->busDriver->deassertCs( interface->busDriver);
//


}


/**
 * @brief Read ENC624J600 register
 * @param[in] interface Underlying network interface
 * @param[in] address Register address
 * @return Register value
 **/
uint16_t enc624_PSP_ReadReg(NetInterface *interface, uint8_t address)
{
	uint16_t data = 0;
	data = interface->busDriver->read_u8( interface->busDriver, ( address + 0x7e00));
	data |= interface->busDriver->read_u8( interface->busDriver, ( address + 0x7e00 + 1)) << 8 ;
	return data;

}
//uint16_t enc624j600ReadReg(NetInterface *interface, uint8_t address)
//{
	//todo SPI接口根据后期需要再去实现
//   uint16_t data = 0;


   //Pull the CS pin low
//   interface->busDriver->assertCs( interface->busDriver);
//
//   //Write opcode
//   interface->busDriver->write_u8( interface->busDriver, ENC624J600_CMD_RCRU);
//   //Write register address
//   interface->busDriver->write_u8( interface->busDriver, address);
//   //Read the lower 8 bits of data
//   data = interface->busDriver->read_u8( interface->busDriver, 0x00);
//   //Read the upper 8 bits of data
//   data |= interface->busDriver->read_u8( interface->busDriver, 0x00) << 8;
//
//   //Terminate the operation by raising the CS pin
//   interface->busDriver->deassertCs( interface->busDriver);

   //Return register contents

//}


/**
 * @brief Write PHY register
 * @param[in] interface Underlying network interface
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void enc624j600WritePhyReg(NetInterface *interface, uint8_t address, uint16_t data)
{
   //Write the address of the PHY register to write to
   WriteReg[I_type](interface, ENC624J600_REG_MIREGADR, MIREGADR_R8 | address);
   //Write the 16 bits of data into the MIWR register
   WriteReg[I_type](interface, ENC624J600_REG_MIWR, data);

   //Wait until the PHY register has been written
   while(ReadReg[ I_type](interface, ENC624J600_REG_MISTAT) & MISTAT_BUSY);
}


/**
 * @brief Read PHY register
 * @param[in] interface Underlying network interface
 * @param[in] address PHY register address
 * @return Register value
 **/

uint16_t enc624j600ReadPhyReg(NetInterface *interface, uint8_t address)
{
   //Write the address of the PHY register to read from
   WriteReg[I_type](interface, ENC624J600_REG_MIREGADR, MIREGADR_R8 | address);
   //Start read operation
   WriteReg[I_type](interface, ENC624J600_REG_MICMD, MICMD_MIIRD);

   //Wait at least 25.6us before polling the BUSY bit
//   usleep(100);
   //Wait for the read operation to complete
   while(ReadReg[ I_type](interface, ENC624J600_REG_MISTAT) & MISTAT_BUSY);

   //Clear command register
   WriteReg[I_type](interface, ENC624J600_REG_MICMD, 0x00);

   //Return register contents
   return ReadReg[ I_type](interface, ENC624J600_REG_MIRD);
}


/**
 * @brief Write SRAM buffer
 * @param[in] interface Underlying network interface
 * @param[in] opcode SRAM buffer operation
 * @param[in] buffer Multi-part buffer containing the data to be written
 * @param[in] offset Offset to the first data byte
 **/

void enc624_PSP_WriteSram(NetInterface *interface,
   uint16_t *address, const NetBuffer *buffer, size_t offset)
{
	int i;
//   for(i = 0; i < buffer->chunkCount; i++)
//   {
//      //Is there any data to copy from the current chunk?
//      if(offset < buffer->chunk[i].length)
//      {
//         //Point to the first byte to be read
//         p = (uint8_t *) buffer->chunk[i].address + offset;
//         //Compute the number of bytes to copy at a time
//         n = buffer->chunk[i].length - offset;
//
//         //Copy data to SRAM buffer
//         for(j = 0; j < n; j++)
//            interface->busDriver->write_u8( interface->busDriver, p[j]);
//
//         //Process the next block from the start
//         offset = 0;
//      }
//      else
//      {
//         //Skip the current chunk
//         offset -= buffer->chunk[i].length;
//      }
//   }


	for( i = 0; i < buffer->len; i ++) {
		interface->busDriver->write_u8( interface->busDriver, *address, buffer->data[i + offset]);
		*address = *address + 1;
	}
}

void enc624j600WriteBuffer(NetInterface *interface,
   uint8_t opcode, const NetBuffer *buffer, size_t offset)
{

//sundh delay
//   uint_t i;
//   size_t j;
//   size_t n;
//   uint8_t *p;
//
//   //Pull the CS pin low
//   interface->busDriver->assertCs( interface->busDriver);
//
//   //Write opcode
//   interface->busDriver->write_u8( interface->busDriver, opcode);
//
//
   //Loop through data chunks
//   for(i = 0; i < buffer->chunkCount; i++)
//   {
//      //Is there any data to copy from the current chunk?
//      if(offset < buffer->chunk[i].length)
//      {
//         //Point to the first byte to be read
//         p = (uint8_t *) buffer->chunk[i].address + offset;
//         //Compute the number of bytes to copy at a time
//         n = buffer->chunk[i].length - offset;
//
//         //Copy data to SRAM buffer
//         for(j = 0; j < n; j++)
//            interface->busDriver->write_u8( interface->busDriver, p[j]);
//
//         //Process the next block from the start
//         offset = 0;
//      }
//      else
//      {
//         //Skip the current chunk
//         offset -= buffer->chunk[i].length;
//      }
//   }
//
//   //Terminate the operation by raising the CS pin
//   interface->busDriver->deassertCs( interface->busDriver);
}


/**
 * @brief Read SRAM buffer
 * @param[in] interface Underlying network interface
 * @param[in] opcode SRAM buffer operation
 * @param[in] data Buffer where to store the incoming data
 * @param[in] length Number of data to read
 **/

void enc624_PSP_ReadSram(NetInterface *interface,
   uint16_t *address, uint8_t *data, size_t length)
{
	int i;
	for( i = 0; i < length; i ++) {
		data[i] = interface->busDriver->read_u8( interface->busDriver, *address);
		*address = *address + 1;
	}
}

void enc624j600ReadBuffer(NetInterface *interface,
   uint8_t opcode, uint8_t *data, size_t length)
{
	//TODO
//   size_t i;
//
//   //Pull the CS pin low
//   interface->busDriver->assertCs( interface->busDriver);
//
//   //Write opcode
//   interface->busDriver->write_u8( interface->busDriver, opcode);
//
//   //Copy data from SRAM buffer
//   for(i = 0; i < length; i++)
//      data[i] = interface->busDriver->read_u8( interface->busDriver, 0x00);
//
//   //Terminate the operation by raising the CS pin
//   interface->busDriver->deassertCs( interface->busDriver);
}


/**
 * @brief Set bit field
 * @param[in] interface Underlying network interface
 * @param[in] address Register address
 * @param[in] mask Bits to set in the target register
 **/
void enc624_PSP_SetBit(NetInterface *interface, uint8_t address, uint16_t mask)
{

	uint16_t	reg = enc624_PSP_ReadReg( interface, address);
	reg |= mask;

	enc624_PSP_WriteReg( interface, address,  reg);
//	interface->busDriver->write_u8( interface->busDriver, address + 0x7e00, LSB(reg));
//	interface->busDriver->write_u8( interface->busDriver, address + 0x7e00 + 1, LSB(reg));

}
void enc624j600SetBit(NetInterface *interface, uint8_t address, uint16_t mask)
{
	//todo
//	//Pull the CS pin low
//	interface->busDriver->assertCs( interface->busDriver);
//
//	//Write opcode
//	interface->busDriver->write_u8( interface->busDriver, ENC624J600_CMD_BFSU);
//	//Write register address
//	interface->busDriver->write_u8( interface->busDriver, address);
//	//Write bit mask
//	interface->busDriver->write_u8( interface->busDriver, LSB(mask));
//	interface->busDriver->write_u8( interface->busDriver, MSB(mask));

//	//Terminate the operation by raising the CS pin
//	interface->busDriver->deassertCs( interface->busDriver);
}


/**
 * @brief Clear bit field
 * @param[in] interface Underlying network interface
 * @param[in] address Register address
 * @param[in] mask Bits to clear in the target register
 **/
void enc624_PSP_ClearBit(NetInterface *interface, uint8_t address, uint16_t mask)
{

	uint16_t	reg = enc624_PSP_ReadReg( interface, address);
	reg &= ~mask;
	enc624_PSP_WriteReg( interface, address,  reg);
//	interface->busDriver->write_u8( interface->busDriver, address + 0x7e00, LSB(reg));
//	interface->busDriver->write_u8( interface->busDriver, address + 0x7e00 + 1, LSB(reg));

}
void enc624j600ClearBit(NetInterface *interface, uint8_t address, uint16_t mask)
{
	//TODO later
	//Pull the CS pin low
//	interface->busDriver->assertCs( interface->busDriver);
//
//	//Write opcode
//	interface->busDriver->write_u8( interface->busDriver, ENC624J600_CMD_BFCU);
//	//Write register address
//	interface->busDriver->write_u8( interface->busDriver, address);
//	//Write bit mask
//	interface->busDriver->write_u8( interface->busDriver, LSB(mask));
//	interface->busDriver->write_u8( interface->busDriver, MSB(mask));
//
//	//Terminate the operation by raising the CS pin
//	interface->busDriver->deassertCs( interface->busDriver);
}


/**
 * @brief CRC calculation using the polynomial 0x4C11DB7
 * @param[in] data Pointer to the data over which to calculate the CRC
 * @param[in] length Number of bytes to process
 * @return Resulting CRC value
 **/

uint32_t enc624j600CalcCrc(const void *data, size_t length)
{
   uint_t i;
   uint_t j;

   //Point to the data over which to calculate the CRC
   const uint8_t *p = (uint8_t *) data;
   //CRC preset value
   uint32_t crc = 0xFFFFFFFF;

   //Loop through data
   for(i = 0; i < length; i++)
   {
      //The message is processed bit by bit
      for(j = 0; j < 8; j++)
      {
         //Update CRC value
         if(((crc >> 31) ^ (p[i] >> j)) & 0x01)
            crc = (crc << 1) ^ 0x04C11DB7;
         else
            crc = crc << 1;
      }
   }

   //Return CRC value
   return crc;
}


/**
 * @brief Dump registers for debugging purpose
 * @param[in] interface Underlying network interface
 **/


void enc624j600_print_reg(NetInterface *interface, uint16_t addr)
{
	TRACE_DEBUG( " print reg 0x%02x ", addr);
	TRACE_DEBUG("0x%04x ", ReadReg[ I_type](interface, addr));
	TRACE_DEBUG("\r\n");
}

void enc624j600DumpReg(NetInterface *interface)
{
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   uint8_t i;
   uint8_t bank;
   uint16_t address;
   if( I_type == ENC_INTERFACE_PSP)
   {
	   TRACE_DEBUG( " Dump reg \r\n");
	   for(i = 0; i < 0xa0; i += 2)
	   {
		   TRACE_DEBUG("%02x : ", i);
		   TRACE_DEBUG("0x%04x ", ReadReg[ I_type](interface, i));
		   TRACE_DEBUG("\r\n");
	   }
	   TRACE_DEBUG("\r\n");
   }
   else
   {
	   //Display header
	  TRACE_DEBUG("    Bank 0  Bank 1  Bank 2  Bank 3  Unbanked\r\n");

	  //Loop through register addresses
	  for(i = 0; i < 32; i += 2)
	  {
		 //Display register address
		 TRACE_DEBUG("%02d : ", i);

		 //Loop through bank numbers
		 for(bank = 0; bank < 5; bank++)
		 {
			//Format register address
			address = 0x7E00 | (bank << 5) | i;
			//Display register contents
			TRACE_DEBUG("0x%04x ", ReadReg[ I_type](interface, address));
		 }

		 //Jump to the following line
		 TRACE_DEBUG("\r\n");
	  }

	  //Terminate with a line feed
	  TRACE_DEBUG("\r\n");
   }

#endif
}


/**
 * @brief Dump PHY registers for debugging purpose
 * @param[in] interface Underlying network interface
 **/

void enc624j600DumpPhyReg(NetInterface *interface)
{
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   uint8_t i;
   TRACE_DEBUG( " Dump phy reg \r\n");
   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register

      TRACE_DEBUG("%02x: 0x%04x \r\n", i, enc624j600ReadPhyReg(interface, i));

   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
#endif
}
