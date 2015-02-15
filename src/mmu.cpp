// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : April 22, 2014
// Description : Game Boy Advance memory manager unit
//
// Handles reading and writing bytes to memory locations

#include "mmu.h"

/****** MMU Constructor ******/
MMU::MMU() 
{
	reset();
}

/****** MMU Deconstructor ******/
MMU::~MMU() 
{ 
	memory_map.clear();
	std::cout<<"MMU::Shutdown\n"; 
}

/****** MMU Reset ******/
void MMU::reset()
{
	memory_map.clear();
	memory_map.resize(0x10000000, 0);

	eeprom.data.clear();
	eeprom.data.resize(0x200, 0);
	eeprom.size = 0x200;
	eeprom.size_lock = false;

	flash_ram.data.clear();
	flash_ram.bank = 0;
	flash_ram.current_command = 0;
	flash_ram.write_single_byte = false;
	flash_ram.switch_bank = false;
	flash_ram.grab_ids = false;
	flash_ram.next_write = false;

	flash_ram.data.resize(2);
	flash_ram.data[0].resize(0x10000, 0xFF);
	flash_ram.data[1].resize(0x10000, 0xFF);

	//HLE stuff
	memory_map[DISPCNT] = 0x80;
	write_u16(0x4000134, 0x8000);

	bios_lock = false;

	write_u32(0x18, 0xEA000042);
	write_u32(0x128, 0xE92D500F);
	write_u32(0x12C, 0xE3A00301);
	write_u32(0x130, 0xE28FE000);
	write_u32(0x134, 0xE510F004);
	write_u32(0x138, 0xE8BD500F);
	write_u32(0x13C, 0xE25EF004);

	bios_lock = true;

	//Default memory access timings (4, 2)
	n_clock = 4;
	s_clock = 2;

	dma[0].enable = dma[1].enable = dma[2].enable = dma[3].enable = false;
	dma[0].started = dma[1].started = dma[2].started = dma[3].started = false;

	current_save_type = NONE;

	g_pad = NULL;
	timer = NULL;

	std::cout<<"MMU::Initialized\n";
}

/****** Read byte from memory ******/
u8 MMU::read_u8(u32 address) const
{
	//Check for unused memory first
	if(address >= 0x10000000) { std::cout<<"Out of bounds read : 0x" << std::hex << address << "\n"; return 0; }

	//Read from FLASH RAM
	if(((current_save_type == FLASH_64) || (current_save_type == FLASH_128)) && (address >= 0xE000000) && (address <= 0xE00FFFF))
	{
		if((address == 0xE000000) && (current_save_type == FLASH_64) && (flash_ram.grab_ids)) { return 0x32; }
		else if((address == 0xE000000) && (current_save_type == FLASH_128) && (flash_ram.grab_ids)) { return 0xC2; }

		else if((address == 0xE000001) && (current_save_type == FLASH_64) && (flash_ram.grab_ids)) { return 0x1B; }
		else if((address == 0xE000001) && (current_save_type == FLASH_128) && (flash_ram.grab_ids)) { return 0x09; }

		return flash_ram.data[flash_ram.bank][(address & 0xFFFF)];
	}

	switch(address)
	{
		case TM0CNT_L:
			return (timer->at(0).counter & 0xFF);
			break;

		case TM0CNT_L+1:
			return (timer->at(0).counter >> 8);
			break;

		case TM1CNT_L:
			return (timer->at(1).counter & 0xFF);
			break;

		case TM1CNT_L+1:
			return (timer->at(1).counter >> 8);
			break;

		case TM2CNT_L:
			return (timer->at(2).counter & 0xFF);
			break;

		case TM2CNT_L+1:
			return (timer->at(2).counter >> 8);
			break;

		case TM3CNT_L:
			return (timer->at(3).counter & 0xFF);
			break;

		case TM3CNT_L+1:
			return (timer->at(3).counter >> 8);
			break;

		case KEYINPUT:
			return (g_pad->key_input & 0xFF);
			break;

		case KEYINPUT+1:
			return (g_pad->key_input >> 8);
			break;
		
		default:
			return memory_map[address];
	}
}

/****** Read 2 bytes from memory ******/
u16 MMU::read_u16(u32 address) const
{
	return ((read_u8(address+1) << 8) | read_u8(address)); 
}

/****** Read 4 bytes from memory ******/
u32 MMU::read_u32(u32 address) const
{
	return ((read_u8(address+3) << 24) | (read_u8(address+2) << 16) | (read_u8(address+1) << 8) | read_u8(address));
}

