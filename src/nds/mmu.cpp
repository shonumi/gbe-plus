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

	//Default access mode starts off with NDS9
	access_mode = 1;

	nds7_bios.clear();
	nds7_bios.resize(0x4000, 0);
	nds7_bios_vector = 0x0;
	nds7_irq_handler = 0x380FFFC;
	nds7_ie = 0x0;
	nds7_if = 0x0;

	nds9_bios.clear();
	nds9_bios.resize(0xC00, 0);
	nds9_bios_vector = 0xFFFF0000;
	nds9_irq_handler = 0x0;
	nds9_ie = 0x0;
	nds9_if = 0x0;

	//HLE stuff
	memory_map[NDS_DISPCNT_A] = 0x80;

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

	std::cout<<"MMU::Initialized\n";
}

/****** Read byte from memory ******/
u8 NTR_MMU::read_u8(u32 address) const
{
	//Read from NDS9 BIOS
	if((address >= nds9_bios_vector) && (address <= (nds9_bios_vector + 0xC00)) && (access_mode)) { return nds9_bios[address - nds9_bios_vector]; }

	//Read from NDS7 BIOS
	if((address >= nds7_bios_vector) && (address <= (nds7_bios_vector + 0x4000)) && (!access_mode)) { return nds7_bios[address - nds7_bios_vector]; }

	//Check for unused memory first
	else if(address >= 0x10000000) { return 0; std::cout<<"Out of bounds read : 0x" << std::hex << address << "\n"; return 0; }

	//Check for reading IE
	else if((address & ~0x3) == NDS_IE)
	{
		//Return NDS9 IE
		if(access_mode) { return nds9_ie & (0xFF << ((address & 0x3) << 3)); }
		
		//Return NDS7 IE
		else { return nds7_ie & (0xFF << ((address & 0x3) << 3)); }
	}

	//Check for reading IF
	else if((address & ~0x3) == NDS_IF)
	{
		//Return NDS9 IF
		if(access_mode) { return nds9_if & (0xFF << ((address & 0x3) << 3)); }
		
		//Return NDS7 IF
		else { return nds7_if & (0xFF << ((address & 0x3) << 3)); }
	}

	return memory_map[address];
}

/****** Read 2 bytes from memory ******/
u16 NTR_MMU::read_u16(u32 address) const
{
	return ((read_u8(address+1) << 8) | read_u8(address)); 
}

