// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : common.h
// Date : April 09, 2014
// Description : Common functions and definitions
//
// A bunch of typedefs
// Typedefs make handling GBA hardware easier than C++ types

#ifndef EMU_COMMON
#define EMU_COMMON

/* Byte Definitions */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long int s64;

/* ARM CPSR Flags */
const u32 CPSR_N_FLAG = 0x80000000;
const u32 CPSR_Z_FLAG = 0x40000000;
const u32 CPSR_C_FLAG = 0x20000000;
const u32 CPSR_V_FLAG = 0x10000000;
const u32 CPSR_Q_FLAG = 0x8000000;
const u32 CPSR_IRQ = 0x80;
const u32 CPSR_FIQ = 0x40;
const u32 CPSR_STATE = 0x20;
const u32 CPSR_MODE = 0x1F;

const u32 CPSR_MODE_USR = 0x10;
const u32 CPSR_MODE_FIQ = 0x11;
const u32 CPSR_MODE_IRQ = 0x12;
const u32 CPSR_MODE_SVC = 0x13;
const u32 CPSR_MODE_ABT = 0x17;
const u32 CPSR_MODE_UND = 0x1B;
const u32 CPSR_MODE_SYS = 0x1F;

const u32 SAVE_STATE_VERSION = 0x01;

#endif // EMU_COMMON