/****** Reads 2 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u16 MMU::read_u16_fast(u32 address) const
{
	return ((memory_map[address+1] << 8) | memory_map[address]);
}

/****** Reads 4 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u32 MMU::read_u32_fast(u32 address) const
{
	return ((memory_map[address+3] << 24) | (memory_map[address+2] << 16) | (memory_map[address+1] << 8) | memory_map[address]);
}

/****** Write byte into memory ******/
void MMU::write_u8(u32 address, u8 value)
{
	//Check for unused memory first
	if(address >= 0x10000000) { std::cout<<"Out of bounds write : 0x" << std::hex << address << "\n"; return; }

	//BIOS is read-only, prevent any attempted writes
	else if((address <= 0x3FF) && (bios_lock)) { return; }

	switch(address)
	{
		case DISPCNT:
		case DISPCNT+1:
			memory_map[address] = value;
			lcd_stat->display_control = ((memory_map[DISPCNT+1] << 8) | memory_map[DISPCNT]);
			lcd_stat->bg_mode = lcd_stat->display_control & 0x7;
			lcd_stat->frame_base = (memory_map[DISPCNT] & 0x10) ? 0x600A000 : 0x6000000;

			lcd_stat->window_enable[0] = (lcd_stat->display_control & 0x2000) ? true : false;
			lcd_stat->window_enable[1] = (lcd_stat->display_control & 0x4000) ? true : false;

			lcd_stat->bg_enable[0] = (lcd_stat->display_control & 0x100) ? true : false;
			lcd_stat->bg_enable[1] = (lcd_stat->display_control & 0x200) ? true : false;
			lcd_stat->bg_enable[2] = (lcd_stat->display_control & 0x400) ? true : false;
			lcd_stat->bg_enable[3] = (lcd_stat->display_control & 0x800) ? true : false;
			break;

		case BG0CNT:
		case BG0CNT+1:
			memory_map[address] = value;
			lcd_stat->bg_priority[0] = memory_map[BG0CNT] & 0x3;
			lcd_stat->bg_control[0] = ((memory_map[BG0CNT+1] << 8) | memory_map[BG0CNT]);
			lcd_stat->bg_depth[0] = (lcd_stat->bg_control[0] & 0x80) ? 8 : 4;
			lcd_stat->bg_size[0] = lcd_stat->bg_control[0] >> 14;

			lcd_stat->bg_base_map_addr[0] = 0x6000000 + (0x800 * ((lcd_stat->bg_control[0] >> 8) & 0x1F));
			lcd_stat->bg_base_tile_addr[0] = 0x6000000 + (0x4000 * ((lcd_stat->bg_control[0] >> 2) & 0x3));

			switch(lcd_stat->bg_control[0] >> 14)
			{
				case 0x0: lcd_stat->mode_0_width[0] = 256; lcd_stat->mode_0_height[0] = 256; break;
				case 0x1: lcd_stat->mode_0_width[0] = 512; lcd_stat->mode_0_height[0] = 256; break;
				case 0x2: lcd_stat->mode_0_width[0] = 256; lcd_stat->mode_0_height[0] = 512; break;
				case 0x3: lcd_stat->mode_0_width[0] = 512; lcd_stat->mode_0_height[0] = 512; break;
			}

			break;

		case BG1CNT:
		case BG1CNT+1:
			memory_map[address] = value;
			lcd_stat->bg_priority[1] = memory_map[BG1CNT] & 0x3;
			lcd_stat->bg_control[1] = ((memory_map[BG1CNT+1] << 8) | memory_map[BG1CNT]);
			lcd_stat->bg_depth[1] = (lcd_stat->bg_control[1] & 0x80) ? 8 : 4;
			lcd_stat->bg_size[1] = lcd_stat->bg_control[1] >> 14;

			lcd_stat->bg_base_map_addr[1] = 0x6000000 + (0x800 * ((lcd_stat->bg_control[1] >> 8) & 0x1F));
			lcd_stat->bg_base_tile_addr[1] = 0x6000000 + (0x4000 * ((lcd_stat->bg_control[1] >> 2) & 0x3));

			switch(lcd_stat->bg_control[1] >> 14)
			{
				case 0x0: lcd_stat->mode_0_width[1] = 256; lcd_stat->mode_0_height[1] = 256; break;
				case 0x1: lcd_stat->mode_0_width[1] = 512; lcd_stat->mode_0_height[1] = 256; break;
				case 0x2: lcd_stat->mode_0_width[1] = 256; lcd_stat->mode_0_height[1] = 512; break;
				case 0x3: lcd_stat->mode_0_width[1] = 512; lcd_stat->mode_0_height[1] = 512; break;
			}

			break;

		case BG2CNT:
		case BG2CNT+1:
			memory_map[address] = value;
			lcd_stat->bg_priority[2] = memory_map[BG2CNT] & 0x3;
			lcd_stat->bg_control[2] = ((memory_map[BG2CNT+1] << 8) | memory_map[BG2CNT]);
			lcd_stat->bg_depth[2] = (lcd_stat->bg_control[2] & 0x80) ? 8 : 4;
			lcd_stat->bg_size[2] = lcd_stat->bg_control[2] >> 14;
			lcd_stat->bg_params_update = true;

			lcd_stat->bg_base_map_addr[2] = 0x6000000 + (0x800 * ((lcd_stat->bg_control[2] >> 8) & 0x1F));
			lcd_stat->bg_base_tile_addr[2] = 0x6000000 + (0x4000 * ((lcd_stat->bg_control[2] >> 2) & 0x3));

			switch(lcd_stat->bg_control[2] >> 14)
			{
				case 0x0: lcd_stat->mode_0_width[2] = 256; lcd_stat->mode_0_height[2] = 256; break;
				case 0x1: lcd_stat->mode_0_width[2] = 512; lcd_stat->mode_0_height[2] = 256; break;
				case 0x2: lcd_stat->mode_0_width[2] = 256; lcd_stat->mode_0_height[2] = 512; break;
				case 0x3: lcd_stat->mode_0_width[2] = 512; lcd_stat->mode_0_height[2] = 512; break;
			}

			break;

		case BG3CNT:
		case BG3CNT+1:
			memory_map[address] = value;
			lcd_stat->bg_priority[3] = memory_map[BG3CNT] & 0x3;
			lcd_stat->bg_control[3] = ((memory_map[BG3CNT+1] << 8) | memory_map[BG3CNT]);
			lcd_stat->bg_depth[3] = (lcd_stat->bg_control[3] & 0x80) ? 8 : 4;
			lcd_stat->bg_size[3] = lcd_stat->bg_control[3] >> 14;

			lcd_stat->bg_base_map_addr[3] = 0x6000000 + (0x800 * ((lcd_stat->bg_control[3] >> 8) & 0x1F));
			lcd_stat->bg_base_tile_addr[3] = 0x6000000 + (0x4000 * ((lcd_stat->bg_control[3] >> 2) & 0x3));

			switch(lcd_stat->bg_control[3] >> 14)
			{
				case 0x0: lcd_stat->mode_0_width[3] = 256; lcd_stat->mode_0_height[3] = 256; break;
				case 0x1: lcd_stat->mode_0_width[3] = 512; lcd_stat->mode_0_height[3] = 256; break;
				case 0x2: lcd_stat->mode_0_width[3] = 256; lcd_stat->mode_0_height[3] = 512; break;
				case 0x3: lcd_stat->mode_0_width[3] = 512; lcd_stat->mode_0_height[3] = 512; break;
			}

			break;

		case BG0HOFS:
		case BG0HOFS+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x[0] = ((memory_map[BG0HOFS+1] << 8) | memory_map[BG0HOFS]) & 0x1FF;
			break;

		case BG0VOFS:
		case BG0VOFS+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y[0] = ((memory_map[BG0VOFS+1] << 8) | memory_map[BG0VOFS]) & 0x1FF;
			break;

		case BG1HOFS:
		case BG1HOFS+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x[1] = ((memory_map[BG1HOFS+1] << 8) | memory_map[BG1HOFS]) & 0x1FF;
			break;

		case BG1VOFS:
		case BG1VOFS+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y[1] = ((memory_map[BG1VOFS+1] << 8) | memory_map[BG1VOFS]) & 0x1FF;
			break;

		case BG2HOFS:
		case BG2HOFS+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x[2] = ((memory_map[BG2HOFS+1] << 8) | memory_map[BG2HOFS]) & 0x1FF;
			break;

		case BG2VOFS:
		case BG2VOFS+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y[2] = ((memory_map[BG2VOFS+1] << 8) | memory_map[BG2VOFS]) & 0x1FF;
			break;

		case BG3HOFS:
		case BG3HOFS+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x[3] = ((memory_map[BG3HOFS+1] << 8) | memory_map[BG3HOFS]) & 0x1FF;
			break;

		case BG3VOFS:
		case BG3VOFS+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y[3] = ((memory_map[BG3VOFS+1] << 8) | memory_map[BG3VOFS]) & 0x1FF;
			break;

		case WIN0H:
		case WIN0H+1:
			if(memory_map[address] == value) { return ; }

			memory_map[address] = value;
			lcd_stat->window_x1[0] = memory_map[WIN0H+1];
			lcd_stat->window_x2[0] = memory_map[WIN0H] + 1;

			if(lcd_stat->window_x2[0] > 240) { lcd_stat->window_x2[0] = 240; }
			if(lcd_stat->window_x2[0] < lcd_stat->window_x1[0]) { lcd_stat->window_x2[0] = lcd_stat->window_x1[0] = 240; }
			break;

		case WIN1H:
		case WIN1H+1:
			if(memory_map[address] == value) { return ; }

			memory_map[address] = value;
			lcd_stat->window_x1[1] = memory_map[WIN1H+1];
			lcd_stat->window_x2[1] = memory_map[WIN1H];

			if(lcd_stat->window_x2[1] > 240) { lcd_stat->window_x2[1] = 240; }
			if(lcd_stat->window_x2[1] < lcd_stat->window_x1[1]) { lcd_stat->window_x2[1] = lcd_stat->window_x1[1] = 240; }
			break;

		case WIN0V:
		case WIN0V+1:
			if(memory_map[address] == value) { return ; }

			memory_map[address] = value;
			lcd_stat->window_y1[0] = memory_map[WIN0V+1];
			lcd_stat->window_y2[0] = memory_map[WIN0V];

			if(lcd_stat->window_y2[0] > 160) { lcd_stat->window_y2[0] = 160; }
			if(lcd_stat->window_y2[0] < lcd_stat->window_y1[0]) { lcd_stat->window_y2[0] = lcd_stat->window_y1[0] = 160; }
			break;

		case WIN1V:
		case WIN1V+1:
			if(memory_map[address] == value) { return ; }

			memory_map[address] = value;
			lcd_stat->window_y1[1] = memory_map[WIN1V+1];
			lcd_stat->window_y2[1] = memory_map[WIN1V] + 1;

			if(lcd_stat->window_y2[1] > 160) { lcd_stat->window_y2[1] = 160; }
			if(lcd_stat->window_y2[1] < lcd_stat->window_y1[1]) { lcd_stat->window_y2[1] = lcd_stat->window_y1[1] = 160; }
			break;

		case WININ:
			memory_map[address] = value;
			lcd_stat->window_in_enable[0][0] = (value & 0x1) ? true : false;
			lcd_stat->window_in_enable[1][0] = (value & 0x2) ? true : false;
			lcd_stat->window_in_enable[2][0] = (value & 0x4) ? true : false;
			lcd_stat->window_in_enable[3][0] = (value & 0x8) ? true : false;
			lcd_stat->window_in_enable[4][0] = (value & 0x10) ? true : false;
			lcd_stat->window_in_enable[5][0] = (value & 0x20) ? true : false;
			break;

		case WININ+1:
			memory_map[address] = value;
			lcd_stat->window_in_enable[0][1] = (value & 0x1) ? true : false;
			lcd_stat->window_in_enable[1][1] = (value & 0x2) ? true : false;
			lcd_stat->window_in_enable[2][1] = (value & 0x4) ? true : false;
			lcd_stat->window_in_enable[3][1] = (value & 0x8) ? true : false;
			lcd_stat->window_in_enable[4][1] = (value & 0x10) ? true : false;
			lcd_stat->window_in_enable[5][1] = (value & 0x20) ? true : false;
			break;

		case WINOUT:
			memory_map[address] = value;
			lcd_stat->window_out_enable[0][0] = (value & 0x1) ? true : false;
			lcd_stat->window_out_enable[1][0] = (value & 0x2) ? true : false;
			lcd_stat->window_out_enable[2][0] = (value & 0x4) ? true : false;
			lcd_stat->window_out_enable[3][0] = (value & 0x8) ? true : false;
			lcd_stat->window_out_enable[4][0] = (value & 0x10) ? true : false;
			lcd_stat->window_out_enable[5][0] = (value & 0x20) ? true : false;
			break;

		case WINOUT+1:
			memory_map[address] = value;
			lcd_stat->window_out_enable[0][1] = (value & 0x1) ? true : false;
			lcd_stat->window_out_enable[1][1] = (value & 0x2) ? true : false;
			lcd_stat->window_out_enable[2][1] = (value & 0x4) ? true : false;
			lcd_stat->window_out_enable[3][1] = (value & 0x8) ? true : false;
			lcd_stat->window_out_enable[4][1] = (value & 0x10) ? true : false;
			lcd_stat->window_out_enable[5][1] = (value & 0x20) ? true : false;
			break;

		case BLDCNT:
			memory_map[address] = value;
			lcd_stat->sfx_target[0][0] = (value & 0x1) ? true : false;
			lcd_stat->sfx_target[1][0] = (value & 0x2) ? true : false;
			lcd_stat->sfx_target[2][0] = (value & 0x4) ? true : false;
			lcd_stat->sfx_target[3][0] = (value & 0x8) ? true : false;
			lcd_stat->sfx_target[4][0] = (value & 0x10) ? true : false;
			lcd_stat->sfx_target[5][0] = (value & 0x20) ? true : false;

			switch(value >> 6)
			{
				case 0x0: lcd_stat->current_sfx_type = NORMAL; break;
				case 0x1: lcd_stat->current_sfx_type = ALPHA_BLEND; break;
				case 0x2: lcd_stat->current_sfx_type = BRIGHTNESS_UP; break;
				case 0x3: lcd_stat->current_sfx_type = BRIGHTNESS_DOWN; break;
			}			

			break;

		case BLDCNT+1:
			memory_map[address] = value;
			lcd_stat->sfx_target[0][0] = (value & 0x1) ? true : false;
			lcd_stat->sfx_target[1][0] = (value & 0x2) ? true : false;
			lcd_stat->sfx_target[2][0] = (value & 0x4) ? true : false;
			lcd_stat->sfx_target[3][0] = (value & 0x8) ? true : false;
			lcd_stat->sfx_target[4][0] = (value & 0x10) ? true : false;
			lcd_stat->sfx_target[5][0] = (value & 0x20) ? true : false;
			break;

		case BLDY:
			if(memory_map[address] == value) { return ; }

			memory_map[address] = value;
			if(value > 0xF) { value = 0x10; }
			lcd_stat->brightness_coef = (value & 0x1F) / 16.0;
			break;

		case REG_IF:
		case REG_IF+1:
			memory_map[address] &= ~value;
			break;

		case DMA0CNT_H:
		case DMA0CNT_H+1:
			dma[0].enable = true;
			dma[0].started = false;
			dma[0].delay = 2;

			memory_map[address] = value;
			break;

		case DMA1CNT_H:
			//Start DMA1 transfer if Bit 15 goes from 0 to 1
			if((value & 0x80) && ((memory_map[DMA1CNT_H] & 0x80) == 0))
			{
				dma[1].enable = true;
				dma[1].started = false;
				dma[1].delay = 2;
			}

			//Halt DMA1 transfer if Bit 15 goes from 1 to 0
			else if(((value & 0x80) == 0) && (memory_map[DMA1CNT_H] & 0x80)) { dma[1].enable = false; }

			memory_map[DMA1CNT_H] = value;
			break;

		case DMA2CNT_H:
			//Start DMA2 transfer if Bit 15 goes from 0 to 1
			if((value & 0x80) && ((memory_map[DMA2CNT_H] & 0x80) == 0))
			{
				dma[2].enable = true;
				dma[2].started = false;
				dma[2].delay = 2;
			}

			//Halt DMA2 transfer if Bit 15 goes from 1 to 0
			else if(((value & 0x80) == 0) && (memory_map[DMA2CNT_H] & 0x80)) { dma[2].enable = false; }

			memory_map[DMA2CNT_H] = value;
			break;

		case DMA3CNT_H:
		case DMA3CNT_H+1:
			dma[3].enable = true;
			dma[3].started = false;
			dma[3].delay = 2;

			memory_map[address] = value;
			break;

		case KEYINPUT: break;

		case TM0CNT_L:
		case TM0CNT_L+1:
			memory_map[address] = value;
			timer->at(0).reload_value = ((memory_map[TM0CNT_L+1] << 8) | memory_map[TM0CNT_L]);
			break;

		case TM1CNT_L:
		case TM1CNT_L+1:
			memory_map[address] = value;
			timer->at(1).reload_value = ((memory_map[TM1CNT_L+1] << 8) | memory_map[TM1CNT_L]);
			break;

		case TM2CNT_L:
		case TM2CNT_L+1:
			memory_map[address] = value;
			timer->at(2).reload_value = ((memory_map[TM2CNT_L+1] << 8) | memory_map[TM2CNT_L]);
			break;

		case TM3CNT_L:
		case TM3CNT_L+1:
			memory_map[address] = value;
			timer->at(3).reload_value = ((memory_map[TM3CNT_L+1] << 8) | memory_map[TM3CNT_L]);
			break;

		case TM0CNT_H:
		case TM0CNT_H+1:
			{
				bool prev_enable = (memory_map[TM0CNT_H] & 0x80) ?  true : false;
				memory_map[address] = value;

				timer->at(0).enable = (memory_map[TM0CNT_H] & 0x80) ?  true : false;
				timer->at(0).interrupt = (memory_map[TM0CNT_H] & 0x40) ? true : false;
				if((timer->at(0).enable) && (!prev_enable)) { timer->at(0).counter = timer->at(0).reload_value; }
			}

			switch(memory_map[TM0CNT_H] & 0x3)
			{
				case 0x0: timer->at(0).prescalar = 1; break;
				case 0x1: timer->at(0).prescalar = 64; break;
				case 0x2: timer->at(0).prescalar = 256; break;
				case 0x3: timer->at(0).prescalar = 1024; break;
			}

			break;

		case TM1CNT_H:
		case TM1CNT_H+1:
			{
				bool prev_enable = (memory_map[TM1CNT_H] & 0x80) ?  true : false;
				memory_map[address] = value;

				timer->at(1).enable = (memory_map[TM1CNT_H] & 0x80) ?  true : false;
				timer->at(1).interrupt = (memory_map[TM1CNT_H] & 0x40) ? true : false;
				if((timer->at(1).enable) && (!prev_enable)) { timer->at(1).counter = timer->at(1).reload_value; }
			}

			switch(memory_map[TM1CNT_H] & 0x3)
			{
				case 0x0: timer->at(1).prescalar = 1; break;
				case 0x1: timer->at(1).prescalar = 64; break;
				case 0x2: timer->at(1).prescalar = 256; break;
				case 0x3: timer->at(1).prescalar = 1024; break;
			}

			break;

		case TM2CNT_H:
		case TM2CNT_H+1:
			{
				bool prev_enable = (memory_map[TM2CNT_H] & 0x80) ?  true : false;
				memory_map[address] = value;

				timer->at(2).enable = (memory_map[TM2CNT_H] & 0x80) ?  true : false;
				timer->at(2).interrupt = (memory_map[TM2CNT_H] & 0x40) ? true : false;
				if((timer->at(2).enable) && (!prev_enable)) { timer->at(2).counter = timer->at(2).reload_value; }
			}

			switch(memory_map[TM2CNT_H] & 0x3)
			{
				case 0x0: timer->at(2).prescalar = 1; break;
				case 0x1: timer->at(2).prescalar = 64; break;
				case 0x2: timer->at(2).prescalar = 256; break;
				case 0x3: timer->at(2).prescalar = 1024; break;
			}

			break;

		case TM3CNT_H:
		case TM3CNT_H+1:
			{
				bool prev_enable = (memory_map[TM3CNT_H] & 0x80) ?  true : false;
				memory_map[address] = value;

				timer->at(3).enable = (memory_map[TM3CNT_H] & 0x80) ?  true : false;
				timer->at(3).interrupt = (memory_map[TM3CNT_H] & 0x40) ? true : false;
				if((timer->at(3).enable) && (!prev_enable)) { timer->at(3).counter = timer->at(3).reload_value; }
			}

			switch(memory_map[TM3CNT_H] & 0x3)
			{
				case 0x0: timer->at(3).prescalar = 1; break;
				case 0x1: timer->at(3).prescalar = 64; break;
				case 0x2: timer->at(3).prescalar = 256; break;
				case 0x3: timer->at(3).prescalar = 1024; break;
			}

			break;

		case WAITCNT:
		case WAITCNT+1:
			{
				memory_map[address] = value;
				u16 wait_control = read_u16(WAITCNT);

				//Determine first access cycles (Non-Sequential)
				switch((wait_control >> 2) & 0x3)
				{
					case 0x0: n_clock = 4; break;
					case 0x1: n_clock = 3; break;
					case 0x2: n_clock = 2; break;
					case 0x3: n_clock = 8; break;
				}

				//Determine second access cycles (Sequential)
				switch((wait_control >> 4) & 0x1)
				{
					case 0x0: s_clock = 2; break;
					case 0x1: s_clock = 1; break;
				}
			}

			break;

		case FLASH_RAM_CMD0:
			if((current_save_type == FLASH_64) || (current_save_type == FLASH_128))
			{
				//1st byte of the command
				if((flash_ram.current_command == 0) && (value == 0xAA)) { flash_ram.current_command++; }

				//3rd byte of the command, execute command
				else if(flash_ram.current_command == 2)
				{
					switch(value)
					{
						//FLASH erase chip
						case 0x10: 
							flash_erase_chip();
							flash_ram.current_command = 0;
							break;

						//FLASH erase command
						case 0x80:
							flash_ram.current_command = 0;
							break;			

						//FLASH ID start
						case 0x90:
							flash_ram.grab_ids = true;
							flash_ram.current_command = 0;
							break;

						//Write byte
						case 0xA0: 
							flash_ram.write_single_byte = true;
							flash_ram.current_command = 0;
							break;

						//Bank switch
						case 0xB0:
							flash_ram.switch_bank = true;
							flash_ram.current_command = 0;
							break;

						//FLASH ID end
						case 0xF0: 
							flash_ram.grab_ids = false;
							flash_ram.current_command = 0; 
							break;

						default: std::cout<<"MMU::Unknown FLASH RAM command 0x" << std::hex << (int)value << "\n"; break;
					}
				}
			}

			break;

		case FLASH_RAM_CMD1:
			if(((current_save_type == FLASH_64) || (current_save_type == FLASH_128)) && (value == 0x55))
			{
				if(flash_ram.current_command == 1) { flash_ram.current_command++; }
			}
		
			break;

		case FLASH_RAM_SEC0:
		case FLASH_RAM_SEC1:
		case FLASH_RAM_SEC2:
		case FLASH_RAM_SEC3:
		case FLASH_RAM_SEC4:
		case FLASH_RAM_SEC5:
		case FLASH_RAM_SEC6:
		case FLASH_RAM_SEC7:
		case FLASH_RAM_SEC8:
		case FLASH_RAM_SEC9:
		case FLASH_RAM_SECA:
		case FLASH_RAM_SECB:
		case FLASH_RAM_SECC:
		case FLASH_RAM_SECD:
		case FLASH_RAM_SECE:
		case FLASH_RAM_SECF:
			if(((current_save_type == FLASH_64) || (current_save_type == FLASH_128)) && (value == 0x30) && (flash_ram.current_command == 2))
			{
				flash_erase_sector(address);
				flash_ram.current_command = 0;
			}
			
			else if((current_save_type == FLASH_128) && (address == FLASH_RAM_SEC0) && (flash_ram.switch_bank) && (flash_ram.current_command == 0))
			{
				flash_ram.bank = value;
				flash_ram.switch_bank = false;
			}
			
			break;


		default:
			memory_map[address] = value;
	}

	//Mirror memory from 0x3007FXX to 0x3FFFFXX
	if((address >= 0x3007F00) && (address <= 0x3007FFF)) 
	{
		u32 mirror_addr = 0x03FFFF00 + (address & 0xFF);
		memory_map[mirror_addr] = value;
	}

	//Trigger BG scaling+rotation parameter update in LCD
	else if((address >= 0x4000020) && (address <= 0x400003F))
	{
		lcd_stat->bg_params_update = true;
	}

	//Trigger BG palette update in LCD
	else if((address >= 0x5000000) && (address <= 0x50001FF))
	{
		lcd_stat->bg_pal_update = true;
		lcd_stat->bg_pal_update_list[(address & 0x1FF) >> 1] = true;
	}

	//Trigger OBJ palette update in LCD
	else if((address >= 0x5000200) && (address <= 0x50003FF))
	{
		lcd_stat->obj_pal_update = true;
		lcd_stat->obj_pal_update_list[(address & 0x1FF) >> 1] = true;
	}

	//Trigger OAM update in LCD
	else if((address >= 0x7000000) && (address <= 0x70003FF))
	{
		lcd_stat->oam_update = true;
		lcd_stat->oam_update_list[(address & 0x3FF) >> 3] = true;
	}

	//Write to FLASH RAM
	else if(((current_save_type == FLASH_64) || (current_save_type == FLASH_128)) && (flash_ram.next_write) && (address >= 0xE000000) && (address <= 0xE00FFFF))
	{
			flash_ram.data[flash_ram.bank][(address & 0xFFFF)] = value;
			flash_ram.next_write = false;
	}

	if(flash_ram.write_single_byte) 
	{ 
		flash_ram.write_single_byte = false;
		flash_ram.next_write = true;
	}
}