/****** Read 4 bytes from memory ******/
u32 NTR_MMU::read_u32(u32 address) const
{
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
	//Check for unused memory first
	if(address >= 0x10000000) { return; std::cout<<"Out of bounds write : 0x" << std::hex << address << "\n"; return; }

	switch(address)
	{
		//Display Control
		case NDS_DISPCNT_A:
		case NDS_DISPCNT_A+1:
		case NDS_DISPCNT_A+2:
		case NDS_DISPCNT_A+3:
			memory_map[address] = value;
			lcd_stat->display_control_a = ((memory_map[NDS_DISPCNT_A+3] << 24) | (memory_map[NDS_DISPCNT_A+2] << 16) | (memory_map[NDS_DISPCNT_A+1] << 8) | memory_map[NDS_DISPCNT_A]);
			lcd_stat->bg_mode_a = (lcd_stat->display_control_a & 0x7);
			lcd_stat->display_mode_a = (lcd_stat->display_control_a >> 16) & 0x3;
			break;

		//Display Status
		case NDS_DISPSTAT:
			{
				u8 read_only_bits = (memory_map[NDS_DISPSTAT] & 0x7);
				
				memory_map[address] = (value & ~0x7);
				memory_map[address] |= read_only_bits;

				lcd_stat->vblank_irq_enable = (value & 0x8) ? true : false;
				lcd_stat->hblank_irq_enable = (value & 0x10) ? true : false;
				lcd_stat->vcount_irq_enable = (value & 0x20) ? true : false;
			}
 
			break;

		//BG0 Control
		case NDS_BG0CNT:
		case NDS_BG0CNT+1:
			memory_map[address] = value;
			break;

		//BG1 Control
		case NDS_BG1CNT:
		case NDS_BG1CNT+1:
			memory_map[address] = value;
			break;

		//BG2 Control
		case NDS_BG2CNT:
		case NDS_BG2CNT+1:
			memory_map[address] = value;
			break;

		//BG3 Control
		case NDS_BG3CNT:
		case NDS_BG3CNT+1:
			memory_map[address] = value;
			break;

		//BG0 Horizontal Offset
		case NDS_BG0HOFS:
		case NDS_BG0HOFS+1:
			memory_map[address] = value;
			break;

		//BG0 Vertical Offset
		case NDS_BG0VOFS:
		case NDS_BG0VOFS+1:
			memory_map[address] = value;
			break;

		//BG1 Horizontal Offset
		case NDS_BG1HOFS:
		case NDS_BG1HOFS+1:
			memory_map[address] = value;
			break;

		//BG1 Vertical Offset
		case NDS_BG1VOFS:
		case NDS_BG1VOFS+1:
			memory_map[address] = value;
			break;

		//BG2 Horizontal Offset
		case NDS_BG2HOFS:
		case NDS_BG2HOFS+1:
			memory_map[address] = value;
			break;

		//BG2 Vertical Offset
		case NDS_BG2VOFS:
		case NDS_BG2VOFS+1:
			memory_map[address] = value;
			break;

		//BG3 Horizontal Offset
		case NDS_BG3HOFS:
		case NDS_BG3HOFS+1:
			memory_map[address] = value;
			break;

		//BG3 Vertical Offset
		case NDS_BG3VOFS:
		case NDS_BG3VOFS+1:
			memory_map[address] = value;
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

		//VRAM Bank A Control
		case NDS_VRAMCNT_A:
			memory_map[address] = value;
			{
				u8 offset = (value >> 3) & 0x3;

				switch(value & 0x3)
				{
					case 0x0: lcd_stat->vram_bank_addr[0] = 0x6800000; break;
					case 0x1: lcd_stat->vram_bank_addr[0] = (0x6000000 + (0x20000 * offset)); break;
				}
			}

			break;

		//DMA0 Start Address
		case NDS_DMA0SAD:
		case NDS_DMA0SAD+1:
		case NDS_DMA0SAD+2:
		case NDS_DMA0SAD+3:
			memory_map[address] = value;
			dma[0].start_address = ((memory_map[NDS_DMA0SAD+3] << 24) | (memory_map[NDS_DMA0SAD+2] << 16) | (memory_map[NDS_DMA0SAD+1] << 8) | memory_map[NDS_DMA0SAD]) & 0xFFFFFFF;
			break;

		//DMA0 Destination Address
		case NDS_DMA0DAD:
		case NDS_DMA0DAD+1:
		case NDS_DMA0DAD+2:
		case NDS_DMA0DAD+3:
			memory_map[address] = value;
			dma[0].destination_address = ((memory_map[NDS_DMA0DAD+3] << 24) | (memory_map[NDS_DMA0DAD+2] << 16) | (memory_map[NDS_DMA0DAD+1] << 8) | memory_map[NDS_DMA0DAD]) & 0xFFFFFFF;
			break;

		//DMA0 Control
		case NDS_DMA0CNT:
		case NDS_DMA0CNT+1:
		case NDS_DMA0CNT+2:
		case NDS_DMA0CNT+3:
			memory_map[address] = value;
			dma[0].control = ((memory_map[NDS_DMA0CNT+3] << 24) | (memory_map[NDS_DMA0CNT+2] << 16) | (memory_map[NDS_DMA0CNT+1] << 8) | memory_map[NDS_DMA0CNT]);
			dma[0].word_count = dma[0].control & 0x1FFFFF;
			dma[0].dest_addr_ctrl = (dma[0].control >> 21) & 0x3;
			dma[0].src_addr_ctrl = (dma[0].control >> 23) & 0x3;
			dma[0].word_type = (dma[0].control & 0x4000000) ? 1 : 0;
			
			dma[0].enable = true;
			dma[0].started = false;
			dma[0].delay = 2;
			break;

		//DMA1 Start Address
		case NDS_DMA1SAD:
		case NDS_DMA1SAD+1:
		case NDS_DMA1SAD+2:
		case NDS_DMA1SAD+3:
			memory_map[address] = value;
			dma[1].start_address = ((memory_map[NDS_DMA1SAD+3] << 24) | (memory_map[NDS_DMA1SAD+2] << 16) | (memory_map[NDS_DMA1SAD+1] << 8) | memory_map[NDS_DMA1SAD]) & 0xFFFFFFF;
			break;

		//DMA1 Destination Address
		case NDS_DMA1DAD:
		case NDS_DMA1DAD+1:
		case NDS_DMA1DAD+2:
		case NDS_DMA1DAD+3:
			memory_map[address] = value;
			dma[1].destination_address = ((memory_map[NDS_DMA1DAD+3] << 24) | (memory_map[NDS_DMA1DAD+2] << 16) | (memory_map[NDS_DMA1DAD+1] << 8) | memory_map[NDS_DMA1DAD]) & 0xFFFFFFF;
			break;

		//DMA1 Control
		case NDS_DMA1CNT:
		case NDS_DMA1CNT+1:
		case NDS_DMA1CNT+2:
		case NDS_DMA1CNT+3:
			memory_map[address] = value;
			dma[1].control = ((memory_map[NDS_DMA1CNT+3] << 24) | (memory_map[NDS_DMA1CNT+2] << 16) | (memory_map[NDS_DMA1CNT+1] << 8) | memory_map[NDS_DMA1CNT]);
			dma[1].word_count = dma[1].control & 0x1FFFFF;
			dma[1].dest_addr_ctrl = (dma[1].control >> 21) & 0x3;
			dma[1].src_addr_ctrl = (dma[1].control >> 23) & 0x3;
			dma[1].word_type = (dma[1].control & 0x4000000) ? 1 : 0;
			
			dma[1].enable = true;
			dma[1].started = false;
			dma[1].delay = 2;
			break;

		//DMA2 Start Address
		case NDS_DMA2SAD:
		case NDS_DMA2SAD+1:
		case NDS_DMA2SAD+2:
		case NDS_DMA2SAD+3:
			memory_map[address] = value;
			dma[2].start_address = ((memory_map[NDS_DMA2SAD+3] << 24) | (memory_map[NDS_DMA2SAD+2] << 16) | (memory_map[NDS_DMA2SAD+1] << 8) | memory_map[NDS_DMA2SAD]) & 0xFFFFFFF;
			break;

		//DMA2 Destination Address
		case NDS_DMA2DAD:
		case NDS_DMA2DAD+1:
		case NDS_DMA2DAD+2:
		case NDS_DMA2DAD+3:
			memory_map[address] = value;
			dma[2].destination_address = ((memory_map[NDS_DMA2DAD+3] << 24) | (memory_map[NDS_DMA2DAD+2] << 16) | (memory_map[NDS_DMA2DAD+1] << 8) | memory_map[NDS_DMA2DAD]) & 0xFFFFFFF;
			break;

		//DMA2 Control
		case NDS_DMA2CNT:
		case NDS_DMA2CNT+1:
		case NDS_DMA2CNT+2:
		case NDS_DMA2CNT+3:
			memory_map[address] = value;
			dma[2].control = ((memory_map[NDS_DMA2CNT+3] << 24) | (memory_map[NDS_DMA2CNT+2] << 16) | (memory_map[NDS_DMA2CNT+1] << 8) | memory_map[NDS_DMA2CNT]);
			dma[2].word_count = dma[2].control & 0x1FFFFF;
			dma[2].dest_addr_ctrl = (dma[2].control >> 21) & 0x3;
			dma[2].src_addr_ctrl = (dma[2].control >> 23) & 0x3;
			dma[2].word_type = (dma[2].control & 0x4000000) ? 1 : 0;
			
			dma[2].enable = true;
			dma[2].started = false;
			dma[2].delay = 2;
			break;

		//DMA3 Start Address
		case NDS_DMA3SAD:
		case NDS_DMA3SAD+1:
		case NDS_DMA3SAD+2:
		case NDS_DMA3SAD+3:
			memory_map[address] = value;
			dma[3].start_address = ((memory_map[NDS_DMA3SAD+3] << 24) | (memory_map[NDS_DMA3SAD+2] << 16) | (memory_map[NDS_DMA3SAD+1] << 8) | memory_map[NDS_DMA3SAD]) & 0xFFFFFFF;
			break;

		//DMA3 Destination Address
		case NDS_DMA3DAD:
		case NDS_DMA3DAD+1:
		case NDS_DMA3DAD+2:
		case NDS_DMA3DAD+3:
			memory_map[address] = value;
			dma[3].destination_address = ((memory_map[NDS_DMA3DAD+3] << 24) | (memory_map[NDS_DMA3DAD+2] << 16) | (memory_map[NDS_DMA3DAD+1] << 8) | memory_map[NDS_DMA3DAD]) & 0xFFFFFFF;
			break;

		//DMA3 Control
		case NDS_DMA3CNT:
		case NDS_DMA3CNT+1:
		case NDS_DMA3CNT+2:
		case NDS_DMA3CNT+3:
			memory_map[address] = value;
			dma[3].control = ((memory_map[NDS_DMA3CNT+3] << 24) | (memory_map[NDS_DMA3CNT+2] << 16) | (memory_map[NDS_DMA3CNT+1] << 8) | memory_map[NDS_DMA3CNT]);
			dma[3].word_count = dma[3].control & 0x1FFFFF;
			dma[3].dest_addr_ctrl = (dma[3].control >> 21) & 0x3;
			dma[3].src_addr_ctrl = (dma[3].control >> 23) & 0x3;
			dma[3].word_type = (dma[3].control & 0x4000000) ? 1 : 0;
			
			dma[3].enable = true;
			dma[3].started = false;
			dma[3].delay = 2;
			break;

		case NDS_IME:
		case NDS_IME+1:
			memory_map[address] = value;
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
				std::cout<<"NDS9 IE -> 0x" << std::hex << nds9_ie << "\n";
			}

			//Write to NDS7 IE
			else
			{
				nds7_ie &= ~(0xFF << ((address & 0x3) << 3));
				nds7_ie |= (value << ((address & 0x3) << 3));
				std::cout<<"NDS7 IE -> 0x" << std::hex << nds7_ie << "\n";
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
			else { nds7_ie &= ~(0xFF << ((address & 0x3) << 3)); }

			break;

		case NDS_IPCSYNC: break;

		case NDS_IPCSYNC+1:
			memory_map[address] = value;

			//Copy Bits 8-11 to Bits 0-3
			memory_map[NDS_IPCSYNC] &= ~0xF;
			memory_map[NDS_IPCSYNC] |= (value & 0xF);

			//Trigger IPC IRQ
			if((value & 0x20) && (value & 0x40))
			{
				if(access_mode)
				{
					nds9_if |= 0x10000;
					std::cout<<"NDS7 to NDS9 IRQ\n";
				}
				
				else
				{
					nds7_if |= 0x10000;
					std::cout<<"NDS9 to NDS7 IRQ\n";
				}
			}


			std::cout<<"NDS IPCSYNC -> 0x" << std::hex << read_u16(NDS_IPCSYNC) << "\n";
			break;

		case NDS_IPCFIFOCNT:
		case NDS_IPCFIFOCNT+1:
			memory_map[address] = value;
			std::cout<<"NDS IPCFIFOCNT -> 0x" << std::hex << read_u16(NDS_IPCFIFOCNT) << "\n";
			break;

		case NDS_IPCFIFOSND:
		case NDS_IPCFIFOSND+1:
		case NDS_IPCFIFOSND+2:
		case NDS_IPCFIFOSND+3:
			memory_map[address] = value;
			std::cout<<"NDS IPCFIFOSND\n";
			break;

		case NDS_IPCFIFORECV:
		case NDS_IPCFIFORECV+1:
		case NDS_IPCFIFORECV+2:
		case NDS_IPCFIFORECV+3:
			break;

		default:
			memory_map[address] = value;
			break;

		//Trigger BG palette update in LCD - Engine A
		if((address >= 0x5000000) && (address <= 0x50001FF))
		{
			lcd_stat->bg_pal_update_a = true;
			lcd_stat->bg_pal_update_list_a[(address & 0x1FF) >> 1] = true;
		}
	}
}

