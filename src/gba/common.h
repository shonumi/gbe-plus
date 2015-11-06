// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : common.h
// Date : April 09, 2014
// Description : Common functions and definitions
//
// Consts are quick references to commonly used memory locations 

#include "common/common.h"

#ifndef GBA_COMMON
#define GBA_COMMON

/* Display Registers */
const u32 DISPCNT = 0x4000000;
const u32 DISPSTAT = 0x4000004;
const u32 VCOUNT = 0x4000006;
const u32 BG0CNT = 0x4000008;
const u32 BG1CNT = 0x400000A;
const u32 BG2CNT = 0x400000C;
const u32 BG3CNT = 0x400000E;

const u32 BG0HOFS = 0x4000010;
const u32 BG0VOFS = 0x4000012;
const u32 BG1HOFS = 0x4000014;
const u32 BG1VOFS = 0x4000016;
const u32 BG2HOFS = 0x4000018;
const u32 BG2VOFS = 0x400001A;
const u32 BG3HOFS = 0x400001C;
const u32 BG3VOFS = 0x400001E;

const u32 BG2PA = 0x4000020;
const u32 BG2PB = 0x4000022;
const u32 BG2PC = 0x4000024;
const u32 BG2PD = 0x4000026;

const u32 BG2X_L = 0x4000028;
const u32 BG2X_H = 0x400002A;
const u32 BG2Y_L = 0x400002C;
const u32 BG2Y_H = 0x400002E;

const u32 BG3PA = 0x4000030;
const u32 BG3PB = 0x4000032;
const u32 BG3PC = 0x4000034;
const u32 BG3PD = 0x4000036;

const u32 BG3X_L = 0x4000038;
const u32 BG3X_H = 0x400003A;
const u32 BG3Y_L = 0x400003C;
const u32 BG3Y_H = 0x400003E;

const u32 WIN0H = 0x4000040;
const u32 WIN1H = 0x4000042;

const u32 WIN0V = 0x4000044;
const u32 WIN1V = 0x4000046;

const u32 WININ = 0x4000048;
const u32 WINOUT = 0x400004A;

const u32 BLDCNT = 0x4000050;
const u32 BLDALPHA = 0x4000052;
const u32 BLDY = 0x4000054;

/* Sound */
const u32 SND1CNT_L = 0x4000060;
const u32 SND1CNT_H = 0x4000062;
const u32 SND1CNT_X = 0x4000064;

const u32 SND2CNT_L = 0x4000068;
const u32 SND2CNT_H = 0x400006C;

const u32 SND3CNT_L = 0x4000070;
const u32 SND3CNT_H = 0x4000072;
const u32 SND3CNT_X = 0x4000074;

const u32 SND4CNT_L = 0x4000078;
const u32 SND4CNT_H = 0x400007C;

const u32 SNDCNT_L = 0x4000080;
const u32 SNDCNT_H = 0x4000082;
const u32 SNDCNT_X = 0x4000084;

const u32 SNDBIAS = 0x4000088;

const u32 WAVERAM0_L = 0x4000090;
const u32 WAVERAM0_H = 0x4000092;
const u32 WAVERAM1_L = 0x4000094;
const u32 WAVERAM1_H = 0x4000096;
const u32 WAVERAM2_L = 0x4000098;
const u32 WAVERAM2_H = 0x400009A;
const u32 WAVERAM3_L = 0x400009C;
const u32 WAVERAM3_H = 0x400009E;

const u32 FIFO_A = 0x40000A0;
const u32 FIFO_B = 0x40000A4;

/* Timers */
const u32 TM0CNT_L = 0x4000100;
const u32 TM1CNT_L = 0x4000104;
const u32 TM2CNT_L = 0x4000108;
const u32 TM3CNT_L = 0x400010C;

const u32 TM0CNT_H = 0x4000102;
const u32 TM1CNT_H = 0x4000106;
const u32 TM2CNT_H = 0x400010A;
const u32 TM3CNT_H = 0x400010E;


/* Input */
const u32 KEYINPUT = 0x4000130;
const u32 KEYCNT = 0x4000132;

/* Interrupts */
const u32 REG_IE = 0x4000200;
const u32 REG_IF = 0x4000202;
const u32 REG_IME = 0x4000208;

/* Wait Control */
const u32 WAITCNT = 0x4000204;

/* DMA */
const u32 DMA0SAD = 0x40000B0;
const u32 DMA1SAD = 0x40000BC;
const u32 DMA2SAD = 0x40000C8;
const u32 DMA3SAD = 0x40000D4;

const u32 DMA0DAD = 0x40000B4;
const u32 DMA1DAD = 0x40000C0;
const u32 DMA2DAD = 0x40000CC;
const u32 DMA3DAD = 0x40000D8;

const u32 DMA0CNT_L = 0x40000B8;
const u32 DMA1CNT_L = 0x40000C4;
const u32 DMA2CNT_L = 0x40000D0;
const u32 DMA3CNT_L = 0x40000DC;

const u32 DMA0CNT_H = 0x40000BA;
const u32 DMA1CNT_H = 0x40000C6;
const u32 DMA2CNT_H = 0x40000D2;
const u32 DMA3CNT_H = 0x40000DE;

/* GPIO */
const u32 GPIO_DATA = 0x80000C4;
const u32 GPIO_DIRECTION = 0x80000C6;
const u32 GPIO_CNT = 0x80000C8;

/* Misc */
const u32 FLASH_RAM_CMD0 = 0xE005555;
const u32 FLASH_RAM_CMD1 = 0xE002AAA;
const u32 FLASH_RAM_SEC0 = 0xE000000;
const u32 FLASH_RAM_SEC1 = 0xE001000;
const u32 FLASH_RAM_SEC2 = 0xE002000;
const u32 FLASH_RAM_SEC3 = 0xE003000;
const u32 FLASH_RAM_SEC4 = 0xE004000;
const u32 FLASH_RAM_SEC5 = 0xE005000;
const u32 FLASH_RAM_SEC6 = 0xE006000;
const u32 FLASH_RAM_SEC7 = 0xE007000;
const u32 FLASH_RAM_SEC8 = 0xE008000;
const u32 FLASH_RAM_SEC9 = 0xE009000;
const u32 FLASH_RAM_SECA = 0xE00A000;
const u32 FLASH_RAM_SECB = 0xE00B000;
const u32 FLASH_RAM_SECC = 0xE00C000;
const u32 FLASH_RAM_SECD = 0xE00D000;
const u32 FLASH_RAM_SECE = 0xE00E000;
const u32 FLASH_RAM_SECF = 0xE00F000;

#endif // GBA_COMMON