/****** Write 2 bytes into memory ******/
void MMU::write_u16(u32 address, u16 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
}

/****** Write 4 bytes into memory ******/
void MMU::write_u32(u32 address, u32 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
	write_u8((address+2), ((value >> 16) & 0xFF));
	write_u8((address+3), ((value >> 24) & 0xFF));
}

/****** Writes 2 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void MMU::write_u16_fast(u32 address, u16 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
}

/****** Writes 4 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void MMU::write_u32_fast(u32 address, u32 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
	memory_map[address+2] = ((value >> 16) & 0xFF);
	memory_map[address+3] = ((value >> 24) & 0xFF);
}	

/****** Read binary file to memory ******/
bool MMU::read_file(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	u8* ex_mem = &memory_map[0x8000000];

	//Read data from the ROM file
	file.read((char*)ex_mem, file_size);

	file.close();
	std::cout<<"MMU::" << filename << " loaded successfully. \n";

	std::string backup_file = filename + ".sav";

	//Try to auto-detect save-type, if any
	for(u32 x = 0x8000000; x < (0x8000000 + file_size); x+=4)
	{
		switch(memory_map[x])
		{
			//EEPROM
			case 0x45:
				if((memory_map[x+1] == 0x45) && (memory_map[x+2] == 0x50) && (memory_map[x+3] == 0x52) && (memory_map[x+4] == 0x4F) && (memory_map[x+5] == 0x4D))
				{
					std::cout<<"MMU::EEPROM save type detected\n";
					current_save_type = EEPROM;
					load_backup(backup_file);
					return true;
				}
				
				break;

			//FLASH RAM
			case 0x46:
				//64KB "FLASH_Vnnn"
				if((memory_map[x+1] == 0x4C) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x53) && (memory_map[x+4] == 0x48) && (memory_map[x+5] == 0x5F))
				{
					std::cout<<"MMU::FLASH RAM (64KB) save type detected\n";
					current_save_type = FLASH_64;
					load_backup(backup_file);
					return true;
				}

				//64KB "FLASH512_Vnnn"
				else if((memory_map[x+1] == 0x4C) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x53) && (memory_map[x+4] == 0x48) && (memory_map[x+5] == 0x35)
				&& (memory_map[x+6] == 0x31) && (memory_map[x+7] == 0x32)) 
				{
					std::cout<<"MMU::FLASH RAM (64KB) save type detected\n";
					current_save_type = FLASH_64;
					load_backup(backup_file);
					return true;
				}

				//128KB "FLASH1M_V"
				else if((memory_map[x+1] == 0x4C) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x53) && (memory_map[x+4] == 0x48) && (memory_map[x+5] == 0x31)
				&& (memory_map[x+6] == 0x4D))
				{
					std::cout<<"MMU::FLASH RAM (128KB) save type detected\n";
					current_save_type = FLASH_128;
					load_backup(backup_file);
					return true;
				}

				break;

			//SRAM
			case 0x53:
				if((memory_map[x+1] == 0x52) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x4D))
				{
					std::cout<<"MMU::SRAM save type detected\n";
					current_save_type = SRAM;
					load_backup(backup_file);
					return true;
				}

				break;
		}
	}
		
	return true;
}

