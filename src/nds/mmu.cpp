// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : September 14, 2015
// Description : NDS memory manager unit
//
// Handles reading and writing bytes to memory locations

#include "mmu.h"

/****** MMU Constructor ******/
NTR_MMU::NTR_MMU() 
{
	reset();
}

/****** MMU Deconstructor ******/
NTR_MMU::~NTR_MMU() 
{ 
	memory_map.clear();
	cart_data.clear();
	nds7_bios.clear();
	nds9_bios.clear();
	std::cout<<"MMU::Shutdown\n"; 
}

/****** MMU Reset ******/
void NTR_MMU::reset()
{
	memory_map.clear();
	memory_map.resize(0x10000000, 0);

	cart_data.clear();

	firmware.clear();
	firmware.resize(0x40000, 0);

	//Default access mode starts off with NDS9
	access_mode = 1;

	nds7_bios.clear();
	nds7_bios.resize(0x4000, 0);
	nds7_bios_vector = 0x0;
	nds7_irq_handler = 0x380FFFC;
	nds7_ie = 0x0;
	nds7_if = 0x0;
	nds7_old_ie = 0x0;
	nds7_ime = 0;
	power_cnt2 = 0;

	nds9_bios.clear();
	nds9_bios.resize(0xC00, 0);
	nds9_bios_vector = 0xFFFF0000;
	nds9_irq_handler = 0x0;
	nds9_ie = 0x0;
	nds9_if = 0x0;
	nds9_old_ie = 0x0;
	nds9_ime = 0;
	power_cnt1 = 0;

	//HLE MMIO stuff
	memory_map[NDS_DISPCNT_A] = 0x80;
	write_u16_fast(NDS_KEYINPUT, 0x3FF);

	//HLE firmware stuff
	setup_default_firmware();

	firmware_state = 0;
	firmware_index = 0;

	//Default memory access timings (4, 2)
	n_clock = 4;
	s_clock = 2;

	//Setup NDS_DMA info
	for(int x = 0; x < 8; x++)
	{
		dma[x].enable = false;
		dma[x].started = false;
		dma[x].start_address = 0;
		dma[x].original_start_address = 0;
		dma[x].destination_address = 0;
		dma[x].current_dma_position = 0;
		dma[x].word_count = 0;
		dma[x].word_type = 0;
		dma[x].control = 0;
		dma[x].dest_addr_ctrl = 0;
		dma[x].src_addr_ctrl = 0;
		dma[x].delay = 0;
	}

	//Clear IPC FIFO if necessary
	while(!nds7_ipc.fifo.empty()) { nds7_ipc.fifo.pop(); }
	while(!nds9_ipc.fifo.empty()) { nds9_ipc.fifo.pop(); }

	nds7_ipc.sync = 0;
	nds7_ipc.cnt = 0x101;
	nds7_ipc.fifo_latest = 0;
	nds7_ipc.fifo_incoming = 0;

	nds9_ipc.sync = 0;
	nds9_ipc.cnt = 0x101;
	nds9_ipc.fifo_latest = 0;
	nds9_ipc.fifo_incoming = 0;

	nds7_spi.cnt = 0;
	nds7_spi.data = 0;
	nds7_spi.baud_rate = 64;
	nds7_spi.transfer_clock = 0;
	nds7_spi.active_transfer = false;

	touchscreen.adc_x1 = read_u16(0x27FFCD8) & 0x1FFF;
	touchscreen.adc_y1 = read_u16(0x27FFCDA) & 0x1FFF;
	touchscreen.scr_x1 = read_u8(0x27FFCDC);
	touchscreen.scr_x2 = read_u8(0x27FFCDD);

	touchscreen.adc_x2 = read_u16(0x27FFCDE) & 0x1FFF;
	touchscreen.adc_y2 = read_u16(0x27FFCE0) & 0x1FFF;
	touchscreen.scr_x2 = read_u8(0x27FFCE2);
	touchscreen.scr_y2 = read_u8(0x27FFCE3);

	touchscreen_state = 0;

	g_pad = NULL;

	std::cout<<"MMU::Initialized\n";
}

/****** Read byte from memory ******/
u8 NTR_MMU::read_u8(u32 address)
{
	//Mirror memory address if applicable
	switch(address >> 24)
	{
		case 0x2:
			address &= 0x23FFFFF;
			break;
	}

	switch(address)
	{
		case NDS_KEYINPUT:
			return (g_pad->key_input & 0xFF);
			break;

		case NDS_KEYINPUT+1:
			return (g_pad->key_input >> 8);
			break;

		case NDS_EXTKEYIN:
			return (g_pad->ext_key_input & 0xFF);
			break;

		case NDS_EXTKEYIN+1:
			return (g_pad->ext_key_input >> 8);
			break;
	}

	//Read from NDS9 BIOS
	if((address >= nds9_bios_vector) && (address <= (nds9_bios_vector + 0xC00)) && (access_mode)) { return nds9_bios[address - nds9_bios_vector]; }

	//Read from NDS7 BIOS
	if((address >= nds7_bios_vector) && (address <= (nds7_bios_vector + 0x4000)) && (!access_mode)) { return nds7_bios[address - nds7_bios_vector]; }

	//Check for unused memory first
	else if(address >= 0x10000000) { return 0; std::cout<<"Out of bounds read : 0x" << std::hex << address << "\n"; return 0; }

	//Check for reading DISPSTAT
	else if((address & ~0x1) == NDS_DISPSTAT)
	{
		u8 addr_shift = (address & 0x1) << 3;

		//Return NDS9 DISPSTAT
		if(access_mode) { return ((lcd_stat->display_stat_a >> addr_shift) & 0xFF); }
		
		//Return NDS7 DISPSTAT
		else { return ((lcd_stat->display_stat_b >> addr_shift) & 0xFF); }
	}

	//Check for reading IME
	else if((address & ~0x3) == NDS_IME)
	{
		u8 addr_shift = (address & 0x3) << 3;

		//Return NDS9 IME
		if(access_mode) { return ((nds9_ime >> addr_shift) & 0xFF); }
		
		//Return NDS7 IME
		else { return ((nds7_ime >> addr_shift) & 0xFF);  }
	}

	//Check for reading IE
	else if((address & ~0x3) == NDS_IE)
	{
		u8 addr_shift = (address & 0x3) << 3;

		//Return NDS9 IE
		if(access_mode) { return ((nds9_ie >> addr_shift) & 0xFF); }
		
		//Return NDS7 IE
		else { return ((nds7_ie >> addr_shift) & 0xFF);  }
	}

	//Check for reading IF
	else if((address & ~0x3) == NDS_IF)
	{
		u8 addr_shift = (address & 0x3) << 3;

		//Return NDS9 IF
		if(access_mode) { return ((nds9_if >> addr_shift) & 0xFF); }
		
		//Return NDS7 IF
		else { return ((nds7_if >> addr_shift) & 0xFF); }
	}

	//Check for IPCSYNC
	else if((address & ~0x1) == NDS_IPCSYNC)
	{
		u8 addr_shift = (address & 0x1) << 3;

		//Return NDS9 IPCSYNC
		if(access_mode) { return ((nds9_ipc.sync >> addr_shift) & 0xFF); }
		
		//Return NDS7 IPCSYNC
		else { return ((nds7_ipc.sync >> addr_shift) & 0xFF); }
	}

	//Check for IPCFIFOCNT
	else if((address & ~0x1) == NDS_IPCFIFOCNT)
	{
		u8 addr_shift = (address & 0x1) << 3;

		//Return NDS9 IPCFIFOCNT
		if(access_mode) { return ((nds9_ipc.cnt >> addr_shift) & 0xFF); }
		
		//Return NDS7 IPCFIFOCNT
		else { return ((nds7_ipc.cnt >> addr_shift) & 0xFF); }
	}

	//Check for reading IPCFIFORECV
	else if((address & ~0x3) == NDS_IPCFIFORECV)
	{
		u8 addr_shift = (address & 0x3) << 3;

		//NDS9 - Read from NDS7 IPC FIFOSND
		if(access_mode)
		{
			//Return last FIFO entry if empty, or zero if no data was ever put there
			if(nds7_ipc.fifo.empty())
			{
				//Set Bit 14 of NDS9 IPCFIFOCNT to indicate error
				nds9_ipc.cnt |= 0x4000;

				return ((nds7_ipc.fifo_latest >> addr_shift) & 0xFF);
			}
		
			//Otherwise, read FIFO normally
			else
			{
				//Unset Bit 14 of NDS9 IPCFIFOCNT to indicate no errors
				nds9_ipc.cnt &= ~0x4000;

				u8 fifo_entry = ((nds7_ipc.fifo.front() >> addr_shift) & 0xFF);
		
				//If Bit 15 of IPCFIFOCNT is 0, read back oldest entry but do not pop it
				//If Bit 15 is set, get rid of the oldest entry now
				if((nds9_ipc.cnt & 0x8000) && (address == NDS_IPCFIFORECV))
				{
					nds7_ipc.fifo.pop();

					//Unset NDS7 SNDFIFO FULL Status
					nds7_ipc.cnt &= ~0x2;

					//Unset NDS9 RECVFIFO FULL Status
					nds9_ipc.cnt &= ~0x200;

					if(nds7_ipc.fifo.empty())
					{
						//Set SNDFIFO EMPTY Status on NDS7
						nds7_ipc.cnt |= 0x1;

						//Set RECVFIFO EMPTY Status on NDS9
						nds9_ipc.cnt |= 0x100;

						//Raise Send FIFO EMPTY IRQ if necessary
						if(nds7_ipc.cnt & 0x4) { nds7_if |= 0x20000; }
					}
				}

				return fifo_entry;
			}
		}

		//NDS7 - Read from NDS9 IPC FIFOSND
		else
		{
			//Return last FIFO entry if empty, or zero if no data was ever put there
			if(nds9_ipc.fifo.empty())
			{
				//Set Bit 14 of NDS7 IPCFIFOCNT to indicate error
				nds7_ipc.cnt |= 0x4000;

				return ((nds9_ipc.fifo_latest >> addr_shift) & 0xFF);
			}
		
			//Otherwise, read FIFO normally
			else
			{
				//Unset Bit 14 of NDS7 IPCFIFOCNT to indicate no errors
				nds7_ipc.cnt &= ~0x4000;

				u8 fifo_entry = ((nds9_ipc.fifo.front() >> addr_shift) & 0xFF);
		
				//If Bit 15 of IPCFIFOCNT is 0, read back oldest entry but do not pop it
				//If Bit 15 is set, get rid of the oldest entry now
				if((nds7_ipc.cnt & 0x8000) && (address == NDS_IPCFIFORECV))
				{
					nds9_ipc.fifo.pop();

					//Unset NDS9 SNDFIFO FULL Status
					nds9_ipc.cnt &= ~0x2;

					//Unset NDS7 RECVFIFO FULL Status
					nds7_ipc.cnt &= ~0x200;

					if(nds9_ipc.fifo.empty())
					{
						//Set SNDFIFO EMPTY Status on NDS9
						nds9_ipc.cnt |= 0x1;

						//Set RECVFIFO EMPTY Status on NDS7
						nds7_ipc.cnt |= 0x100;

						//Raise Send FIFO EMPTY IRQ if necessary
						if(nds9_ipc.cnt & 0x4) { nds9_if |= 0x20000; }
					}
				}

				return fifo_entry;
			}
		}
	}

	//Check for SPICNT
	else if((address & ~0x1) == NDS_SPICNT)
	{
		//Only NDS7 can access this register, return 0 for NDS9
		//TODO - This really probably return the same as other unused IO
		if(access_mode) { return 0; }

		//Return SPICNT
		u8 addr_shift = (address & 0x1) << 3;
		return ((nds7_spi.cnt >> addr_shift) & 0xFF);
	}

	//Check for SPIDATA
	else if((address & ~0x1) == NDS_SPIDATA)
	{
		//Only NDS7 can access this register, return 0 for NDS9
		//TODO - This really probably return the same as other unused IO
		if(access_mode) { return 0; }

		//NDS7 should only access SPI bus if it is enabled
		//TODO - This really probably return the same as other unused IO
		if((nds7_spi.cnt & 0x8000) == 0) { return 0; }

		//Return SPIDATA
		u8 addr_shift = (address & 0x1) << 3;
		return ((nds7_spi.data >> addr_shift) & 0xFF);
	}

	//Check for POWERCNT
	else if((address & ~0x1) == NDS_POWERCNT)
	{
		u8 addr_shift = (address & 0x1) << 3;

		if(access_mode) { return ((power_cnt1 >> addr_shift) & 0xFF); }
		else { return ((power_cnt2 >> addr_shift) & 0xFF); }
	}

	return memory_map[address];
}

