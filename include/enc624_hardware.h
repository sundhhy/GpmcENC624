#ifndef ENC624J600_H
#define ENC624J600_H
#include <stdio.h>
//#include "ethdefs.h"
#define UI08_t unsigned char
#define bool_t  char
#define UI16_t unsigned int
// SPI register definitions
#define regBank0 0x00
#define regBank1 0x20
#define regBank2 0x40
#define regBank3 0x60
#define TRUE        1
#define FALSE       0
#define regDefine(addr, bank) (addr | regBank##bank)
#define regGlobal 0x80
typedef struct EthernetFrame_s
{
    UI08_t dstMac[6];
    UI08_t srcMac[6];
    UI16_t type;
} EthernetFrame_t;
//typedef enum enc624j600RegistersMap_e
//{
    /********************** BANK 0 *********************/
  #define   ETXST                    regDefine(0x00, 0),
  #define   ETXLEN                   regDefine(0x02, 0),
  #define  ERXST                    regDefine(0x04, 0),
  #define  ERXTAIL                  regDefine(0x06, 0),
  #define   ERXHEAD                  regDefine(0x08, 0),
  #define  EDMAST                   regDefine(0x0A, 0),
  #define  EDMALEN                  regDefine(0x0C, 0),
  #define  EDMADST                  regDefine(0x0E, 0),
  #define  EDMACS                   regDefine(0x10, 0),
  #define  ETXSTAT                  regDefine(0x12, 0),
  #define  ETXWIRE                  regDefine(0x14, 0),

    /********************** BANK 1 *********************/
  #define  ETH1                     regDefine(0x00, 1),
  #define  ETH2                     regDefine(0x02, 1),
  #define  ETH3                     regDefine(0x04, 1),
  #define  ETH4                     regDefine(0x06, 1),
  #define  EPMM1                    regDefine(0x08, 1),
  #define  EPMM2                    regDefine(0x0A, 1),
  #define  EPMM3                    regDefine(0x0C, 1),
  #define  EPMM4                    regDefine(0x0E, 1),
  #define  EPMCS                    regDefine(0x10, 1),
  #define  EPMO                     regDefine(0x12, 1),
  #define  ERXFCON                  regDefine(0x14, 1),

    /********************** BANK 2 *********************/
  #define  MACON1                   regDefine(0x00, 2),
  #define  MACON2                   regDefine(0x02, 2),
  #define  MABBIPG                  regDefine(0x04, 2),
  #define  MAIPG                    regDefine(0x06, 2),
  #define  MACLCON                  regDefine(0x08, 2),
  #define  MAMXFL                   regDefine(0x0A, 2),
  #define MICMD                     regDefine(0x12, 2),
  #define  MIREGADR                 regDefine(0x14, 2),

    /********************** BANK 3 *********************/
  #define  MADDR3                   regDefine(0x00, 3),
  #define  MADDR2                   regDefine(0x02, 3),
  #define  MADDR1                   regDefine(0x04, 3),
  #define  MIWR                     regDefine(0x06, 3),
  #define  MIRD                     regDefine(0x08, 3),
  #define  MISTAT                   regDefine(0x0A, 3),
  #define  EPAUS                    regDefine(0x0C, 3),
  #define  ECON2                    regDefine(0x0E, 3),
  #define  ERXWM                    regDefine(0x10, 3),
  #define  EIE                      regDefine(0x12, 3),
  #define  EIDLED                   regDefine(0x14, 3),

    /**************** GENERAL BANK ********************/
  #define EUDAST                    regDefine(0x16, 0) | regGlobal,
  #define EUDAND                    regDefine(0x18, 0) | regGlobal,
  #define  ESTAT                    regDefine(0x1A, 0) | regGlobal,
  #define  EIR                      regDefine(0x1C, 0) | regGlobal,
  #define  ECON1                    regDefine(0x1E, 0) | regGlobal,
//} enc624j600RegistersMap_t;