/****** Read BIOS file into memory ******/
bool MMU::read_bios(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::BIOS file " << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	u8* ex_mem = &memory_map[0];

	//Read data from the ROM file
	file.read((char*)ex_mem, file_size);

	file.close();
	std::cout<<"MMU::BIOS file " << filename << " loaded successfully. \n";

	return true;
}

/****** Load backup save data ******/
bool MMU::load_backup(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	std::vector<u8> save_data;

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " save data could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);
	save_data.resize(file_size);

	//Load SRAM
	if(current_save_type == SRAM)
	{
		if(file_size > 0x8000) { std::cout<<"MMU::Warning - Irregular backup save size\n"; }

		//Read data from file
		file.read(reinterpret_cast<char*> (&save_data[0]), file_size);

		//Write that data into 0xE000000 to 0xE007FFF
		for(u32 x = 0; x <= 0x7FFF; x++)
		{
			memory_map[0xE000000 + x] = save_data[x];
			memory_map[0xE008000 + x] = 0xFF;
		}
	}

	//Load EEPROM
	else if(current_save_type == EEPROM)
	{
		if((file_size != 0x200) && (file_size != 0x2000)) { file_size = 0x200; std::cout<<"MMU::Warning - Irregular backup save size\n"; }

		//Read data from file
		file.read(reinterpret_cast<char*> (&save_data[0]), file_size);

		//Clear eeprom data and resize
		eeprom.size = file_size;
		eeprom.data.clear();
		eeprom.data.resize(file_size, 0);

		//Write that data into EEPROM
		for(u32 x = 0; x < file_size; x++)
		{
			eeprom.data[x] = save_data[x];
		}

		eeprom.size_lock = true;
	}

	//Load 64KB FLASH RAM
	if(current_save_type == FLASH_64)
	{
		//Read data from file
		file.read(reinterpret_cast<char*> (&save_data[0]), file_size);

		//Write that data into 0xE000000 to 0xE00FFFF of FLASH RAM
		for(u32 x = 0; x < 0x10000; x++)
		{
			flash_ram.data[0][x] = save_data[x];
		}
	}

	//Load 128KB FLASH RAM
	if(current_save_type == FLASH_128)
	{
		//Read data from file
		file.read(reinterpret_cast<char*> (&save_data[0]), file_size);

		//Write that data into 0xE000000 to 0xE00FFFF of FLASH RAM
		for(u32 x = 0; x < 0x10000; x++)
		{
			flash_ram.data[0][x] = save_data[x];
		}

		for(u32 x = 0x10000; x < 0x20000; x++)
		{
			flash_ram.data[1][x - 0x10000] = save_data[x];
		}
	}

	file.close();

	std::cout<<"MMU::Loaded save data file " << filename <<  "\n";

	return true;
}