/****** Read 2 bytes from memory ******/
u16 NTR_MMU::read_u16(u32 address)
{
	return ((read_u8(address+1) << 8) | read_u8(address)); 
}

/****** Read 4 bytes from memory ******/
u32 NTR_MMU::read_u32(u32 address)
{
	//Misaligned word read
	if(address & 0x3)
	{
		//Align by word, ROR value by offset
		u8 offset = (address & 0x3);
		address &= ~0x3;

		switch(offset)
		{
			//ROR 8
			case 0x1:
				return ((read_u8(address) << 24) | (read_u8(address+3) << 16) | (read_u8(address+2) << 8) | read_u8(address+1));

			//ROR 16
			case 0x2:
				return ((read_u8(address+1) << 24) | (read_u8(address) << 16) | (read_u8(address+3) << 8) | read_u8(address+2));

			//ROR 24
			case 0x3:
				return ((read_u8(address+2) << 24) | (read_u8(address+1) << 16) | (read_u8(address) << 8) | read_u8(address+3));
		}
	}

	return ((read_u8(address+3) << 24) | (read_u8(address+2) << 16) | (read_u8(address+1) << 8) | read_u8(address));
}

/****** Reads 2 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u16 NTR_MMU::read_u16_fast(u32 address) const
{
	return ((memory_map[address+1] << 8) | memory_map[address]);
}

/****** Reads 4 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u32 NTR_MMU::read_u32_fast(u32 address) const
{
	return ((memory_map[address+3] << 24) | (memory_map[address+2] << 16) | (memory_map[address+1] << 8) | memory_map[address]);
}

/****** Write byte into memory ******/
void NTR_MMU::write_u8(u32 address, u8 value)
{
	//Mirror memory address if applicable
	switch(address >> 24)
	{
		case 0x2:
			address &= 0x23FFFFF;
			break;
	}

	//Check for unused memory first
	if(address >= 0x10000000) { return; std::cout<<"Out of bounds write : 0x" << std::hex << address << "\n"; return; }

	switch(address)
	{
		//Display Control A
		case NDS_DISPCNT_A:
		case NDS_DISPCNT_A+1:
		case NDS_DISPCNT_A+2:
		case NDS_DISPCNT_A+3:
			memory_map[address] = value;
			lcd_stat->display_control_a = ((memory_map[NDS_DISPCNT_A+3] << 24) | (memory_map[NDS_DISPCNT_A+2] << 16) | (memory_map[NDS_DISPCNT_A+1] << 8) | memory_map[NDS_DISPCNT_A]);
			lcd_stat->bg_mode_a = (lcd_stat->display_control_a & 0x7);
			lcd_stat->display_mode_a = (lcd_stat->display_control_a >> 16) & 0x3;

			//Enable or disable BGs
			lcd_stat->bg_enable_a[0] = (lcd_stat->display_control_a & 0x100) ? true : false;
			lcd_stat->bg_enable_a[1] = (lcd_stat->display_control_a & 0x200) ? true : false;
			lcd_stat->bg_enable_a[2] = (lcd_stat->display_control_a & 0x400) ? true : false;
			lcd_stat->bg_enable_a[3] = (lcd_stat->display_control_a & 0x800) ? true : false;

			//Update all BG controls
			lcd_stat->update_bg_control_a = true;

			break;

		//Display Control B
		case NDS_DISPCNT_B:
		case NDS_DISPCNT_B+1:
		case NDS_DISPCNT_B+2:
		case NDS_DISPCNT_B+3:
			memory_map[address] = value;
			lcd_stat->display_control_b = ((memory_map[NDS_DISPCNT_B+3] << 24) | (memory_map[NDS_DISPCNT_B+2] << 16) | (memory_map[NDS_DISPCNT_B+1] << 8) | memory_map[NDS_DISPCNT_B]);
			lcd_stat->bg_mode_b = (lcd_stat->display_control_b & 0x7);
			lcd_stat->display_mode_b = (lcd_stat->display_control_b >> 16) & 0x1;

			//Enable or disable BGs
			lcd_stat->bg_enable_b[0] = (lcd_stat->display_control_b & 0x100) ? true : false;
			lcd_stat->bg_enable_b[1] = (lcd_stat->display_control_b & 0x200) ? true : false;
			lcd_stat->bg_enable_b[2] = (lcd_stat->display_control_b & 0x400) ? true : false;
			lcd_stat->bg_enable_b[3] = (lcd_stat->display_control_b & 0x800) ? true : false;

			//Update all BG controls
			lcd_stat->update_bg_control_b = true;

			break;

		//Display Status
		case NDS_DISPSTAT:
			if(access_mode)
			{	
				lcd_stat->display_stat_a &= ~0xF8;
				lcd_stat->display_stat_a |= (value & ~0x7);

				lcd_stat->vblank_irq_enable_a = (value & 0x8) ? true : false;
				lcd_stat->hblank_irq_enable_a = (value & 0x10) ? true : false;
				lcd_stat->vcount_irq_enable_a = (value & 0x20) ? true : false;

				//MSB of LYC is Bit 7 of DISPSTAT
				lcd_stat->lyc_a = (lcd_stat->display_stat_a >> 7) & 0x1FF;
			}

			else
			{	
				lcd_stat->display_stat_b &= ~0xF8;
				lcd_stat->display_stat_b |= (value & ~0x7);

				lcd_stat->vblank_irq_enable_b = (value & 0x8) ? true : false;
				lcd_stat->hblank_irq_enable_b = (value & 0x10) ? true : false;
				lcd_stat->vcount_irq_enable_b = (value & 0x20) ? true : false;

				//MSB of LYC is Bit 7 of DISPSTAT
				lcd_stat->lyc_b = (lcd_stat->display_stat_b >> 7) & 0x1FF;
			}
 
			break;

		//Display Status - Lower 8 bits of LYC
		case NDS_DISPSTAT+1:
			if(access_mode)
			{
				lcd_stat->display_stat_a &= 0xFF;
				lcd_stat->display_stat_a |= (value << 8);

				lcd_stat->lyc_a = (lcd_stat->display_stat_a >> 7) & 0x1FF;
			}

			else
			{
				lcd_stat->display_stat_b &= 0xFF;
				lcd_stat->display_stat_b |= (value << 8);

				lcd_stat->lyc_b = (lcd_stat->display_stat_b >> 7) & 0x1FF;
			}

			break;

		//Vertical-Line Count (Writable, unlike the GBA)
		case NDS_VCOUNT:
			memory_map[address] = value;
			lcd_stat->current_scanline = (memory_map[NDS_VCOUNT+1] << 8) | memory_map[NDS_VCOUNT];
			lcd_stat->lcd_clock = (2130 * lcd_stat->current_scanline);
			lcd_stat->lcd_mode = (lcd_stat->lcd_clock < 4089600) ? 0 : 2;
			break;

		case NDS_VCOUNT+1:
			memory_map[address] = value & 0x1;
			lcd_stat->current_scanline = (memory_map[NDS_VCOUNT+1] << 8) | memory_map[NDS_VCOUNT];
			lcd_stat->lcd_clock = (2130 * lcd_stat->current_scanline);
			lcd_stat->lcd_mode = (lcd_stat->lcd_clock < 4089600) ? 0 : 2;
			break;

		//BG0 Control A
		case NDS_BG0CNT_A:
		case NDS_BG0CNT_A+1:
			{
				memory_map[address] = value;
				lcd_stat->bg_control_a[0] = (memory_map[NDS_BG0CNT_A+1] << 8) | memory_map[NDS_BG0CNT_A];

				//Determine BG Priority
				lcd_stat->bg_priority_a[0] = lcd_stat->bg_control_a[0] & 0x3;
			
				//Calculate tile data and tile map addresses
				u32 char_base = ((lcd_stat->display_control_a >> 24) & 0x7);
				u32 screen_base = ((lcd_stat->display_control_a >> 27) & 0x7);

				u32 char_block = ((lcd_stat->bg_control_a[0] >> 2) & 0xF);
				u32 screen_block = ((lcd_stat->bg_control_a[0] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_a[0] = (char_base * 0x10000) + (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_a[0] = (screen_base * 0x10000) + (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_a[0] = (lcd_stat->bg_control_a[0] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_a[0] = (lcd_stat->bg_control_a[0] >> 14) & 0x3;
			}

			break;

		//BG0 Control B
		case NDS_BG0CNT_B:
		case NDS_BG0CNT_B+1:
			{
				memory_map[address] = value;
				lcd_stat->bg_control_b[0] = (memory_map[NDS_BG0CNT_B+1] << 8) | memory_map[NDS_BG0CNT_B];

				//Determine BG Priority
				lcd_stat->bg_priority_b[0] = lcd_stat->bg_control_b[0] & 0x3;

				u32 char_block = ((lcd_stat->bg_control_b[0] >> 2) & 0xF);
				u32 screen_block = ((lcd_stat->bg_control_b[0] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_b[0] = (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_b[0] = (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_b[0] = (lcd_stat->bg_control_b[0] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_b[0] = (lcd_stat->bg_control_b[0] >> 14) & 0x3;
			}

			break;

		//BG1 Control A
		case NDS_BG1CNT_A:
		case NDS_BG1CNT_A+1:
			{
				memory_map[address] = value;
				lcd_stat->bg_control_a[1] = (memory_map[NDS_BG1CNT_A+1] << 8) | memory_map[NDS_BG1CNT_A];

				//Determine BG Priority
				lcd_stat->bg_priority_a[1] = lcd_stat->bg_control_a[1] & 0x3;
			
				//Calculate tile data and tile map addresses
				u32 char_base = ((lcd_stat->display_control_a >> 24) & 0x7);
				u32 screen_base = ((lcd_stat->display_control_a >> 27) & 0x7);

				u32 char_block = ((lcd_stat->bg_control_a[1] >> 2) & 0x3);
				u32 screen_block = ((lcd_stat->bg_control_a[1] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_a[1] = (char_base * 0x10000) + (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_a[1] = (screen_base * 0x10000) + (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_a[1] = (lcd_stat->bg_control_a[1] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_a[1] = (lcd_stat->bg_control_a[1] >> 14) & 0x3;
			}

			break;

		//BG1 Control B
		case NDS_BG1CNT_B:
		case NDS_BG1CNT_B+1:
			{
				memory_map[address] = value;
				lcd_stat->bg_control_b[1] = (memory_map[NDS_BG1CNT_B+1] << 8) | memory_map[NDS_BG1CNT_B];

				//Determine BG Priority
				lcd_stat->bg_priority_b[1] = lcd_stat->bg_control_b[1] & 0x3;
			
				//Calculate tile data and tile map addresses
				u32 char_base = ((lcd_stat->display_control_b >> 24) & 0x7);
				u32 screen_base = ((lcd_stat->display_control_b >> 27) & 0x7);

				u32 char_block = ((lcd_stat->bg_control_b[1] >> 2) & 0x3);
				u32 screen_block = ((lcd_stat->bg_control_b[1] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_b[1] = (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_b[1] = (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_b[1] = (lcd_stat->bg_control_b[1] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_b[1] = (lcd_stat->bg_control_b[1] >> 14) & 0x3;
			}

			break;

		//BG2 Control A
		case NDS_BG2CNT_A:
		case NDS_BG2CNT_A+1:
			{
				memory_map[address] = value;
				lcd_stat->bg_control_a[2] = (memory_map[NDS_BG2CNT_A+1] << 8) | memory_map[NDS_BG2CNT_A];

				//Determine BG Priority
				lcd_stat->bg_priority_a[2] = lcd_stat->bg_control_a[2] & 0x3;
			
				//Calculate tile data and tile map addresses
				u32 char_base = ((lcd_stat->display_control_a >> 24) & 0x7);
				u32 screen_base = ((lcd_stat->display_control_a >> 27) & 0x7);

				u32 char_block = ((lcd_stat->bg_control_a[2] >> 2) & 0x3);
				u32 screen_block = ((lcd_stat->bg_control_a[2] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_a[2] = (char_base * 0x10000) + (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_a[2] = (screen_base * 0x10000) + (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_a[2] = (lcd_stat->bg_control_a[2] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_a[2] = (lcd_stat->bg_control_a[2] >> 14) & 0x3;
			}

			break;

		//BG2 Control B
		case NDS_BG2CNT_B:
		case NDS_BG2CNT_B+1:
			{
				memory_map[address] = value;
				lcd_stat->bg_control_b[2] = (memory_map[NDS_BG2CNT_B+1] << 8) | memory_map[NDS_BG2CNT_B];

				//Determine BG Priority
				lcd_stat->bg_priority_b[2] = lcd_stat->bg_control_b[2] & 0x3;
			
				//Calculate tile data and tile map addresses
				u32 char_base = ((lcd_stat->display_control_b >> 24) & 0x7);
				u32 screen_base = ((lcd_stat->display_control_b >> 27) & 0x7);

				u32 char_block = ((lcd_stat->bg_control_b[2] >> 2) & 0x3);
				u32 screen_block = ((lcd_stat->bg_control_b[2] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_b[2] = (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_b[2] = (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_b[2] = (lcd_stat->bg_control_b[2] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_b[2] = (lcd_stat->bg_control_b[2] >> 14) & 0x3;
			}

			break;

		//BG3 Control A
		case NDS_BG3CNT_A:
		case NDS_BG3CNT_A+1:
			{
				memory_map[address] = value;
				lcd_stat->bg_control_a[3] = (memory_map[NDS_BG3CNT_A+1] << 8) | memory_map[NDS_BG3CNT_A];

				//Determine BG Priority
				lcd_stat->bg_priority_a[3] = lcd_stat->bg_control_a[3] & 0x3;
			
				//Calculate tile data and tile map addresses
				u32 char_base = ((lcd_stat->display_control_a >> 24) & 0x7);
				u32 screen_base = ((lcd_stat->display_control_a >> 27) & 0x7);

				u32 char_block = ((lcd_stat->bg_control_a[3] >> 2) & 0x3);
				u32 screen_block = ((lcd_stat->bg_control_a[3] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_a[3] = (char_base * 0x10000) + (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_a[3] = (screen_base * 0x10000) + (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_a[3] = (lcd_stat->bg_control_a[3] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_a[3] = (lcd_stat->bg_control_a[3] >> 14) & 0x3;
			}

			break;

		//BG3 Control B
		case NDS_BG3CNT_B:
		case NDS_BG3CNT_B+1:
			{
				memory_map[address] = value;
				lcd_stat->bg_control_b[3] = (memory_map[NDS_BG3CNT_B+1] << 8) | memory_map[NDS_BG3CNT_B];

				//Determine BG Priority
				lcd_stat->bg_priority_b[3] = lcd_stat->bg_control_b[3] & 0x3;
			
				//Calculate tile data and tile map addresses
				u32 char_base = ((lcd_stat->display_control_b >> 24) & 0x7);
				u32 screen_base = ((lcd_stat->display_control_b >> 27) & 0x7);

				u32 char_block = ((lcd_stat->bg_control_b[3] >> 2) & 0x3);
				u32 screen_block = ((lcd_stat->bg_control_b[3] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_b[3] = (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_b[3] = (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_b[3] = (lcd_stat->bg_control_b[3] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_b[3] = (lcd_stat->bg_control_b[3] >> 14) & 0x3;
			}

			break;

		//BG0 Horizontal Offset A
		case NDS_BG0HOFS_A:
		case NDS_BG0HOFS_A+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x_a[0] = ((memory_map[NDS_BG0HOFS_A+1] << 8) | memory_map[NDS_BG0HOFS_A]) & 0x1FF;
			break;

		//BG0 Horizontal Offset B
		case NDS_BG0HOFS_B:
		case NDS_BG0HOFS_B+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x_b[0] = ((memory_map[NDS_BG0HOFS_B+1] << 8) | memory_map[NDS_BG0HOFS_B]) & 0x1FF;
			break;

		//BG0 Vertical Offset A
		case NDS_BG0VOFS_A:
		case NDS_BG0VOFS_A+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y_a[0] = ((memory_map[NDS_BG0VOFS_A+1] << 8) | memory_map[NDS_BG0VOFS_A]) & 0x1FF;
			break;

		//BG0 Vertical Offset B
		case NDS_BG0VOFS_B:
		case NDS_BG0VOFS_B+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y_b[0] = ((memory_map[NDS_BG0VOFS_B+1] << 8) | memory_map[NDS_BG0VOFS_B]) & 0x1FF;
			break;

		//BG1 Horizontal Offset A
		case NDS_BG1HOFS_A:
		case NDS_BG1HOFS_A+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x_a[1] = ((memory_map[NDS_BG1HOFS_A+1] << 8) | memory_map[NDS_BG1HOFS_A]) & 0x1FF;
			break;

		//BG1 Horizontal Offset B
		case NDS_BG1HOFS_B:
		case NDS_BG1HOFS_B+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x_b[1] = ((memory_map[NDS_BG1HOFS_B+1] << 8) | memory_map[NDS_BG1HOFS_B]) & 0x1FF;
			break;

		//BG1 Vertical Offset A
		case NDS_BG1VOFS_A:
		case NDS_BG1VOFS_A+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y_a[1] = ((memory_map[NDS_BG1VOFS_A+1] << 8) | memory_map[NDS_BG1VOFS_A]) & 0x1FF;
			break;

		//BG1 Vertical Offset B
		case NDS_BG1VOFS_B:
		case NDS_BG1VOFS_B+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y_b[1] = ((memory_map[NDS_BG1VOFS_B+1] << 8) | memory_map[NDS_BG1VOFS_B]) & 0x1FF;
			break;

		//BG2 Horizontal Offset A
		case NDS_BG2HOFS_A:
		case NDS_BG2HOFS_A+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x_a[2] = ((memory_map[NDS_BG2HOFS_A+1] << 8) | memory_map[NDS_BG2HOFS_A]) & 0x1FF;
			break;

		//BG2 Horizontal Offset B
		case NDS_BG2HOFS_B:
		case NDS_BG2HOFS_B+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x_b[2] = ((memory_map[NDS_BG2HOFS_B+1] << 8) | memory_map[NDS_BG2HOFS_B]) & 0x1FF;
			break;

		//BG2 Vertical Offset A
		case NDS_BG2VOFS_A:
		case NDS_BG2VOFS_A+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y_a[2] = ((memory_map[NDS_BG2VOFS_A+1] << 8) | memory_map[NDS_BG2VOFS_A]) & 0x1FF;
			break;

		//BG2 Vertical Offset B
		case NDS_BG2VOFS_B:
		case NDS_BG2VOFS_B+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y_b[2] = ((memory_map[NDS_BG2VOFS_B+1] << 8) | memory_map[NDS_BG2VOFS_B]) & 0x1FF;
			break;

		//BG3 Horizontal Offset A
		case NDS_BG3HOFS_A:
		case NDS_BG3HOFS_A+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x_a[3] = ((memory_map[NDS_BG3HOFS_A+1] << 8) | memory_map[NDS_BG3HOFS_A]) & 0x1FF;
			break;

		//BG3 Horizontal Offset B
		case NDS_BG3HOFS_B:
		case NDS_BG3HOFS_B+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_x_b[3] = ((memory_map[NDS_BG3HOFS_B+1] << 8) | memory_map[NDS_BG3HOFS_B]) & 0x1FF;
			break;

		//BG3 Vertical Offset A
		case NDS_BG3VOFS_A:
		case NDS_BG3VOFS_A+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y_a[3] = ((memory_map[NDS_BG3VOFS_A+1] << 8) | memory_map[NDS_BG3VOFS_A]) & 0x1FF;
			break;

		//BG3 Vertical Offset B
		case NDS_BG3VOFS_B:
		case NDS_BG3VOFS_B+1:
			memory_map[address] = value;
			lcd_stat->bg_offset_y_b[3] = ((memory_map[NDS_BG3VOFS_B+1] << 8) | memory_map[NDS_BG3VOFS_B]) & 0x1FF;
			break;

		//BG2 Scale/Rotation Parameter A
		case NDS_BG2PA:
		case NDS_BG2PA+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Parameter B
		case NDS_BG2PB:
		case NDS_BG2PB+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Parameter C
		case NDS_BG2PC:
		case NDS_BG2PC+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Parameter D
		case NDS_BG2PD:
		case NDS_BG2PD+1:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation X Reference
		case NDS_BG2X_L:
		case NDS_BG2X_L+1:
		case NDS_BG2X_L+2:
		case NDS_BG2X_L+3:
			memory_map[address] = value;
			break;

		//BG2 Scale/Rotation Y Reference
		case NDS_BG2Y_L:
		case NDS_BG2Y_L+1:
		case NDS_BG2Y_L+2:
		case NDS_BG2Y_L+3:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Parameter A
		case NDS_BG3PA:
		case NDS_BG3PA+1:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Parameter B
		case NDS_BG3PB:
		case NDS_BG3PB+1:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Parameter C
		case NDS_BG3PC:
		case NDS_BG3PC+1:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Parameter D
		case NDS_BG3PD:
		case NDS_BG3PD+1:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation X Reference
		case NDS_BG3X_L:
		case NDS_BG3X_L+1:
		case NDS_BG3X_L+2:
		case NDS_BG3X_L+3:
			memory_map[address] = value;
			break;

		//BG3 Scale/Rotation Y Reference
		case NDS_BG3Y_L:
		case NDS_BG3Y_L+1:
		case NDS_BG3Y_L+2:
		case NDS_BG3Y_L+3:
			memory_map[address] = value;
			break;

		//Window 0 Horizontal Coordinates
		case NDS_WIN0H:
		case NDS_WIN0H+1:
			memory_map[address] = value;
			break;

		//Window 1 Horizontal Coordinates
		case NDS_WIN1H:
		case NDS_WIN1H+1:
			memory_map[address] = value;
			break;

		//Window 0 Vertical Coordinates
		case NDS_WIN0V:
		case NDS_WIN0V+1:
			memory_map[address] = value;
			break;

		//Window 1 Vertical Coordinates
		case NDS_WIN1V:
		case NDS_WIN1V+1:
			memory_map[address] = value;
			break;

		//Window 0 In Enable Flags
		case NDS_WININ:
			memory_map[address] = value;
			break;

		//Window 1 In Enable Flags
		case NDS_WININ+1:
			memory_map[address] = value;
			break;

		//Window 0 Out Enable Flags
		case NDS_WINOUT:
			memory_map[address] = value;
			break;

		//Window 1 Out Enable Flags
		case NDS_WINOUT+1:
			memory_map[address] = value;
			break;

		//SFX Control
		case NDS_BLDCNT:
			memory_map[address] = value;			
			break;

		case NDS_BLDCNT+1:
			memory_map[address] = value;
			break;

		//SFX Alpha Control
		case NDS_BLDALPHA:
			memory_map[address] = value;
			break;

		case NDS_BLDALPHA+1:
			memory_map[address] = value;
			break;

		//SFX Brightness Control
		case NDS_BLDY:
			memory_map[address] = value;
			break;

		//VRAM Bank Control A-G
		case NDS_VRAMCNT_A:
		case NDS_VRAMCNT_B:
		case NDS_VRAMCNT_C:
		case NDS_VRAMCNT_D:
		case NDS_VRAMCNT_E:
		case NDS_VRAMCNT_F:
		case NDS_VRAMCNT_G:
		case NDS_VRAMCNT_H:
		case NDS_VRAMCNT_I:
			if(access_mode)
			{
				memory_map[address] = value;

				u8 mst = value & 0x7;
				u8 offset = (value >> 3) & 0x3;
				u32 bank_id = (address - 0x4000240); 

				//Bit 2 of MST is unused by banks A, B, H, and I
				if((bank_id < 2) || (bank_id > 6)) { mst &= 0x3; }

				switch(mst)
				{
					//MST 0 - LCDC Mode
					case 0x0:
						switch(bank_id)
						{
							case 0x0: lcd_stat->vram_bank_addr[0] = 0x6800000; break;
							case 0x1: lcd_stat->vram_bank_addr[1] = 0x6820000; break;
							case 0x2: lcd_stat->vram_bank_addr[2] = 0x6840000; break;
							case 0x3: lcd_stat->vram_bank_addr[3] = 0x6860000; break;
							case 0x4: lcd_stat->vram_bank_addr[4] = 0x6880000; break;
							case 0x5: lcd_stat->vram_bank_addr[5] = 0x6890000; break;
							case 0x6: lcd_stat->vram_bank_addr[6] = 0x6894000; break;
							case 0x7: lcd_stat->vram_bank_addr[7] = 0x6898000; break;
							case 0x8: lcd_stat->vram_bank_addr[8] = 0x68A0000; break;
						}
		
						break;

					//MST 1 - 2D Graphics Engine A and B (BG VRAM)
					case 0x1:
						switch(bank_id)
						{
							case 0x0:
							case 0x1:
							case 0x2:
							case 0x3:
								lcd_stat->vram_bank_addr[bank_id] = (0x6000000 + (0x20000 * offset));
								break;

							case 0x4:
								lcd_stat->vram_bank_addr[4] = 0x6000000;
								break;

							case 0x5:
							case 0x6:
								lcd_stat->vram_bank_addr[bank_id] = 0x6000000 + (0x4000 * (offset & 0x1)) + (0x10000 * (offset & 0x2));
								break;

							case 0x7:
								lcd_stat->vram_bank_addr[7] = 0x6200000;
								break;

							case 0x8:
								lcd_stat->vram_bank_addr[8] = 0x6208000;
								break;
						}
							
						break;

					//MST 2 - 2D Graphics Engine A and B (OBJ VRAM)
					case 0x2:
						switch(bank_id)
						{
							case 0x0:
							case 0x1:
								lcd_stat->vram_bank_addr[bank_id] = 0x6400000 + (0x20000 * (offset & 0x1));
								break;

							case 0x2:
							case 0x3:
								lcd_stat->vram_bank_addr[bank_id] = 0x6000000 + (0x20000 * (offset & 0x1));
								break;

							case 0x4:
								lcd_stat->vram_bank_addr[4] = 0x6400000;
								break;

							case 0x5:
							case 0x6:
								lcd_stat->vram_bank_addr[bank_id] = 0x6400000 + (0x4000 * (offset & 0x1)) + (0x10000 * (offset & 0x2));
								break;

							case 0x8:
								lcd_stat->vram_bank_addr[8] = 0x6600000;
								break;
						}

						break;

					//MST 4
					case 0x4:
						switch(bank_id)
						{
							case 0x2:
								lcd_stat->vram_bank_addr[2] = 0x6200000;
								break;

							case 0x3:
								lcd_stat->vram_bank_addr[3] = 0x6600000;
								break;
						}

						break;
				}
			}

			break;

		//DMA0 Start Address
		case NDS_DMA0SAD:
		case NDS_DMA0SAD+1:
		case NDS_DMA0SAD+2:
		case NDS_DMA0SAD+3:

			//NDS9 DMA0 SAD
			if(access_mode)
			{
				dma[0].raw_sad[address & 0x3] = value;
				dma[0].start_address = ((dma[0].raw_sad[3] << 24) | (dma[0].raw_sad[2] << 16) | (dma[0].raw_sad[1] << 8) | dma[0].raw_sad[0]) & 0xFFFFFFF;
			}

			//ND7 DMA0 SAD
			else
			{
				dma[4].raw_sad[address & 0x3] = value;
				dma[4].start_address = ((dma[4].raw_sad[3] << 24) | (dma[4].raw_sad[2] << 16) | (dma[4].raw_sad[1] << 8) | dma[4].raw_sad[0]) & 0x7FFFFFF;
			}

			break;

		//DMA0 Destination Address
		case NDS_DMA0DAD:
		case NDS_DMA0DAD+1:
		case NDS_DMA0DAD+2:
		case NDS_DMA0DAD+3:

			//NDS9 DMA0 DAD
			if(access_mode)
			{
				dma[0].raw_dad[address & 0x3] = value;
				dma[0].destination_address = ((dma[0].raw_dad[3] << 24) | (dma[0].raw_dad[2] << 16) | (dma[0].raw_dad[1] << 8) | dma[0].raw_dad[0]) & 0xFFFFFFF;
			}

			//ND7 DMA0 DAD
			else
			{
				dma[4].raw_dad[address & 0x3] = value;
				dma[4].destination_address = ((dma[4].raw_dad[3] << 24) | (dma[4].raw_dad[2] << 16) | (dma[4].raw_dad[1] << 8) | dma[4].raw_dad[0]) & 0x7FFFFFF;
			}

			break;

		//DMA0 Control
		case NDS_DMA0CNT:
		case NDS_DMA0CNT+1:
		case NDS_DMA0CNT+2:
		case NDS_DMA0CNT+3:
			
			//NDS9 DMA0 CNT
			if(access_mode)
			{
				dma[0].raw_cnt[address & 0x3] = value;
				dma[0].control = ((dma[0].raw_cnt[3] << 24) | (dma[0].raw_cnt[2] << 16) | (dma[0].raw_cnt[1] << 8) | dma[0].raw_cnt[0]);
				dma[0].word_count = dma[0].control & 0x1FFFFF;
				dma[0].dest_addr_ctrl = (dma[0].control >> 21) & 0x3;
				dma[0].src_addr_ctrl = (dma[0].control >> 23) & 0x3;
				dma[0].word_type = (dma[0].control & 0x4000000) ? 1 : 0;
			
				dma[0].enable = true;
				dma[0].started = false;
				dma[0].delay = 2;
			}

			//NDS7 DMA0 CNT
			else
			{
				dma[4].raw_cnt[address & 0x3] = value;
				dma[4].control = ((dma[4].raw_cnt[3] << 24) | (dma[4].raw_cnt[2] << 16) | (dma[4].raw_cnt[1] << 8) | dma[4].raw_cnt[0]);
				dma[4].word_count = dma[4].control & 0x3FFF;
				dma[4].dest_addr_ctrl = (dma[4].control >> 21) & 0x3;
				dma[4].src_addr_ctrl = (dma[4].control >> 23) & 0x3;
				dma[4].word_type = (dma[4].control & 0x4000000) ? 1 : 0;
			
				dma[4].enable = true;
				dma[4].started = false;
				dma[4].delay = 2;
			}

			break;

		//DMA1 Start Address
		case NDS_DMA1SAD:
		case NDS_DMA1SAD+1:
		case NDS_DMA1SAD+2:
		case NDS_DMA1SAD+3:

			//NDS9 DMA1 SAD
			if(access_mode)
			{
				dma[1].raw_sad[address & 0x3] = value;
				dma[1].start_address = ((dma[1].raw_sad[3] << 24) | (dma[1].raw_sad[2] << 16) | (dma[1].raw_sad[1] << 8) | dma[1].raw_sad[0]) & 0xFFFFFFF;
			}

			//ND7 DMA1 SAD
			else
			{
				dma[5].raw_sad[address & 0x3] = value;
				dma[5].start_address = ((dma[5].raw_sad[3] << 24) | (dma[5].raw_sad[2] << 16) | (dma[5].raw_sad[1] << 8) | dma[5].raw_sad[0]) & 0xFFFFFFF;
			}

			break;

		//DMA1 Destination Address
		case NDS_DMA1DAD:
		case NDS_DMA1DAD+1:
		case NDS_DMA1DAD+2:
		case NDS_DMA1DAD+3:

			//NDS9 DMA1 DAD
			if(access_mode)
			{
				dma[1].raw_dad[address & 0x3] = value;
				dma[1].destination_address = ((dma[1].raw_dad[3] << 24) | (dma[1].raw_dad[2] << 16) | (dma[1].raw_dad[1] << 8) | dma[1].raw_dad[0]) & 0xFFFFFFF;
			}

			//ND7 DMA1 DAD
			else
			{
				dma[5].raw_dad[address & 0x3] = value;
				dma[5].destination_address = ((dma[5].raw_dad[3] << 24) | (dma[5].raw_dad[2] << 16) | (dma[5].raw_dad[1] << 8) | dma[5].raw_dad[0]) & 0x7FFFFFF;
			}

			break;

		//DMA1 Control
		case NDS_DMA1CNT:
		case NDS_DMA1CNT+1:
		case NDS_DMA1CNT+2:
		case NDS_DMA1CNT+3:

			//NDS9 DMA1 CNT
			if(access_mode)
			{
				dma[1].raw_cnt[address & 0x3] = value;
				dma[1].control = ((dma[1].raw_cnt[3] << 24) | (dma[1].raw_cnt[2] << 16) | (dma[1].raw_cnt[1] << 8) | dma[1].raw_cnt[0]);
				dma[1].word_count = dma[1].control & 0x1FFFFF;
				dma[1].dest_addr_ctrl = (dma[1].control >> 21) & 0x3;
				dma[1].src_addr_ctrl = (dma[1].control >> 23) & 0x3;
				dma[1].word_type = (dma[1].control & 0x4000000) ? 1 : 0;
			
				dma[1].enable = true;
				dma[1].started = false;
				dma[1].delay = 2;
			}

			//NDS7 DMA1 CNT
			else
			{
				dma[5].raw_cnt[address & 0x3] = value;
				dma[5].control = ((dma[5].raw_cnt[3] << 24) | (dma[5].raw_cnt[2] << 16) | (dma[5].raw_cnt[1] << 8) | dma[5].raw_cnt[0]);
				dma[5].word_count = dma[5].control & 0x3FFF;
				dma[5].dest_addr_ctrl = (dma[5].control >> 21) & 0x3;
				dma[5].src_addr_ctrl = (dma[5].control >> 23) & 0x3;
				dma[5].word_type = (dma[5].control & 0x4000000) ? 1 : 0;
			
				dma[5].enable = true;
				dma[5].started = false;
				dma[5].delay = 2;
			}

			break;

		//DMA2 Start Address
		case NDS_DMA2SAD:
		case NDS_DMA2SAD+1:
		case NDS_DMA2SAD+2:
		case NDS_DMA2SAD+3:

			//NDS9 DMA2 SAD
			if(access_mode)
			{
				dma[2].raw_sad[address & 0x3] = value;
				dma[2].start_address = ((dma[2].raw_sad[3] << 24) | (dma[2].raw_sad[2] << 16) | (dma[2].raw_sad[1] << 8) | dma[2].raw_sad[0]) & 0xFFFFFFF;
			}

			//ND7 DMA2 SAD
			else
			{
				dma[6].raw_sad[address & 0x3] = value;
				dma[6].start_address = ((dma[6].raw_sad[3] << 24) | (dma[6].raw_sad[2] << 16) | (dma[6].raw_sad[1] << 8) | dma[6].raw_sad[0]) & 0xFFFFFFF;
			}

			break;

		//DMA2 Destination Address
		case NDS_DMA2DAD:
		case NDS_DMA2DAD+1:
		case NDS_DMA2DAD+2:
		case NDS_DMA2DAD+3:

			//NDS9 DMA2 DAD
			if(access_mode)
			{
				dma[2].raw_dad[address & 0x3] = value;
				dma[2].destination_address = ((dma[2].raw_dad[3] << 24) | (dma[2].raw_dad[2] << 16) | (dma[2].raw_dad[1] << 8) | dma[2].raw_dad[0]) & 0xFFFFFFF;
			}

			//ND7 DMA2 DAD
			else
			{
				dma[6].raw_dad[address & 0x3] = value;
				dma[6].destination_address = ((dma[6].raw_dad[3] << 24) | (dma[6].raw_dad[2] << 16) | (dma[6].raw_dad[1] << 8) | dma[6].raw_dad[0]) & 0x7FFFFFF;
			}

			break;

		//DMA2 Control
		case NDS_DMA2CNT:
		case NDS_DMA2CNT+1:
		case NDS_DMA2CNT+2:
		case NDS_DMA2CNT+3:

			//NDS9 DMA2 CNT
			if(access_mode)
			{
				dma[2].raw_cnt[address & 0x3] = value;
				dma[2].control = ((dma[2].raw_cnt[3] << 24) | (dma[2].raw_cnt[2] << 16) | (dma[2].raw_cnt[1] << 8) | dma[2].raw_cnt[0]);
				dma[2].word_count = dma[2].control & 0x1FFFFF;
				dma[2].dest_addr_ctrl = (dma[2].control >> 21) & 0x3;
				dma[2].src_addr_ctrl = (dma[2].control >> 23) & 0x3;
				dma[2].word_type = (dma[2].control & 0x4000000) ? 1 : 0;
			
				dma[2].enable = true;
				dma[2].started = false;
				dma[2].delay = 2;
			}

			//NDS7 DMA2 CNT
			else
			{
				dma[6].raw_cnt[address & 0x3] = value;
				dma[6].control = ((dma[6].raw_cnt[3] << 24) | (dma[6].raw_cnt[2] << 16) | (dma[6].raw_cnt[1] << 8) | dma[6].raw_cnt[0]);
				dma[6].word_count = dma[6].control & 0x3FFF;
				dma[6].dest_addr_ctrl = (dma[6].control >> 21) & 0x3;
				dma[6].src_addr_ctrl = (dma[6].control >> 23) & 0x3;
				dma[6].word_type = (dma[6].control & 0x4000000) ? 1 : 0;
			
				dma[6].enable = true;
				dma[6].started = false;
				dma[6].delay = 2;
			}

			break;

		//DMA3 Start Address
		case NDS_DMA3SAD:
		case NDS_DMA3SAD+1:
		case NDS_DMA3SAD+2:
		case NDS_DMA3SAD+3:

			//NDS9 DMA3 SAD
			if(access_mode)
			{
				dma[3].raw_sad[address & 0x3] = value;
				dma[3].start_address = ((dma[3].raw_sad[3] << 24) | (dma[3].raw_sad[2] << 16) | (dma[3].raw_sad[1] << 8) | dma[3].raw_sad[0]) & 0xFFFFFFF;
			}

			//ND7 DMA3 SAD
			else
			{
				dma[7].raw_sad[address & 0x3] = value;
				dma[7].start_address = ((dma[7].raw_sad[3] << 24) | (dma[7].raw_sad[2] << 16) | (dma[7].raw_sad[1] << 8) | dma[7].raw_sad[0]) & 0xFFFFFFF;
			}

			break;

		//DMA3 Destination Address
		case NDS_DMA3DAD:
		case NDS_DMA3DAD+1:
		case NDS_DMA3DAD+2:
		case NDS_DMA3DAD+3:

			//NDS9 DMA3 DAD
			if(access_mode)
			{
				dma[3].raw_dad[address & 0x3] = value;
				dma[3].destination_address = ((dma[3].raw_dad[3] << 24) | (dma[3].raw_dad[2] << 16) | (dma[3].raw_dad[1] << 8) | dma[3].raw_dad[0]) & 0xFFFFFFF;
			}

			//ND7 DMA3 DAD
			else
			{
				dma[7].raw_dad[address & 0x3] = value;
				dma[7].destination_address = ((dma[7].raw_dad[3] << 24) | (dma[7].raw_dad[2] << 16) | (dma[7].raw_dad[1] << 8) | dma[7].raw_dad[0]) & 0xFFFFFFF;
			}

			break;

		//DMA3 Control
		case NDS_DMA3CNT:
		case NDS_DMA3CNT+1:
		case NDS_DMA3CNT+2:
		case NDS_DMA3CNT+3:

			//NDS9 DMA3 CNT
			if(access_mode)
			{
				dma[3].raw_cnt[address & 0x3] = value;
				dma[3].control = ((dma[3].raw_cnt[3] << 24) | (dma[3].raw_cnt[2] << 16) | (dma[3].raw_cnt[1] << 8) | dma[3].raw_cnt[0]);
				dma[3].word_count = dma[3].control & 0x1FFFFF;
				dma[3].dest_addr_ctrl = (dma[3].control >> 21) & 0x3;
				dma[3].src_addr_ctrl = (dma[3].control >> 23) & 0x3;
				dma[3].word_type = (dma[3].control & 0x4000000) ? 1 : 0;
			
				dma[3].enable = true;
				dma[3].started = false;
				dma[3].delay = 2;
			}

			//NDS7 DMA3 CNT
			else
			{
				dma[7].raw_cnt[address & 0x3] = value;
				dma[7].control = ((dma[7].raw_cnt[3] << 24) | (dma[7].raw_cnt[2] << 16) | (dma[7].raw_cnt[1] << 8) | dma[7].raw_cnt[0]);
				dma[7].word_count = dma[7].control & 0xFFFF;
				dma[7].dest_addr_ctrl = (dma[7].control >> 21) & 0x3;
				dma[7].src_addr_ctrl = (dma[7].control >> 23) & 0x3;
				dma[7].word_type = (dma[7].control & 0x4000000) ? 1 : 0;
			
				dma[7].enable = true;
				dma[7].started = false;
				dma[7].delay = 2;
			}

			break;

		case NDS_IME:
			if(access_mode) { nds9_ime = (value & 0x1); }
			else { nds7_ime = (value & 0x1); }
			break;

		case NDS_IME+1:
		case NDS_IME+2:
		case NDS_IME+3:
			break;

		case NDS_IE:
		case NDS_IE+1:
		case NDS_IE+2:
		case NDS_IE+3:
			
			//Write to NDS9 IE
			if(access_mode)
			{
				nds9_ie &= ~(0xFF << ((address & 0x3) << 3));
				nds9_ie |= (value << ((address & 0x3) << 3));
			}

			//Write to NDS7 IE
			else
			{
				nds7_ie &= ~(0xFF << ((address & 0x3) << 3));
				nds7_ie |= (value << ((address & 0x3) << 3));
			}
				
			break;

		case NDS_IF:
		case NDS_IF+1:
		case NDS_IF+2:
		case NDS_IF+3:
			memory_map[address] &= ~value;

			//Write to NDS9 IF
			if(access_mode) { nds9_if &= ~(value << ((address & 0x3) << 3)); }

			//Write to NDS7 IF
			else { nds7_if &= ~(value << ((address & 0x3) << 3)); }

			break;

		case NDS_IPCSYNC: break;

		case NDS_IPCSYNC+1:

			//NDS9 -> NDS7
			if(access_mode)
			{
				//Set new Bits 8-15
				nds9_ipc.sync &= 0xFF;
				nds9_ipc.sync |= ((value & 0x6F) << 8);
				
				//Copy Bits 8-11 from NDS9 to Bits 0-3 on the NDS7
				nds7_ipc.sync &= 0xFFF0;
				nds7_ipc.sync |= (value & 0xF);

				//Trigger IPC IRQ on NDS7 if sending IRQ is enabled
				if((nds9_ipc.sync & 0x2000) && (nds7_ipc.sync & 0x4000))
				{
					nds7_if |= 0x10000;
				}
			}

			//NDS7 -> NDS9
			else
			{
				//Set new Bits 8-15
				nds7_ipc.sync &= 0xFF;
				nds7_ipc.sync |= ((value & 0x6F) << 8);
				
				//Copy Bits 8-11 from NDS7 to Bits 0-3 on the NDS9
				nds9_ipc.sync &= 0xFFF0;
				nds9_ipc.sync |= (value & 0xF);

				//Trigger IPC IRQ on NDS9 if sending IRQ is enabled
				if((nds7_ipc.sync & 0x2000) && (nds9_ipc.sync & 0x4000))
				{
					nds9_if |= 0x10000;
				}
			}

			break;

		case NDS_IPCFIFOCNT:		
			if(access_mode)
			{
				u16 irq_trigger = nds9_ipc.cnt & 0x4;

				nds9_ipc.cnt &= 0xFFF3;
				nds9_ipc.cnt |= (value & 0xC);

				//Raise Send FIFO Empty IRQ when Bit 2 goes from 0 to 1
				if((irq_trigger == 0) && (nds9_ipc.cnt & 0x4) && (nds9_ipc.cnt & 0x1)) { nds9_if |= 0x20000; }
			}

			else
			{
				u16 irq_trigger = nds7_ipc.cnt & 0x4;

				nds7_ipc.cnt &= 0xFFF3;
				nds7_ipc.cnt |= (value & 0xC);

				//Raise Send FIFO Empty IRQ when Bit 2 goes from 0 to 1
				if((irq_trigger == 0) && (nds7_ipc.cnt & 0x4) && (nds7_ipc.cnt & 0x1)) { nds7_if |= 0x20000; }
			}

			break;

		case NDS_IPCFIFOCNT+1:
			if(access_mode)
			{
				u16 irq_trigger = nds9_ipc.cnt & 0x400;

				nds9_ipc.cnt &= 0x3FF;
				nds9_ipc.cnt |= ((value & 0xC4) << 8);

				//Raise Receive FIFO Not Empty IRQ when Bit 10 goes from 0 to 1
				if((irq_trigger == 0) && (nds9_ipc.cnt & 0x400) && ((nds9_ipc.cnt & 0x100) == 0)) { nds9_if |= 0x40000; }
			}

			else
			{
				u16 irq_trigger = nds7_ipc.cnt & 0x400;

				nds7_ipc.cnt &= 0x3FF;
				nds7_ipc.cnt |= ((value & 0xC4) << 8);

				//Raise Receive FIFO Not Empty IRQ when Bit 10 goes from 0 to 1
				if((irq_trigger == 0) && (nds7_ipc.cnt & 0x400) && ((nds7_ipc.cnt & 0x100) == 0)) { nds7_if |= 0x40000; }
			}

			break;

		case NDS_IPCFIFOSND:
			if((access_mode) && (nds9_ipc.cnt & 0x8000))
			{
				nds9_ipc.fifo_incoming &= ~0xFF;
				nds9_ipc.fifo_incoming |= value;

				//FIFO can only hold a maximum of 16 words
				if(nds9_ipc.fifo.size() == 16)
				{
					nds9_ipc.fifo.pop();

					//Set Bit 14 of IPCFIFOCNT to indicate error
					nds9_ipc.cnt |= 0x4000;
				}

				else
				{
					//Unset Bit 14 of IPCFIFOCNT to indicate no errors
					nds9_ipc.cnt &= ~0x4000;
				}

				//Unset SNDFIFO Empty Status on NDS9
				nds9_ipc.cnt &= ~0x1;

				//Unset RECVFIFO Empty Status on NDS7
				nds7_ipc.cnt &= ~0x100;

				//Set RECVFIFO Not Empty IRQ on NDS7
				if((nds7_ipc.cnt & 0x400) && (nds9_ipc.fifo.empty())) { nds7_if |= 0x40000; }

				//Push new word to back of FIFO, also save that value in case FIFO is emptied
				nds9_ipc.fifo.push(nds9_ipc.fifo_incoming);
				nds9_ipc.fifo_latest = nds9_ipc.fifo_incoming;
				nds9_ipc.fifo_incoming = 0;

				//Set Send FIFO Full Status
				if(nds9_ipc.fifo.size() == 16)
				{
					//Set SNDFIFO FULL Status on NDS9
					nds9_ipc.cnt |= 0x2;

					//Set RECVFIFO FULL Status on NDS7
					nds7_ipc.cnt |= 0x200;
				}
			}

			else if((!access_mode) && (nds7_ipc.cnt & 0x8000))
			{
				nds7_ipc.fifo_incoming &= ~0xFF;
				nds7_ipc.fifo_incoming |= value;

				//FIFO can only hold a maximum of 16 words
				if(nds7_ipc.fifo.size() == 16)
				{
					nds7_ipc.fifo.pop();

					//Set Bit 14 of IPCFIFOCNT to indicate error
					nds7_ipc.cnt |= 0x4000;
				}

				else
				{
					//Unset Bit 14 of IPCFIFOCNT to indicate no errors
					nds7_ipc.cnt &= ~0x4000;
				}

				//Unset SNDFIFO Empty Status on NDS7
				nds7_ipc.cnt &= ~0x1;

				//Unset RECVFIFO Empty Status on NDS9
				nds9_ipc.cnt &= ~0x100;

				//Set RECVFIFO Not Empty IRQ on NDS9
				if((nds9_ipc.cnt & 0x400) && (nds7_ipc.fifo.empty())) { nds9_if |= 0x40000; }

				//Push new word to back of FIFO, also save that value in case FIFO is emptied
				nds7_ipc.fifo.push(nds7_ipc.fifo_incoming);
				nds7_ipc.fifo_latest = nds7_ipc.fifo_incoming;
				nds7_ipc.fifo_incoming = 0;

				//Set Send FIFO Full Status
				if(nds7_ipc.fifo.size() == 16)
				{
					//Set SNDFIFO FULL Status on NDS7
					nds7_ipc.cnt |= 0x2;

					//Set RECVFIFO FULL Status on NDS9
					nds9_ipc.cnt |= 0x200;
				}
			}	

			break;

		case NDS_IPCFIFOSND+1:
			if((access_mode) && (nds9_ipc.cnt & 0x8000)) { nds9_ipc.fifo_incoming &= ~0xFF00; nds9_ipc.fifo_incoming |= (value << 8); }
			else if((!access_mode) && (nds7_ipc.cnt & 0x8000)) { nds7_ipc.fifo_incoming &= ~0xFF00; nds7_ipc.fifo_incoming |= (value << 8); }
			break;

		case NDS_IPCFIFOSND+2:
			if((access_mode) && (nds9_ipc.cnt & 0x8000)) { nds9_ipc.fifo_incoming &= ~0xFF0000; nds9_ipc.fifo_incoming |= (value << 16); }
			else if((!access_mode) && (nds7_ipc.cnt & 0x8000)) { nds7_ipc.fifo_incoming &= ~0xFF0000; nds7_ipc.fifo_incoming |= (value << 16); }
			break;

		case NDS_IPCFIFOSND+3:
			if((access_mode) && (nds9_ipc.cnt & 0x8000)) { nds9_ipc.fifo_incoming &= ~0xFF000000; nds9_ipc.fifo_incoming |= (value << 24); }
			else if((!access_mode) && (nds7_ipc.cnt & 0x8000)) { nds7_ipc.fifo_incoming &= ~0xFF000000; nds7_ipc.fifo_incoming |= (value << 24); }
			break;

		case NDS_IPCFIFORECV:
		case NDS_IPCFIFORECV+1:
		case NDS_IPCFIFORECV+2:
		case NDS_IPCFIFORECV+3:
			break;

		case NDS_KEYINPUT:
		case NDS_KEYINPUT+1:
		case NDS_EXTKEYIN:
		case NDS_EXTKEYIN+1:
			break;

		case NDS_DIVCNT:
		case NDS_DIVCNT+1:
			if(access_mode)
			{
				memory_map[address] = value;
				std::cout<<"MMU::NDS_DIVCNT Write\n";
			}

			break;

		case NDS_SQRTCNT:
		case NDS_SQRTCNT+1:
			if(access_mode)
			{
				memory_map[address] = value;
				std::cout<<"MMU::NDS_SQRTCNT Write\n";
			}

			break;

		case NDS_SPICNT:
			if(access_mode) { return; }

			nds7_spi.cnt &= ~0x7F;
			nds7_spi.cnt |= (value & 0x3);

			//Set transfer baud rate
			switch(value & 0x3)
			{
				case 0: nds7_spi.baud_rate = 64; break;
				case 1: nds7_spi.baud_rate = 128; break;
				case 2: nds7_spi.baud_rate = 256; break;
				case 3: nds7_spi.baud_rate = 512; break;
			}

			break;

		case NDS_SPICNT+1:
			if(access_mode) { return; }

			nds7_spi.cnt &= 0xFF;
			nds7_spi.cnt |= ((value & 0xCF) << 8);

			break;

		case NDS_SPIDATA:
			if(access_mode) { return; }

			//Only initialize transfer if SPI bus is enabled, and there is no active transfer
			if((nds7_spi.cnt & 0x8000) && ((nds7_spi.cnt & 0x80) == 0))
			{
				nds7_spi.active_transfer = true;
				nds7_spi.transfer_clock = nds7_spi.baud_rate;
				nds7_spi.cnt |= 0x80;
				nds7_spi.data = value;
			}

			break;

		case NDS_POWERCNT:
			if(access_mode)
			{
				power_cnt1 &= 0xFF00;
				power_cnt1 |= value;
			}

			else
			{
				power_cnt2 &= 0xFF00;
				power_cnt2 |= value;
			}

			break;

		case NDS_POWERCNT+1:
			if(access_mode)
			{
				power_cnt1 &= 0xFF;
				power_cnt1 |= (value << 8);
			}

			else
			{
				power_cnt2 &= 0xFF;
				power_cnt2 |= (value << 8);
			}

			break;

		default:
			memory_map[address] = value;
			break;
	}

	//Trigger BG palette update in LCD - Engine A
	if((address >= 0x5000000) && (address <= 0x50001FF))
	{
		lcd_stat->bg_pal_update_a = true;
		lcd_stat->bg_pal_update_list_a[(address & 0x1FF) >> 1] = true;
	}

	//Trigger BG palette update in LCD - Engine B
	else if((address >= 0x5000400) && (address <= 0x50005FF))
	{
		lcd_stat->bg_pal_update_b = true;
		lcd_stat->bg_pal_update_list_b[(address & 0x1FF) >> 1] = true;
	}
}

/****** Write 2 bytes into memory ******/
void NTR_MMU::write_u16(u32 address, u16 value)
{
	//Always force half-word alignment
	address &= ~0x1;

	write_u8((address+1), ((value >> 8) & 0xFF));
	write_u8(address, (value & 0xFF));
}

/****** Write 4 bytes into memory ******/
void NTR_MMU::write_u32(u32 address, u32 value)
{
	//Always force word alignment
	address &= ~0x3;

	write_u8((address+3), ((value >> 24) & 0xFF));
	write_u8((address+2), ((value >> 16) & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
	write_u8(address, (value & 0xFF));
}

/****** Writes 2 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void NTR_MMU::write_u16_fast(u32 address, u16 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
}

/****** Writes 4 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void NTR_MMU::write_u32_fast(u32 address, u32 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
	memory_map[address+2] = ((value >> 16) & 0xFF);
	memory_map[address+3] = ((value >> 24) & 0xFF);
}	

/****** Read binary file to memory ******/
bool NTR_MMU::read_file(std::string filename)
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

	cart_data.resize(file_size);

	//Read data from the ROM file
	file.read(reinterpret_cast<char*> (&cart_data[0]), file_size);

	//Copy 512 byte header to Main RAM on boot
	for(u32 x = 0; x < 0x200; x++) { write_u8((0x27FFE00 + x), cart_data[x]); }

	file.close();
	std::cout<<"MMU::" << filename << " loaded successfully. \n";

	parse_header();

	//Copy ARM9 binary from offset to RAM address
	for(u32 x = 0; x < header.arm9_size; x++)
	{
		if((header.arm9_rom_offset + x) >= file_size) { break; }
		write_u8((header.arm9_ram_addr + x), cart_data[header.arm9_rom_offset + x]);
	}

	//Copy ARM7 binary from offset to RAM address
	for(u32 x = 0; x < header.arm7_size; x++)
	{
		if((header.arm7_rom_offset + x) >= file_size) { break; }
		write_u8((header.arm7_ram_addr + x), cart_data[header.arm7_rom_offset + x]);
	}

	return true;
}

/****** Read NDS7 BIOS file into memory ******/
bool NTR_MMU::read_bios_nds7(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::NDS7 BIOS file " << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	if(file_size > 0x4000) { std::cout<<"MMU::Warning - Irregular NDS7 BIOS size\n"; }
	
	if(file_size < 0x4000)
	{
		std::cout<<"MMU::Error - NDS7 BIOS size too small\n";
		file.close();
		return false;
	}
	
	u8* ex_mem = &nds7_bios[0];

	//Read data from the ROM file
	file.read((char*)ex_mem, 0x4000);

	file.close();
	std::cout<<"MMU::NDS7 BIOS file " << filename << " loaded successfully. \n";

	return true;
}

/****** Read NDS9 BIOS file into memory ******/
bool NTR_MMU::read_bios_nds9(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::NDS9 BIOS file " << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	if(file_size > 0x1000) { std::cout<<"MMU::Warning - Irregular NDS9 BIOS size\n"; }
	
	if(file_size < 0x1000)
	{
		std::cout<<"MMU::Error - NDS9 BIOS size too small\n";
		file.close();
		return false;
	}
	
	u8* ex_mem = &nds9_bios[0];

	//Read data from the ROM file
	file.read((char*)ex_mem, 0xC00);

	file.close();
	std::cout<<"MMU::NDS9 BIOS file " << filename << " loaded successfully. \n";

	return true;
}

/****** Read firmware file into memory ******/
bool NTR_MMU::read_firmware(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::Firmware file " << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	if(file_size > 0x40000) { std::cout<<"MMU::Warning - Irregular NDS firmware size\n"; }

	if(file_size < 0x40000)
	{
		std::cout<<"MMU::Error - NDS firmware size too small\n";
		file.close();
		return false;
	}
	
	u8* ex_mem = &firmware[0];

	//Read data from the ROM file
	file.read((char*)ex_mem, 0x40000);

	//Copy current settings from firmware to RAM
	for(u8 x = 0; x < 0x70; x++) { write_u8(0x27FFC80, firmware[0x3FE00 + x]); }

	file.close();
	std::cout<<"MMU::NDS firmware file " << filename << " loaded successfully. \n";

	return true;
}

/****** Load backup save data ******/
bool NTR_MMU::load_backup(std::string filename) { return true; }

/****** Save backup save data ******/
bool NTR_MMU::save_backup(std::string filename) { return true; }

/****** Parses cartridge header ******/
void NTR_MMU::parse_header()
{
	//Game title
	header.title = "";
	for(int x = 0; x < 12; x++) { header.title += cart_data[x]; }

	//Game code
	header.game_code = "";
	for(int x = 0; x < 4; x++) { header.game_code += cart_data[0xC + x]; }

	//Maker code
	header.maker_code = "";
	for(int x = 0; x < 2; x++) { header.maker_code += cart_data[0x10 + x]; }

	std::cout<<"MMU::Game Title - " << header.title << "\n";
	std::cout<<"MMU::Game Code - " << header.game_code << "\n";
	std::cout<<"MMU::Maker Code - " << header.maker_code << "\n";

	//ARM9 ROM Offset
	header.arm9_rom_offset = 0;
	for(int x = 0; x < 4; x++) 
	{
		header.arm9_rom_offset <<= 8;
		header.arm9_rom_offset |= cart_data[0x23 - x];
	}

	//ARM9 Entry Address
	header.arm9_entry_addr = 0;
	for(int x = 0; x < 4; x++) 
	{
		header.arm9_entry_addr <<= 8;
		header.arm9_entry_addr |= cart_data[0x27 - x];
	}

	//ARM9 RAM Address
	header.arm9_ram_addr = 0;
	for(int x = 0; x < 4; x++) 
	{
		header.arm9_ram_addr <<= 8;
		header.arm9_ram_addr |= cart_data[0x2B - x];
	}

	//ARM9 Size
	header.arm9_size = 0;
	for(int x = 0; x < 4; x++) 
	{
		header.arm9_size <<= 8;
		header.arm9_size |= cart_data[0x2F - x];
		if(header.arm9_size > 0x3BFE00) { header.arm9_size = 0x3BFE00; }
	}

	//ARM7 ROM Offset
	header.arm7_rom_offset = 0;
	for(int x = 0; x < 4; x++)
	{
		header.arm7_rom_offset <<= 8;
		header.arm7_rom_offset |= cart_data[0x33 - x];
	}

	//ARM7 Entry Address
	header.arm7_entry_addr = 0;
	for(int x = 0; x < 4; x++) 
	{
		header.arm7_entry_addr <<= 8;
		header.arm7_entry_addr |= cart_data[0x37 - x];
		if(header.arm7_size > 0x3BFE00) { header.arm7_size = 0x3BFE00; }
	}

	//ARM7 RAM Address
	header.arm7_ram_addr = 0;
	for(int x = 0; x < 4; x++) 
	{
		header.arm7_ram_addr <<= 8;
		header.arm7_ram_addr |= cart_data[0x3B - x];
	}

	//ARM7 Size
	header.arm7_size = 0;
	for(int x = 0; x < 4; x++) 
	{
		header.arm7_size <<= 8;
		header.arm7_size |= cart_data[0x3F - x];
	}
}

/****** Handles various SPI Bus interactions ******/
void NTR_MMU::process_spi_bus()
{
	//Send data across SPI bus for each component
	//Process that byte and return data in SPIDATA
	switch((nds7_spi.cnt >> 8) & 0x3)
	{
		//Power Management
		case 0:
			//std::cout<<"MMU::Power Management write -> 0x" << nds7_spi.data << "\n";
			break;

		//Firmware
		case 1:
			//std::cout<<"MMU::Firmware write -> 0x" << nds7_spi.data << "\n";
			process_firmware();
			break;

		//Touchscreen
		case 2:
			//std::cout<<"MMU::Touchscreen write -> 0x" << nds7_spi.data << "\n";
			process_touchscreen();
			break;

		//Reserved
		case 3:
			std::cout<<"MMU::Warning - Writing to reserved device via SPI\n";
			break;
	}

	//Reset SPI for next transfer
	nds7_spi.active_transfer = false;
	nds7_spi.transfer_clock = 0;
	nds7_spi.cnt &= ~0x80;

	//Raise IRQ on ARM7 if necessary
	if(nds7_spi.cnt & 0x4000) { nds7_if |= 0x800000; }
}

/****** Handles read and write data operations to the firmware ******/
void NTR_MMU::process_firmware()
{
	//Check to see if a new command has been sent
	switch(nds7_spi.data)
	{
		//Read Data
		case 0x3:
			firmware_state = 0x0300;
			firmware_index = 0;
			return;

	}

	//Process various firmware states
	switch(firmware_state)
	{
		//Set read address byte 1
		case 0x0300:
			firmware_index |= ((nds7_spi.data & 0xFF) << 16);
			firmware_state++;
			break;

		//Set read address byte 2
		case 0x0301:
			firmware_index |= ((nds7_spi.data & 0xFF) << 8);
			firmware_state++;
			break;

		//Set read address byte 3
		case 0x0302:
			firmware_index |= (nds7_spi.data & 0xFF);
			firmware_state++;
			break;

		//Read byte from firmware index
		case 0x0303:
			if(firmware_index < 0x40000) { nds7_spi.data = firmware[firmware_index++]; }
			break;
	}
}

/****** Handles read and write data operations to the touchscreen controller ******/
void NTR_MMU::process_touchscreen()
{
	//Check to see if the control byte was accessed
	if(nds7_spi.data & 0x80)
	{
		//Read data from the touchscreen's channels
		u8 channel = (nds7_spi.data >> 4) & 0x7;
		touchscreen_state = (channel << 1);

		return;
	}

	u16 touch_x = g_pad->mouse_x;
	u16 touch_y = g_pad->mouse_y;

	//Convert touch screen coordinate via ADC
	touch_x = (((touch_x - touchscreen.scr_x1 + 1) * (touchscreen.adc_x2 - touchscreen.adc_x1)) / (touchscreen.scr_x2 - touchscreen.scr_x1)) + touchscreen.adc_x1;
	touch_y = (((touch_y - touchscreen.scr_y1 + 1) * (touchscreen.adc_y2 - touchscreen.adc_y1)) / (touchscreen.scr_y2 - touchscreen.scr_y1)) + touchscreen.adc_y1;

	touch_x &= 0xFFF;
	touch_y &= 0xFFF;

	//Process various touchscreen states
	switch(touchscreen_state)
	{
		//Read Touch Y Byte 1
		case 0x2:
			nds7_spi.data = (touch_y >> 5);
			touchscreen_state++;
			break;

		//Read Touch Y Byte 2
		case 0x3:
			nds7_spi.data = ((touch_y & 0x1F) << 3);
			touchscreen_state++;
			break;

		//Read Touch X Byte 1
		case 0xA:
			nds7_spi.data = (touch_x >> 5);
			touchscreen_state++;
			break;

		//Read Touch X Byte 2
		case 0xB:
			nds7_spi.data = ((touch_x & 0x1F) << 3);
			touchscreen_state++;
			break;

		default: nds7_spi.data = 0;
	}
}

/****** Sets up a default firmware via HLE ******/
void NTR_MMU::setup_default_firmware()
{
	//Header - Console Type - Original NDS
	firmware[0x1D] = 0xFF;
	firmware[0x1E] = 0xFF;
	firmware[0x1F] = 0xFF;

	//Header - User settings offset - 0x3FE00 aka User Settings Area 1
	firmware[0x20] = 0x7F;
	firmware[0x21] = 0xC0;

	//User Settings Area 1 - Version - Always 0x5
	firmware[0x3FE00] = 0x5;

	//User Settings Area 1 - Favorite Color - We like blue
	firmware[0x3FE02] = 0xB;

	//User Settings Area 1 - Our bday is April 1st
	firmware[0x3FE03] = 0x4;
	firmware[0x3FE04] = 0x1;

	//User Settings Area 1 - Nickname - GBE+
	firmware[0x3FE06] = 0x47;
	firmware[0x3FE08] = 0x42;
	firmware[0x3FE0A] = 0x45;
	firmware[0x3FE0C] = 0x2B;

	//User Settings Area 1 - Touchscreen calibration points
	firmware[0x3FE58] = 0xA4;
	firmware[0x3FE59] = 0x02;
	firmware[0x3FE5A] = 0xF4;
	firmware[0x3FE5B] = 0x02;
	firmware[0x3FE5C] = 0x20;
	firmware[0x3FE5D] = 0x20;
	firmware[0x3FE5E] = 0x24;
	firmware[0x3FE5F] = 0x0D;
	firmware[0x3FE60] = 0xE0;
	firmware[0x3FE61] = 0x0C;
	firmware[0x3FE62] = 0xE0;
	firmware[0x3FE63] = 0xA0;

	//User Settings Area 1 - Nickname length
	firmware[0x3FE1A] = 0x4;

	//Copy firmware user settings to RAM
	for(u32 x = 0; x < 0x6C; x++)
	{
		write_u8((0x27FFC80 + x), firmware[0x3FE00 + x]);
	} 
}
	
/****** Points the MMU to an lcd_data structure (FROM THE LCD ITSELF) ******/
void NTR_MMU::set_lcd_data(ntr_lcd_data* ex_lcd_stat) { lcd_stat = ex_lcd_stat; }