/****** Write 2 bytes into memory ******/
void NTR_MMU::write_u16(u32 address, u16 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
}

/****** Write 4 bytes into memory ******/
void NTR_MMU::write_u32(u32 address, u32 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), ((value >> 8) & 0xFF));
	write_u8((address+2), ((value >> 16) & 0xFF));
	write_u8((address+3), ((value >> 24) & 0xFF));
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
	for(u32 x = 0; x < 0x200; x++) { memory_map[0x27FFE00 + x] = cart_data[x]; }

	file.close();
	std::cout<<"MMU::" << filename << " loaded successfully. \n";

	parse_header();

	//Copy ARM9 binary from offset to entry address
	for(u32 x = 0; x < header.arm9_size; x++)
	{
		if((header.arm9_rom_offset + x) >= file_size) { break; }
		memory_map[header.arm9_entry_addr + x] = cart_data[header.arm9_rom_offset + x];
	}

	//Copy ARM7 binary from offset to entry address
	for(u32 x = 0; x < header.arm7_size; x++)
	{
		if((header.arm7_rom_offset + x) >= file_size) { break; }
		memory_map[header.arm7_entry_addr + x] = cart_data[header.arm7_rom_offset + x];
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

/****** Points the MMU to an lcd_data structure (FROM THE LCD ITSELF) ******/
void NTR_MMU::set_lcd_data(ntr_lcd_data* ex_lcd_stat) { lcd_stat = ex_lcd_stat; }