/****** Save backup save data ******/
bool MMU::save_backup(std::string filename)
{
	//Save SRAM
	if(current_save_type == SRAM)
	{
		std::ofstream file(filename.c_str(), std::ios::binary);
		std::vector<u8> save_data;

		if(!file.is_open()) 
		{
			std::cout<<"MMU::" << filename << " save data could not be written. Check file path or permissions. \n";
			return false;
		}


		//Grab data from 0xE000000 to 0xE007FFF
		for(u32 x = 0; x <= 0x7FFF; x++)
		{
			save_data.push_back(memory_map[0xE000000 + x]);
		}

		//Write the data to a file
		file.write(reinterpret_cast<char*> (&save_data[0]), 0x7FFF);
		file.close();

		std::cout<<"MMU::Wrote save data file " << filename <<  "\n";
	}

	//Save EEPROM
	else if(current_save_type == EEPROM)
	{
		std::ofstream file(filename.c_str(), std::ios::binary);
		std::vector<u8> save_data;

		if(!file.is_open()) 
		{
			std::cout<<"MMU::" << filename << " save data could not be written. Check file path or permissions. \n";
			return false;
		}

		//Grab data from EEPROM
		for(u32 x = 0; x < eeprom.size; x++)
		{
			save_data.push_back(eeprom.data[x]);
		}

		//Write the data to a file
		file.write(reinterpret_cast<char*> (&save_data[0]), eeprom.size);
		file.close();

		std::cout<<"MMU::Wrote save data file " << filename <<  "\n";
	}

	//Save 64KB FLASH RAM
	else if(current_save_type == FLASH_64)
	{
		std::ofstream file(filename.c_str(), std::ios::binary);
		std::vector<u8> save_data;

		if(!file.is_open()) 
		{
			std::cout<<"MMU::" << filename << " save data could not be written. Check file path or permissions. \n";
			return false;
		}


		//Grab data from 0xE000000 to 0xE00FFFF from FLASH RAM
		for(u32 x = 0; x < 0x10000; x++)
		{
			save_data.push_back(flash_ram.data[0][x]);
		}

		//Write the data to a file
		file.write(reinterpret_cast<char*> (&save_data[0]), 0x10000);
		file.close();

		std::cout<<"MMU::Wrote save data file " << filename <<  "\n";
	}

	//Save 128KB FLASH RAM
	else if(current_save_type == FLASH_128)
	{
		std::ofstream file(filename.c_str(), std::ios::binary);
		std::vector<u8> save_data;

		if(!file.is_open()) 
		{
			std::cout<<"MMU::" << filename << " save data could not be written. Check file path or permissions. \n";
			return false;
		}


		//Grab data from 0xE000000 to 0xE00FFFF from FLASH RAM
		for(u32 x = 0; x < 0x10000; x++)
		{
			save_data.push_back(flash_ram.data[0][x]);
		}

		for(u32 x = 0; x < 0x10000; x++)
		{
			save_data.push_back(flash_ram.data[1][x]);
		}

		//Write the data to a file
		file.write(reinterpret_cast<char*> (&save_data[0]), 0x20000);
		file.close();

		std::cout<<"MMU::Wrote save data file " << filename <<  "\n";
	}

	return true;
}

