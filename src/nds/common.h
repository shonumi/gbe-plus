// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : common.h
// Date : September 14, 2015
// Description : Common functions and definitions
//
// A bunch of consts
// Consts are quick references to commonly used memory locations 

#include "common/common.h"

#ifndef NDS_COMMON
#define NDS_COMMON

//Display A registers
const u32 NDS_DISPCNT_A = 0x4000000;
const u32 NDS_DISPSTAT = 0x4000004;
const u32 NDS_VCOUNT = 0x4000006;
const u32 NDS_BG0CNT = 0x4000008;
const u32 NDS_BG1CNT = 0x400000A;
const u32 NDS_BG2CNT = 0x400000C;
const u32 NDS_BG3CNT = 0x400000E;

const u32 NDS_BG0HOFS = 0x4000010;
const u32 NDS_BG0VOFS = 0x4000012;
const u32 NDS_BG1HOFS = 0x4000014;
const u32 NDS_BG1VOFS = 0x4000016;
const u32 NDS_BG2HOFS = 0x4000018;
const u32 NDS_BG2VOFS = 0x400001A;
const u32 NDS_BG3HOFS = 0x400001C;
const u32 NDS_BG3VOFS = 0x400001E;

const u32 NDS_BG2PA = 0x4000020;
const u32 NDS_BG2PB = 0x4000022;
const u32 NDS_BG2PC = 0x4000024;
const u32 NDS_BG2PD = 0x4000026;

const u32 NDS_BG2X_L = 0x4000028;
const u32 NDS_BG2X_H = 0x400002A;
const u32 NDS_BG2Y_L = 0x400002C;
const u32 NDS_BG2Y_H = 0x400002E;

const u32 NDS_BG3PA = 0x4000030;
const u32 NDS_BG3PB = 0x4000032;
const u32 NDS_BG3PC = 0x4000034;
const u32 NDS_BG3PD = 0x4000036;

const u32 NDS_BG3X_L = 0x4000038;
const u32 NDS_BG3X_H = 0x400003A;
const u32 NDS_BG3Y_L = 0x400003C;
const u32 NDS_BG3Y_H = 0x400003E;

const u32 NDS_WIN0H = 0x4000040;
const u32 NDS_WIN1H = 0x4000042;

const u32 NDS_WIN0V = 0x4000044;
const u32 NDS_WIN1V = 0x4000046;

const u32 NDS_WININ = 0x4000048;
const u32 NDS_WINOUT = 0x400004A;

const u32 NDS_BLDCNT = 0x4000050;
const u32 NDS_BLDALPHA = 0x4000052;
const u32 NDS_BLDY = 0x4000054;

//Display B registers
const u32 NDS_DISPCNT_B = 0x4001000;

//Misc Display registers
const u32 NDS_VRAMCNT_A = 0x4000240;

//Interrupt registers
const u32 NDS_IME = 0x4000208;
const u32 NDS_IE = 0x4000210;
const u32 NDS_IF = 0x4000214;

//DMA registers
const u32 NDS_DMA0SAD = 0x40000B0;
const u32 NDS_DMA0DAD = 0x40000B4;
const u32 NDS_DMA0CNT = 0x40000B8;

const u32 NDS_DMA1SAD = 0x40000BC;
const u32 NDS_DMA1DAD = 0x40000C0;
const u32 NDS_DMA1CNT = 0x40000C4;

const u32 NDS_DMA2SAD = 0x40000C8;
const u32 NDS_DMA2DAD = 0x40000CC;
const u32 NDS_DMA2CNT = 0x40000D0;

const u32 NDS_DMA3SAD = 0x40000D4;
const u32 NDS_DMA3DAD = 0x40000D8;
const u32 NDS_DMA3CNT = 0x40000DC;

const u32 NDS_DMA0FILL = 0x40000E0;
const u32 NDS_DMA1FILL = 0x40000E4;
const u32 NDS_DMA2FILL = 0x40000E8;
const u32 NDS_DMA3FILL = 0x40000EC;

//Input
const u32 NDS_KEYINPUT = 0x4000130;
const u32 NDS_KEYCNT = 0x4000132;
const u32 NDS_EXTKEYIN = 0x4000136;

//IPC
const u32 NDS_IPCSYNC = 0x4000180;
const u32 NDS_IPCFIFOCNT = 0x4000184;
const u32 NDS_IPCFIFOSND = 0x4000188;
const u32 NDS_IPCFIFORECV = 0x4100000;

//Math
const u32 NDS_DIVCNT = 0x4000280;
const u32 NDS_SQRTCNT = 0x40002B0;

#endif // NDS_COMMON 