//typedef enum enc624j600CommandSet_e
//{
  #define  BANK_SEL     0b11000000 // select ban
  #define  SETETHRST    0b11000110// reset ethernet controller?
  #define  FCDISABLE    0b11100000 // flow-control
  #define  FCSINGLE     0b11100010
  #define  FCMULTIPLE   0b11100100
  #define  FCCLEAR      0b11100110

  #define  SETPKTDEC    0b11001100 //decrement pkt counter
  #define  DMASTOP      0b11010010 // dma control
  #define  DMACKSUM     0b11011000
  #define  DMACKSUMS    0b11011010
  #define  DMACOPY      0b11011100
  #define  DMACOPYS     0b1101110

  #define  SETTXRTS     0b11010100 // enable tx packet
  #define  ENABLERX     0b11101000 // enable rx reception
  #define  DISABLERX    0b11101010 // disable rx reception

  #define  SETEIE       0b11101100 //set/clr interrupts
  #define  CLREIE       0b11101110

  #define  RBSEL        0b11001000 // get bank selection

    //GP = General Purpose
    //Rx = Receive buffer pointer
    //UDA = User-defined area
  #define  WGPRDPT    0b01100000 // Write/read E-GP-Read-Pointer
  #define  RGPRDPT    0b01100010

  #define  WRXRDPT     0b01100100 // Write/read E-Rx-Read-Pointer
  #define  RRXRDPT     0b01100110

  #define  WUDARDPT    0b01101000 // Write/read E-UDA-Read-Pointer
  #define  RUDARDPT    0b01101010

   #define WGPWRPT     0b01101100 // Write/read E-GP-Write-Pointer
  #define  RGPWRPT     0b01101110

  #define  WRXWRPT     0b01110000 // Write/read E-Rx-Write-Pointer
  #define  RRXWRPT     0b01110010

  #define  WUDAWRPT    0b01110100 // Write/read E-UDA-Write-Pointer
   #define RUDAWRPT    0b01110110

  #define  RCR        0b00000000 //000a aaaa
   #define WCR        0b01000000 //010a aaaa

  #define  RCRU       0b00100000
  #define  WCRU       0b00100010

  #define  BFS        0b10000000 // 100a aaaa
  #define  BFC        0b10100000  // 101a aaaa
  #define  BFSU       0b00100100
  #define  BFCU       0b00100110

    // Read/write data

    //GP = General Purpose
    //Rx = Receive buffer pointer
    //UDA = User-defined area
  #define  RGPDATA    0b00101000 //  data
  #define  WGPDATA    0b00101010

  #define  RRXDATA    0b00101100 // rx data
  #define  WRXDATA    0b00101110

  #define  RUDADATA   0b00110000 // uda data
  #define  WUDADATA   0b00110010

//} enc624j600CommandSet_t;

#define ENC624J600_RXBUF_START 0x3000

void enc624j600_delay(UI08_t d);
void enc624j600SpiCommand(UI08_t cmd, UI08_t* bf, UI08_t size);
void enc624j600_setBank(UI08_t bank);
UI08_t enc624j600_spi_TxRx(UI08_t d);
void enc624j600Initialize();

UI08_t enc624j600GetPacketCount();
bool_t enc624j600PacketPending();
bool_t enc624j600GetLinkStatus();
bool_t enc624j600GetTxBusyStatus();

void enc624j600RxFrame(UI08_t* packet, UI16_t length);
void enc624j600TxFrame(EthernetFrame_t* packet, UI16_t length);
void enc624j600TxReplyFrame(EthernetFrame_t* frame, UI16_t length);

UI08_t enc624j600SpiReadRegister8(UI08_t addr);
UI16_t enc624j600SpiReadRegister16(UI08_t addr);

void enc624j600SpiWriteRegister8(UI08_t addr, UI08_t value);
void enc624j600SpiWriteRegister16(UI08_t addr, UI16_t value);

void enc624j600SpiBSetRegister8(UI08_t addr, UI08_t value);
void enc624j600SpiBSetRegister16(UI08_t addr, UI16_t value);

void enc624j600SpiReadData(UI16_t ptr, UI08_t* bf, UI16_t size);
void enc624j600SpiReadRxData(UI16_t ptr, UI08_t* bf, UI16_t size);
void enc624j600SpiWriteData(UI16_t ptr, UI08_t* bf, UI16_t size);

void enc624j600SpiCommandTxBf(UI08_t cmd, UI08_t* bf, UI08_t size);
void enc624j600SpiCommandRxBf(UI08_t cmd, UI08_t* bf, UI08_t size);

/******************* SPI ********************/
#define L 0x55
#define H 0xAA

#define enc624j600CommandTxBf(CMD, BF, SIZE) (enc624j600SpiCommandTxBf(CMD, BF, SIZE));
#define enc624j600CommandRxBf(CMD, BF, SIZE) (enc624j600SpiCommandRxBf(CMD, BF, SIZE));

#define enc624j600Command(CMD, BF, SIZE) (enc624j600SpiCommand(CMD, BF, SIZE));
#define enc624j600ReadGpData(ADDR, BF, SIZE) enc624j600SpiReadData(ADDR, BF, SIZE);
#define enc624j600ReadRxData(ADDR, BF, SIZE) enc624j600SpiReadRxData(ADDR, BF, SIZE);
#define enc624j600WriteGpData(ADDR, BF, SIZE) enc624j600SpiWriteData(ADDR, BF, SIZE);

#define enc624j600ReadRegister8(ADDR, TYPE) ((TYPE == L) ? enc624j600SpiReadRegister8(ADDR) : enc624j600SpiReadRegister8(ADDR+1))
#define enc624j600ReadRegister16(ADDR) enc624j600SpiReadRegister16(ADDR)

#define enc624j600WriteRegister8(ADDR, TYPE, V) ((TYPE == L) ? enc624j600SpiWriteRegister8(ADDR, V) : enc624j600SpiWriteRegister8(ADDR+1, V))
#define enc624j600WriteRegister16(ADDR, V) enc624j600SpiWriteRegister16(ADDR, V)

#define enc624j600BitSetRegister8(ADDR, TYPE, MASK) ((TYPE == L) ? enc624j600SpiBSetRegister8(ADDR, MASK) : enc624j600SpiBSetRegister8(ADDR+1, MASK))


#endif