/****** Start the DMA channels during blanking periods ******/
void MMU::start_blank_dma()
{
	//Repeat bits automatically enable DMAs
	if(dma[0].control & 0x200) { dma[0].enable = true; }
	if(dma[3].control & 0x200) { dma[3].enable = true; }

	//DMA0
	if(dma[0].enable)
	{
		u8 dma_type = ((dma[0].control >> 12) & 0x3);
		
		if(dma_type == 2) { dma[0].started = true; }
	}

	//DMA3
	if(dma[3].enable)
	{
		u8 dma_type = ((dma[3].control >> 12) & 0x3);
		
		if(dma_type == 2) { dma[3].started = true; }
	}
}

/****** Set EEPROM read-write address ******/
void MMU::eeprom_set_addr()
{
	//Clear EEPROM address
	eeprom.address = 0;

	//Skip 2 bits in the bitstream
	eeprom.dma_ptr += 4;

	//Read 6 or 14 bits from the bitstream, MSB 1st
	u16 bits = 0x20;
	u8 bit_length = 6;
	
	if(eeprom.size == 0x2000) { bits = 0x2000; bit_length = 14; }

	for(int x = 0; x < bit_length; x++)
	{
		u16 bitstream = read_u16(eeprom.dma_ptr);
		
		//If bit 0 of the halfword in the bitstream is set, flip the bit for the EEPROM address as well
		if(bitstream & 0x1) { eeprom.address |= bits; }

		eeprom.dma_ptr += 2;
		bits >>= 1;
	}
}

