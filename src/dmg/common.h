// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : common.h
// Date : May 09, 2015
// Description : Common functions and definitions
//
// A bunch of consts
// Consts are quick references to commonly used memory locations 

#include "common/common.h"

#ifndef DMG_COMMON
#define DMG_COMMON

//ROM Header
const u16 ROM_COLOR = 0x143;
const u16 ROM_MBC = 0x147;
const u16 ROM_ROMSIZE = 0x148;
const u16 ROM_RAMSIZE = 0x149;

//Object Attribute Memory
const u16 OAM = 0xFE00;

//Joypad
const u16 REG_P1 = 0xFF00;

//Timer
const u16 REG_DIV = 0xFF04;
const u16 REG_TIMA = 0xFF05;
const u16 REG_TMA = 0xFF06;
const u16 REG_TAC = 0xFF07;

//Interrupt Flags
const u16 IF_FLAG = 0xFF0F;
const u16 IE_FLAG = 0xFFFF;

//Sound registers
const u16 NR10 = 0xFF10;
const u16 NR11 = 0xFF11;
const u16 NR12 = 0xFF12;
const u16 NR13 = 0xFF13;
const u16 NR14 = 0xFF14;

const u16 NR21 = 0xFF16;
const u16 NR22 = 0xFF17;
const u16 NR24 = 0xFF19;

const u16 NR30 = 0xFF1A;
const u16 NR31 = 0xFF1B;
const u16 NR32 = 0xFF1C;
const u16 NR33 = 0xFF1D;
const u16 NR34 = 0xFF1E;

const u16 NR41 = 0xFF20;
const u16 NR42 = 0xFF21;
const u16 NR43 = 0xFF22;
const u16 NR44 = 0xFF23;

const u16 NR50 = 0xFF24;
const u16 NR51 = 0xFF25;
const u16 NR52 = 0xFF26;

//Display registers
const u16 REG_LCDC = 0xFF40;
const u16 REG_STAT = 0xFF41;
const u16 REG_SY = 0xFF42;
const u16 REG_SX = 0xFF43;
const u16 REG_LY = 0xFF44;
const u16 REG_LYC = 0xFF45;
const u16 REG_DMA = 0xFF46;
const u16 REG_BGP = 0xFF47;
const u16 REG_OBP0 = 0xFF48;
const u16 REG_OBP1 = 0xFF49;
const u16 REG_WY = 0xFF4A;
const u16 REG_WX = 0xFF4B;

//Double Speed Control
const u16 REG_KEY1 = 0xFF4D;

//HDMA
const u16 REG_HDMA1 = 0xFF51;
const u16 REG_HDMA2 = 0xFF52;
const u16 REG_HDMA3 = 0xFF53;
const u16 REG_HDMA4 = 0xFF54;
const u16 REG_HDMA5 = 0xFF55;

//Video RAM Bank
const u16 REG_VBK = 0xFF4F;

//GBC palettes
const u16 REG_BCPS = 0xFF68;
const u16 REG_BCPD = 0xFF69;
const u16 REG_OCPS = 0xFF6A;
const u16 REG_OCPD = 0xFF6B;

//Working RAM Bank
const u16 REG_SVBK = 0xFF70;

#endif // DMG_COMMON 
