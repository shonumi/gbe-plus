// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : common.h
// Date : April 09, 2014
// Description : Common functions and definitions
//
// A bunch of typedefs and consts
// Typedefs make handling GBA hardware easier than C++ types
// Consts are quick references to commonly used memory locations 

/****** Common Function & Definitions ******/  

#ifndef GBA_COMMON
#define GBA_COMMON

/* Byte Definitions */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long int s64;

/* CPSR Flags */
const u32 CPSR_N_FLAG = 0x80000000;
const u32 CPSR_Z_FLAG = 0x40000000;
const u32 CPSR_C_FLAG = 0x20000000;
const u32 CPSR_V_FLAG = 0x10000000;
const u32 CPSR_Q_FLAG = 0x8000000;
const u32 CPSR_IRQ = 0x80;
const u32 CPSR_FIQ = 0x40;
const u32 CPSR_STATE = 0x20;
const u32 CPSR_MODE = 0x1F;

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

#endif // GBA_COMMON