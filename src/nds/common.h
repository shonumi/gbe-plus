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

//Display registers
const u32 NDS_DISPCNT = 0x4000000;
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

const u32 NDS_IE = 0x4000210;
const u32 NDS_IF = 0x4000214;

#endif // NDS_COMMON 
