// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : common.h
// Date : December 02, 2020
// Description : Common functions and definitions
//
// A bunch of consts
// Consts are quick references to commonly used memory locations 

#include "common/common.h"

#ifndef PM_COMMON
#define PM_COMMON

//CPU Overflow
const s32 MIN_8 = -128;
const s32 MAX_8 = 127;
const s32 MIN_8_UNPACK = -8;
const s32 MAX_8_UNPACK = 7;
const s32 MIN_16 = -32768;
const s32 MAX_16 = 32767;

//ROM Header
const u32 MIN_GAME_CODE = 0x21AC;
const u32 MIN_GAME_TITLE = 0x21B0;

//Flag Register Masks
const u8 ZERO_FLAG = 0x01;
const u8 CARRY_FLAG = 0x02;
const u8 OVERFLOW_FLAG = 0x04;
const u8 NEGATIVE_FLAG = 0x08;
const u8 DECIMAL_FLAG = 0x10;
const u8 UNPACK_FLAG = 0x20;

//Interrupt Flags
const u32 SYSTEM_RESET_IRQ = 0x01;
const u32 PRC_COPY_IRQ = 0x08;
const u32 PRC_OVERFLOW_IRQ = 0x10;
const u32 TIMER2_UPPER_UNDERFLOW_IRQ = 0x20;
const u32 TIMER2_LOWER_UNDERFLOW_IRQ = 0x40;
const u32 TIMER1_UPPER_UNDERFLOW_IRQ = 0x80;
const u32 TIMER1_LOWER_UNDERFLOW_IRQ = 0x100;
const u32 TIMER3_UPPER_UNDERFLOW_IRQ = 0x200;
const u32 TIMER3_PIVOT_IRQ = 0x400;
const u32 TIMER_32HZ_IRQ = 0x800;
const u32 TIMER_8HZ_IRQ = 0x1000;
const u32 TIMER_2HZ_IRQ = 0x2000;
const u32 TIMER_1HZ_IRQ = 0x4000;
const u32 IR_RECEIVER_IRQ = 0x8000;
const u32 SHOCK_SENSOR_IRQ = 0x10000;
const u32 CART_EJECT_IRQ = 0x80000;
const u32 CART_IRQ = 0x100000;
const u32 POWER_KEY_IRQ = 0x200000;
const u32 RIGHT_KEY_IRQ = 0x400000;
const u32 LEFT_KEY_IRQ = 0x800000;
const u32 DOWN_KEY_IRQ = 0x1000000;
const u32 UP_KEY_IRQ = 0x2000000;
const u32 C_KEY_IRQ = 0x4000000;
const u32 B_KEY_IRQ = 0x8000000;
const u32 A_KEY_IRQ = 0x10000000;

//MMIO Registers
const u16 SYS_CNT1 = 0x2000;
const u16 SYS_CNT2 = 0x2001;
const u16 SYS_CNT3 = 0x2002;

const u16 SEC_CNT = 0x2008;
const u16 RTC_SEC_LO = 0x2009;
const u16 RTC_SEC_MID = 0x200A;
const u16 RTC_SEC_HI = 0x200B;

const u16 SYS_BATT = 0x2010;

const u16 TIMER1_SCALE = 0x2018;
const u16 TIMER1_OSC = 0x2019;

const u16 TIMER2_SCALE = 0x201A;
const u16 TIMER2_OSC = 0x201B;

const u16 TIMER3_SCALE = 0x201C;
const u16 TIMER3_OSC = 0x201D;

const u16 IRQ_PRI_1 = 0x2020;
const u16 IRQ_PRI_2 = 0x2021;
const u16 IRQ_PRI_3 = 0x2022;

const u16 IRQ_ENA_1 = 0x2023;
const u16 IRQ_ENA_2 = 0x2024;
const u16 IRQ_ENA_3 = 0x2025;
const u16 IRQ_ENA_4 = 0x2026;

const u16 IRQ_ACT_1 = 0x2027;
const u16 IRQ_ACT_2 = 0x2028;
const u16 IRQ_ACT_3 = 0x2029;
const u16 IRQ_ACT_4 = 0x202A;

const u16 TIMER1_CNT_LO = 0x2030;
const u16 TIMER1_CNT_HI = 0x2031;
const u16 TIMER1_PRESET_LO = 0x2032;
const u16 TIMER1_PRESET_HI = 0x2033;
const u16 TIMER1_PIVOT_LO = 0x2034;
const u16 TIMER1_PIVOT_HI = 0x2035;
const u16 TIMER1_COUNT_LO = 0x2036;
const u16 TIMER1_COUNT_HI = 0x2037;

const u16 TIMER2_CNT_LO = 0x2038;
const u16 TIMER2_CNT_HI = 0x2039;
const u16 TIMER2_PRESET_LO = 0x203A;
const u16 TIMER2_PRESET_HI = 0x203B;
const u16 TIMER2_PIVOT_LO = 0x203C;
const u16 TIMER2_PIVOT_HI = 0x203D;
const u16 TIMER2_COUNT_LO = 0x203E;
const u16 TIMER2_COUNT_HI = 0x203F;

const u16 TIMER256_CNT = 0x2040;
const u16 TIMER256_COUNT = 0x2041;

const u16 TIMER3_CNT_LO = 0x2048;
const u16 TIMER3_CNT_HI = 0x2049;
const u16 TIMER3_PRESET_LO = 0x204A;
const u16 TIMER3_PRESET_HI = 0x204B;
const u16 TIMER3_PIVOT_LO = 0x204C;
const u16 TIMER3_PIVOT_HI = 0x204D;
const u16 TIMER3_COUNT_LO = 0x204E;
const u16 TIMER3_COUNT_HI = 0x204F;

const u16 PM_KEYPAD = 0x2052;

const u16 PM_IO_DIR = 0x2060;
const u16 PM_IO_DATA = 0x2061;

const u16 PM_AUDIO_VOLUME = 0x2071;

const u16 PRC_MODE = 0x2080;
const u16 PRC_RATE = 0x2081;
const u16 PRC_MAP_LO = 0x2082;
const u16 PRC_MAP_MID = 0x2083;
const u16 PRC_MAP_HI = 0x2084;
const u16 PRC_SY = 0x2085;
const u16 PRC_SX = 0x2086;
const u16 PRC_SPR_LO = 0x2087;
const u16 PRC_SPR_MID = 0x2088;
const u16 PRC_SPR_HI = 0x2089;
const u16 PRC_CNT = 0x208A;

const u16 MIN_LCD_CNT = 0x20FE;
const u16 MIN_LCD_DATA = 0x20FF;

const float PRC_COUNT_TIME = 55634 / 65.0;

#endif // PM_COMMON 