/****** Read EEPROM data ******/
void MMU::eeprom_read_data()
{
	//First 4 bits of the stream are ignored, send 0
	for(int x = 0; x < 4; x++) 
	{ 
		write_u16(eeprom.dma_ptr, 0x0);
		eeprom.dma_ptr += 2;
	}
	
	u16 temp_addr = (eeprom.address * 8);
	u8 bits = 0x80;

	//Get 64 bits from EEPROM, MSB 1st, write them to address pointed by the DMA (as halfwords)
	for(int x = 0; x < 64; x++)
	{
		u8 bitstream = (eeprom.data[temp_addr] & bits) ? 1 : 0;
		bits >>= 1;

		//Write stream to address provided by DMA
		write_u16(eeprom.dma_ptr, bitstream);
		eeprom.dma_ptr += 2;

		//On the 8th bit, move to next 8 bits in EEPROM, reload stuff
		if(bits == 0) 
		{
			temp_addr++;
			bits = 0x80;
		}
	}
}

/****** Write EEPROM data ******/
void MMU::eeprom_write_data()
{
	//Clear EEPROM address
	eeprom.address = 0;

	//Skip 2 bits in the bitstream
	eeprom.dma_ptr += 4;

	//Read 6 or 14 bits from the bitstream, MSB 1st
	u16 bits = 0x20;
	u8 bit_length = 6;
	
	if(eeprom.size == 0x2000) { bits = 0x2000; bit_length = 14; }

	for(int x = 0; x < bit_length; x++)
	{
		u16 bitstream = read_u16(eeprom.dma_ptr);
		
		//If bit 0 of the halfword in the bitstream is set, flip the bit for the EEPROM address as well
		if(bitstream & 0x1) { eeprom.address |= bits; }

		eeprom.dma_ptr += 2;
		bits >>= 1;
	}

	u8 temp_byte = 0;
	u16 temp_addr = (eeprom.address * 8);
	bits = 0x80;

	//Read 64 bits from the bitstream to store at EEPROM address, MSB 1st
	for(int x = 0; x < 64; x++)
	{
		u16 bitstream = read_u16(eeprom.dma_ptr);

		if(bitstream & 0x1) { temp_byte |= bits; }

		eeprom.dma_ptr += 2;
		bits >>= 1;

		//On the 8th bit, send the data to EEPROM, reload stuff
		if(bits == 0) 
		{
			eeprom.data[temp_addr] = temp_byte;
			temp_byte = 0;
			temp_addr++;
			bits = 0x80;
		}
	}

	memory_map[0xD000000] = 0x1;
}

/****** Erase entire FLASH RAM ******/
void MMU::flash_erase_chip()
{
	for(int x = 0; x < 0x10000; x++) 
	{ 
		flash_ram.data[0][x] = 0xFF;
		flash_ram.data[1][x] = 0xFF; 
	}
}

/****** Erase 4KB sector of FLASH RAM ******/
void MMU::flash_erase_sector(u32 sector)
{
	for(u32 x = sector; x < (sector + 0x1000); x++) 
	{ 
		flash_ram.data[flash_ram.bank][(x & 0xFFFF)] = 0xFF; 
	}
}	

/****** Points the MMU to an lcd_data structure (FROM THE LCD ITSELF) ******/
void MMU::set_lcd_data(lcd_data* ex_lcd_stat) { lcd_stat = ex_lcd_stat; }
