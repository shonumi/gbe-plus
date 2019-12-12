// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.h
// Date : September 14, 2015
// Description : NDS memory manager unit
//
// Handles reading and writing bytes to memory locations

#include "mmu.h"
#include "common/util.h"

#include <cmath>

/****** MMU Constructor ******/
NTR_MMU::NTR_MMU() 
{
	reset();
}

/****** MMU Deconstructor ******/
NTR_MMU::~NTR_MMU() 
{
	save_backup(config::save_file);
	memory_map.clear();
	cart_data.clear();
	nds7_bios.clear();
	nds9_bios.clear();
	std::cout<<"MMU::Shutdown\n"; 
}

/****** MMU Reset ******/
void NTR_MMU::reset()
{
	current_save_type = AUTO;
	gba_save_type = GBA_NONE;

	switch(config::nds_slot2_device)
	{
		case 0: current_slot2_device = SLOT2_AUTO; break;
		case 1: current_slot2_device = SLOT2_NONE; break;
		case 2: current_slot2_device = SLOT2_PASSME; break;
		case 3: current_slot2_device = SLOT2_GBA_CART; break;
	}	

	memory_map.clear();
	memory_map.resize(0x10000000, 0);

	cart_data.clear();

	firmware.clear();
	firmware.resize(0x40000, 0);

	save_data.clear();
	save_data.resize(0x10000, 0xFF);

	nds7_bios.clear();
	nds7_bios.resize(0x4000, 0);
	nds7_bios_vector = 0x0;
	nds7_irq_handler = 0x380FFFC;
	nds7_ie = 0x0;
	nds7_if = 0x0;
	nds7_temp_if = 0x0;
	nds7_ime = 0;
	nds7_exmem = 0;
	power_cnt2 = 0;

	nds9_bios.clear();
	nds9_bios.resize(0xC00, 0);
	nds9_bios_vector = 0xFFFF0000;
	nds9_irq_handler = 0x0;
	nds9_ie = 0x0;
	nds9_if = 0x0;
	nds9_temp_if = 0x0;
	nds9_ime = 0;
	nds9_exmem = 0;
	power_cnt1 = 0;

	//HLE MMIO stuff
	if(!config::use_bios || !config::use_firmware)
	{
		memory_map[NDS_DISPCNT_A] = 0x80;
		memory_map[NDS_POSTFLG] = 0x1;

		write_u16_fast(NDS_BG2PA_A, 0x100);
		write_u16_fast(NDS_BG2PD_A, 0x100);
		write_u16_fast(NDS_BG3PA_A, 0x100);
		write_u16_fast(NDS_BG3PD_A, 0x100);

		write_u16_fast(NDS_BG2PA_B, 0x100);
		write_u16_fast(NDS_BG2PD_B, 0x100);
		write_u16_fast(NDS_BG3PA_B, 0x100);
		write_u16_fast(NDS_BG3PD_B, 0x100);
	
		//NDS7 BIOS CRC
		write_u16_fast(0x23FF850, 0x5835);
		write_u16_fast(0x23FFC10, 0x5835);

		//NDS9 to NDS7 message
		write_u32_fast(0x23FF880, 0x7);

		//NDS7 Boot Task
		write_u32_fast(0x23FF884, 0x6);

		//Misc Boot Flags
		write_u32_fast(0x23FF890, 0xB0002A22);

		//Frame Counter
		write_u32_fast(0x23FFC3C, 0x332);

		//Boot Status
		write_u16_fast(0x23FFC40, 0x1);

		//Setup EXMEM + default access mode to NDS9
		access_mode = 0;
		write_u16(NDS_EXMEM, 0xE880);
		access_mode = 1;
		write_u16(NDS_EXMEM, 0xE880);
	}

	//HLE firmware stuff
	if(!config::use_firmware) { setup_default_firmware(); }

	firmware_state = 0;
	firmware_index = 0;

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

	nds_aux_spi.cnt = 0;
	nds_aux_spi.data = 0;
	nds_aux_spi.transfer_count = 0;
	nds_aux_spi.eeprom_stat = 0x0;
	nds_aux_spi.backup_cmd = 0;
	nds_aux_spi.backup_cmd_ready = true;
	nds_aux_spi.state = 0;
	nds_aux_spi.last_state = 0;

	nds_card.cnt = 0x800000;
	nds_card.data = 0;
	nds_card.baud_rate = 0;
	nds_card.transfer_clock = 0;
	nds_card.transfer_src = 0;
	nds_card.transfer_count = 0;
	nds_card.state = 0;
	nds_card.last_state = 0;
	nds_card.active_transfer = false;
	nds_card.cmd_lo = 0;
	nds_card.cmd_hi = 0;
	nds_card.chip_id = 0xFC2;

	nds7_rtc.cnt = 0;
	nds7_rtc.data = 0;
	nds7_rtc.io_direction = 0;
	nds7_rtc.state = 0x100;
	nds7_rtc.serial_counter = 0;
	nds7_rtc.serial_byte = 0;
	nds7_rtc.data_index = 0;
	nds7_rtc.serial_len = 0;
	nds7_rtc.read_stat = 0;
	nds7_rtc.reg_index = 0;
	nds7_rtc.regs[0] = 0x2;
	
	for(int x = 1; x < 10; x++) { nds7_rtc.regs[x] = 0; }

	nds7_rtc.int1_clock = 0;
	nds7_rtc.int1_freq = 0;
	nds7_rtc.int1_enable = false;
	nds7_rtc.int2_enable = false;

	nds9_math.div_numer = 0;
	nds9_math.div_denom = 0;
	nds9_math.div_result = 0;
	nds9_math.div_remainder = 0;
	nds9_math.sqrt_param = 0;
	nds9_math.sqrt_result = 0;

	touchscreen_state = 0;

	key1_table.clear();
	key_code.clear();
	key_level = 0;
	key_id = 0;

	g_pad = NULL;
	nds9_timer = NULL;
	nds7_timer = NULL;

	dtcm_addr = 0xDEADC0DE;
	itcm_addr = 0;

	dtcm.clear();
	dtcm.resize(0x4000, 0);

	pal_a_bg_slot[0] = 0x6880000;
	pal_a_bg_slot[1] = 0x6890000;
	pal_a_bg_slot[2] = 0x6894000;
	pal_a_bg_slot[3] = 0x6894000;

	pal_b_bg_slot[0] = 0x6898000;
	pal_b_bg_slot[1] = 0x689A000;
	pal_b_bg_slot[2] = 0x689C000;
	pal_b_bg_slot[3] = 0x689E000;

	access_mode = 1;
	wram_mode = 3;
	do_save = false;

	//Advanced debugging
	#ifdef GBE_DEBUG
	debug_read = false;
	debug_write = false;
	debug_addr[0] = 0;
	debug_addr[1] = 0;
	debug_addr[2] = 0;
	debug_addr[3] = 0;
	debug_access = 0;
	#endif

	std::cout<<"MMU::Initialized\n";
}

/****** Read byte from memory ******/
u8 NTR_MMU::read_u8(u32 address)
{
	//Advanced debugging
	#ifdef GBE_DEBUG
	debug_read = true;
	debug_addr[address & 0x3] = address;
	debug_access = (access_mode) ? 0 : 1;
	#endif

	//Check DTCM first
	if((access_mode) && (address >= dtcm_addr) && (address <= (dtcm_addr + 0x3FFF)))
	{
		return dtcm[address - dtcm_addr];
	}

	//Mirror memory address if applicable
	switch(address >> 24)
	{
		case 0x0:
			if(access_mode) { address &= 0x7FFF; }
			
			else
			{
				//NDS7 BIOS read
				if(address < 0x4000) { return nds7_bios[address - nds7_bios_vector]; }		
			}

			break;

		case 0x2:
			address &= 0x23FFFFF;
			break;

		case 0x3:
			//ARM9 WRAM reading
			if(access_mode)
			{
				switch(wram_mode)
				{
					case 0x0: address &= 0x3007FFF; break;
					case 0x1: address = 0x3004000 | (address & 0x3FFF); break;
					case 0x2: address = 0x3000000 | (address & 0x3FFF); break;
					case 0x3: return 0;
				}
			}

			//ARM7 WRAM reading
			else if((!access_mode) && (address <= 0x37FFFFF))
			{
				switch(wram_mode)
				{
					case 0x0: address = 0x380FFFF | (address & 0xFFFF); break;
					case 0x1: address = 0x3000000 | (address & 0x3FFF); break;
					case 0x2: address = 0x3004000 | (address & 0x3FFF); break;
					case 0x3: address &= 0x3007FFF; break;
				}
			}

			else { address &= 0x380FFFF; }

			break;

		case 0x4:
			if((!access_mode) && (address >= 0x4000400) && (address < 0x4000500))
			{
				apu_io_id = (address >> 4) & 0xF;
				address &= 0x400040F;
			}

			break;

		case 0x5:
			if(access_mode) { address &= 0x5007FFF; }
			break;

		case 0x7:
			if(access_mode) { address &= 0x7007FFF; }
			break;

		case 0x8:
			if(current_slot2_device == SLOT2_PASSME)
			{
				if((address & 0x7FFFFFF) < cart_data.size()) { return cart_data[address & 0x7FFFFFF]; }
				else { return 0xFF; }
			}

			else { return 0xFF; }

			break;

		case 0xFF:
			//NDS9 BIOS read
			if((access_mode) && (address < 0xFFFF0C00)) { return nds9_bios[address - nds9_bios_vector]; }
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

	//Check for unused memory first
	if(address >= 0x10000000)
	{
		if(access_mode) { std::cout<<"ARM9::Out of bounds read : 0x" << std::hex << address << "\n"; }
		else { std::cout<<"ARM7::Out of bounds read : 0x" << std::hex << address << "\n"; }

		return 0;
	}

	//Check for reading DISPSTAT
	else if((address & ~0x1) == NDS_DISPSTAT)
	{
		u8 addr_shift = (address & 0x1) << 3;

		//Return NDS9 DISPSTAT
		if(access_mode) { return ((lcd_stat->display_stat_nds9 >> addr_shift) & 0xFF); }
		
		//Return NDS7 DISPSTAT
		else { return ((lcd_stat->display_stat_nds7 >> addr_shift) & 0xFF); }
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

	//Check for Timer registers
	else if((address & ~0xF) == NDS_TM0CNT_L)
	{
		u8 timer_id = ((address & 0xF) >> 2);
		u8 addr_shift = (address & 0x1) << 3;
		bool timer_cnt = (address & 0x2) ? true : false;

		//NDS9 and NDS7 timer control
		if(access_mode && timer_cnt) { return ((nds9_timer->at(timer_id).cnt >> addr_shift) & 0xFF); }
		else if(!access_mode && timer_cnt) { return ((nds7_timer->at(timer_id).cnt >> addr_shift) & 0xFF); }

		//NDS9 and NDS7 timer counter
		else if(access_mode && !timer_cnt) { return ((nds9_timer->at(timer_id).counter >> addr_shift) & 0xFF); }
		else { return ((nds7_timer->at(timer_id).counter >> addr_shift) & 0xFF); }
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

	//Check for AUXSPICNT
	else if((address & ~0x1) == NDS_AUXSPICNT)
	{
		if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
		{
			u8 addr_shift = (address & 0x1) << 3;
			return ((nds_aux_spi.cnt >> addr_shift) & 0xFF);
		}

		else { return 0; }
	}

	//Check for AUXSPIDATA
	else if((address & ~0x1) == NDS_AUXSPIDATA)
	{
		if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
		{
			return memory_map[address];
		}
	}

	//Check for ROMCNT
	else if((address & ~0x3) == NDS_ROMCNT)
	{
		if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
		{
			u8 addr_shift = (address & 0x3) << 3;
			return ((nds_card.cnt >> addr_shift) & 0xFF);
		}
	}

	//Check for NDS_CARDCMD_LO
	else if((address & ~0x3) == NDS_CARDCMD_LO)
	{
		if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
		{
			u8 addr_shift = (address & 0x3) << 3;
			return ((nds_card.cmd_lo >> addr_shift) & 0xFF);
		}
	}

	//Check for NDS_CARDCMD_HI
	else if((address & ~0x3) == NDS_CARDCMD_HI)
	{
		if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
		{
			u8 addr_shift = (address & 0x3) << 3;
			return ((nds_card.cmd_hi >> addr_shift) & 0xFF);
		}
	}

	//Check for NDS_CARD_DATA
	else if((address & ~0x3) == NDS_CARD_DATA)
	{
		if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
		{
			nds_card.transfer_count++;
			u8 ret_val = memory_map[address];

			if(nds_card.transfer_count == 4)
			{
				process_card_bus();
				nds_card.transfer_count = 0;
			}

			return ret_val;
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

	//Check for EXMEMCNT - EXMEMSTAT
	else if((address & ~0x1) == NDS_EXMEM)
	{
		u8 addr_shift = (address & 0x1) << 3;

		if(access_mode) { return ((nds9_exmem >> addr_shift) & 0xFF); }
		else { return ((nds7_exmem >> addr_shift) & 0xFF); }
	}

	//Check for POWERCNT
	else if((address & ~0x1) == NDS_POWERCNT)
	{
		u8 addr_shift = (address & 0x1) << 3;

		if(access_mode) { return ((power_cnt1 >> addr_shift) & 0xFF); }
		else { return ((power_cnt2 >> addr_shift) & 0xFF); }
	}

	//Check POSTFLG - NDS7
	else if((address == NDS_POSTFLG) && (!access_mode)) { return memory_map[address] & 0x1; }

	//Check for SOUNDXCNT - NDS7
	else if((address & ~0x3) == NDS_SOUNDXCNT)
	{
		//Only NDS7 can access this register, return 0 for NDS9
		if(access_mode) { return 0; }

		u8 addr_shift = (address & 0x3) << 3;
		return ((apu_stat->channel[apu_io_id].cnt >> addr_shift) & 0xFF);
	}

	//Check for DMA0CNT
	else if((address & ~0x3) == NDS_DMA0CNT)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[0].control >> addr_shift) & 0xFF); }
		else { return ((dma[4].control >> addr_shift) & 0xFF); }
	}

	//Check for DMA0SAD
	else if((address & ~0x3) == NDS_DMA0SAD)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[0].start_address >> addr_shift) & 0xFF); }
		else { return ((dma[4].start_address >> addr_shift) & 0xFF); }
	}

	//Check for DMA0DAD
	else if((address & ~0x3) == NDS_DMA0DAD)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[0].destination_address >> addr_shift) & 0xFF); }
		else { return ((dma[4].destination_address >> addr_shift) & 0xFF); }
	}
		
	//Check for DMA1CNT
	else if((address & ~0x3) == NDS_DMA1CNT)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[1].control >> addr_shift) & 0xFF); }
		else { return ((dma[5].control >> addr_shift) & 0xFF); }
	}

	//Check for DMA1SAD
	else if((address & ~0x3) == NDS_DMA1SAD)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[1].start_address >> addr_shift) & 0xFF); }
		else { return ((dma[5].start_address >> addr_shift) & 0xFF); }
	}

	//Check for DMA1DAD
	else if((address & ~0x3) == NDS_DMA1DAD)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[1].destination_address >> addr_shift) & 0xFF); }
		else { return ((dma[5].destination_address >> addr_shift) & 0xFF); }
	}

	//Check for DMA2CNT
	else if((address & ~0x3) == NDS_DMA2CNT)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[2].control >> addr_shift) & 0xFF); }
		else { return ((dma[6].control >> addr_shift) & 0xFF); }
	}

	//Check for DMA2SAD
	else if((address & ~0x3) == NDS_DMA2SAD)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[2].start_address >> addr_shift) & 0xFF); }
		else { return ((dma[6].start_address >> addr_shift) & 0xFF); }
	}

	//Check for DMA2DAD
	else if((address & ~0x3) == NDS_DMA2DAD)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[2].destination_address >> addr_shift) & 0xFF); }
		else { return ((dma[6].destination_address >> addr_shift) & 0xFF); }
	}

	//Check for DMA3CNT
	else if((address & ~0x3) == NDS_DMA3CNT)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[3].control >> addr_shift) & 0xFF); }
		else { return ((dma[7].control >> addr_shift) & 0xFF); }
	}

	//Check for DMA3SAD
	else if((address & ~0x3) == NDS_DMA3SAD)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[3].start_address >> addr_shift) & 0xFF); }
		else { return ((dma[7].start_address >> addr_shift) & 0xFF); }
	}

	//Check for DMA3DAD
	else if((address & ~0x3) == NDS_DMA3DAD)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((dma[3].destination_address >> addr_shift) & 0xFF); }
		else { return ((dma[7].destination_address >> addr_shift) & 0xFF); }
	}

	//Check for DISP3DCNT
	else if((address & ~0x3) == NDS_DISP3DCNT)
	{
		u8 addr_shift = (address & 0x3) << 3;
		
		if(access_mode) { return ((lcd_3D_stat->display_control >> addr_shift) & 0xFF); }
		else { return 0; }
	}

	//Check for RAMCOUNT
	else if((address & ~0x3) == NDS_GXRAM_COUNT)
	{
		u8 addr_shift = (address & 0x3) << 3;

		if(access_mode) { return ((((lcd_3D_stat->vert_count << 8) | lcd_3D_stat->poly_count)  >> addr_shift) & 0xFF); }
		else { return 0; }
	}

	else if(address == NDS_RTC)
	{
		return (access_mode) ? 0 : read_rtc();
	}
	
	return memory_map[address];
}

/****** Read 2 bytes from memory ******/
u16 NTR_MMU::read_u16(u32 address)
{
	address &= ~0x1;
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
	address &= ~0x1;
	return ((memory_map[address+1] << 8) | memory_map[address]);
}

/****** Reads 4 bytes from memory - No checks done on the read, used for known memory locations such as registers ******/
u32 NTR_MMU::read_u32_fast(u32 address) const
{
	return ((memory_map[address+3] << 24) | (memory_map[address+2] << 16) | (memory_map[address+1] << 8) | memory_map[address]);
}

/****** Reads 2 bytes from cartridge memory - No checks done on the read ******/
u16 NTR_MMU::read_cart_u16(u32 address) const
{
	return ((cart_data[address+1] << 8) | cart_data[address]);
}

/****** Reads 4 bytes from cartridge memory - No checks done on the read ******/
u32 NTR_MMU::read_cart_u32(u32 address) const
{
	return ((cart_data[address+3] << 24) | (cart_data[address+2] << 16) | (cart_data[address+1] << 8) | cart_data[address]);
}

/****** Write byte into memory ******/
void NTR_MMU::write_u8(u32 address, u8 value)
{
	//Advanced debugging
	#ifdef GBE_DEBUG
	debug_write = true;
	debug_addr[address & 0x3] = address;
	debug_access = (access_mode) ? 0 : 1;
	#endif

	//Check DTCM first
	if((access_mode) && (address >= dtcm_addr) && (address <= (dtcm_addr + 0x3FFF)))
	{
		dtcm[address - dtcm_addr] = value;
	}

	//Mirror memory address if applicable
	//Or narrow down certain I/O regs (sound)
	switch(address >> 24)
	{
		case 0x2:
			address &= 0x23FFFFF;
			break;

		case 0x3:
			//ARM9 WRAM reading
			if(access_mode)
			{
				switch(wram_mode)
				{
					case 0x0: address &= 0x3007FFF; break;
					case 0x1: address = 0x3004000 | (address & 0x3FFF); break;
					case 0x2: address = 0x3000000 | (address & 0x3FFF); break;
					case 0x3: return;
				}
			}

			//ARM7 WRAM reading
			else if((!access_mode) && (address <= 0x37FFFFF))
			{
				switch(wram_mode)
				{
					case 0x0: address = 0x380FFFF | (address & 0xFFFF); break;
					case 0x1: address = 0x3000000 | (address & 0x3FFF); break;
					case 0x2: address = 0x3004000 | (address & 0x3FFF); break;
					case 0x3: address &= 0x3007FFF; break;
				}
			}

			else { address &= 0x380FFFF; }

			break;

		case 0x4:
			if((!access_mode) && (address >= 0x4000400) && (address < 0x4000500))
			{
				apu_io_id = (address >> 4) & 0xF;
				address &= 0x400040F;
			}

			//Some 3D on NDS9 and sound on NDS7 share I/O addresses
			//Process NDS9 stuff here
			if((access_mode) && (address >= 0x4000400) && (address < 0x4000520) && (power_cnt1 & 0x8))
			{
				switch(address)
				{
					//3D GX FIFO
					case NDS_GXFIFO:
					case NDS_GXFIFO+1:
					case NDS_GXFIFO+2:
					case NDS_GXFIFO+3:
						
						if(nds9_gx_fifo.size() == 1280) { nds9_gx_fifo.pop(); }
						nds9_gx_fifo.push(value);

						//Determine if new command is packed or unpacked
						if((!lcd_3D_stat->process_command) && (address & 0x3))
						{
							lcd_3D_stat->packed_command = (value) ? true : false;
							lcd_3D_stat->process_command = true;
						}

						//TODO - Break down parameters
						//TODO - Actually process commands
						
						if((address & 0x3) == 0)
						{
							lcd_3D_stat->process_command = false;
						}

						break;

					//MTX_MODE
					case 0x4000440:
						//std::cout<<"GX - MTX_MODE\n";
						lcd_3D_stat->current_gx_command = 0x10;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 1) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_PUSH
					case 0x4000444:
						//std::cout<<"GX - MTX_PUSH -> " << (u16)lcd_3D_stat->matrix_mode << "\n";
						lcd_3D_stat->current_gx_command = 0x11;
						lcd_3D_stat->process_command = true;

						break;

					//MTX_POP
					case 0x4000448:
					case 0x4000449:
					case 0x400044A:
					case 0x400044B:
						//std::cout<<"GX - MTX_POP -> " << (u16)lcd_3D_stat->matrix_mode << "\n";
						lcd_3D_stat->current_gx_command = 0x12;
						lcd_3D_stat->process_command = true;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_STORE
					case 0x400044C:
					case 0x400044D:
					case 0x400044E:
					case 0x400044F:
						//std::cout<<"GX - MTX_STORE -> " << (u16)lcd_3D_stat->matrix_mode << "\n";
						lcd_3D_stat->current_gx_command = 0x13;
						lcd_3D_stat->process_command = true;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_RESTORE
					case 0x4000450:
					case 0x4000451:
					case 0x4000452:
					case 0x4000453:
						//std::cout<<"GX - MTX_RESTORE -> " << (u16)lcd_3D_stat->matrix_mode << "\n";
						lcd_3D_stat->current_gx_command = 0x14;
						lcd_3D_stat->process_command = true;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }

						break;
						
					//MTX_IDENTITY:
					case 0x4000454:
						//std::cout<<"GX - MTX_IDENTITY\n";
						lcd_3D_stat->current_gx_command = 0x15;
						lcd_3D_stat->process_command = true;

						break;

					//MTX_LOAD_4x4
					case 0x4000458:
					case 0x4000459:
					case 0x400045A:
					case 0x400045B:
						//std::cout<<"GX - MTX_LOAD_4x4\n";
						lcd_3D_stat->current_gx_command = 0x16;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 64) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_LOAD_4x3
					case 0x400045C:
					case 0x400045D:
					case 0x400045E:
					case 0x400045F:
						//std::cout<<"GX - MTX_LOAD_4x3\n";
						lcd_3D_stat->current_gx_command = 0x17;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 48) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_MULT_4x4
					case 0x4000460:
					case 0x4000461:
					case 0x4000462:
					case 0x4000463:
						//std::cout<<"GX - MTX_MULT_4x4\n";
						lcd_3D_stat->current_gx_command = 0x18;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 64) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_MULT_4x3
					case 0x4000464:
					case 0x4000465:
					case 0x4000466:
					case 0x4000467:
						//std::cout<<"GX - MTX_MULT_4x3\n";
						lcd_3D_stat->current_gx_command = 0x19;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 48) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_MULT_3x3
					case 0x4000468:
					case 0x4000469:
					case 0x400046A:
					case 0x400046B:
						//std::cout<<"GX - MTX_MULT_3x3\n";
						lcd_3D_stat->current_gx_command = 0x1A;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 36) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_SCALE
					case 0x400046C:
					case 0x400046D:
					case 0x400046E:
					case 0x400046F:
						//std::cout<<"GX - MTX_SCALE\n";
						lcd_3D_stat->current_gx_command = 0x1B;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 12) { lcd_3D_stat->process_command = true; }

						break;

					//MTX_TRANS
					case 0x4000470:
					case 0x4000471:
					case 0x4000472:
					case 0x4000473:
						//std::cout<<"GX - MTX_TRANS ->" << (u16)lcd_3D_stat->matrix_mode << "\n";
						lcd_3D_stat->current_gx_command = 0x1C;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 12) { lcd_3D_stat->process_command = true; }

						break;

					//VERT_COLOR
					case 0x4000480:
					case 0x4000481:
					case 0x4000482:
					case 0x4000483:
						//std::cout<<"GX - VERT_COLOR -> " << (u16)value << "\n";
						lcd_3D_stat->current_gx_command = 0x20;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }

						break;

					//VTX_16
					case 0x400048C:
					case 0x400048D:
					case 0x400048E:
					case 0x400048F:
						//std::cout<<"GX - VTX_16\n";
						lcd_3D_stat->current_gx_command = 0x23;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 8) { lcd_3D_stat->process_command = true; }

						break;

					//VTX_10
					case 0x4000490:
					case 0x4000491:
					case 0x4000492:
					case 0x4000493:
						//std::cout<<"GX - VTX_10\n";
						lcd_3D_stat->current_gx_command = 0x24;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }

						break;

					//VTX_XY
					case 0x4000494:
					case 0x4000495:
					case 0x4000496:
					case 0x4000497:
						//std::cout<<"GX - VTX_XY\n";
						lcd_3D_stat->current_gx_command = 0x25;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }

						break;

					//VTX_XZ
					case 0x4000498:
					case 0x4000499:
					case 0x400049A:
					case 0x400049B:
						//std::cout<<"GX - VTX_XZ\n";
						lcd_3D_stat->current_gx_command = 0x26;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }

						break;

					//VTX_YZ
					case 0x400049C:
					case 0x400049D:
					case 0x400049E:
					case 0x400049F:
						//std::cout<<"GX - VTX_YZ\n";
						lcd_3D_stat->current_gx_command = 0x27;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }

						break;

					//BEGIN_VTXS
					case 0x4000500:
					case 0x4000501:
					case 0x4000502:
					case 0x4000503:
						//std::cout<<"GX - BEGIN_VTXS -> " << (value & 0x3) << "\n";
						lcd_3D_stat->current_gx_command = 0x40;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }
						
						break;

					//END_VTXS
					case 0x4000504:
					case 0x4000505:
					case 0x4000506:
					case 0x4000507:
						lcd_3D_stat->current_gx_command = 0x41;
						lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
						if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }
						//std::cout<<"GX - END_VTXS\n";
						break;
				}
			}

			break;

		case 0x5:
			if(access_mode) { address &= 0x5007FFF; }
			break;

		case 0x7:
			if(access_mode) { address &= 0x7007FFF; }
			break;
	}

	//Check for unused memory first
	if(address >= 0x10000000)
	{
		if(access_mode) { std::cout<<"ARM9::Out of bounds write : 0x" << std::hex << address << "\n"; }
		else { std::cout<<"ARM7::Out of bounds write : 0x" << std::hex << address << "\n"; }

		return;
	}

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

			//Forced Blank
			lcd_stat->forced_blank_a = (lcd_stat->display_control_a & 0x80) ? true : false;
			lcd_stat->display_mode_a = (lcd_stat->forced_blank_a) ? (lcd_stat->display_mode_a | 0x80) : (lcd_stat->display_mode_a & ~0x80);

			//Enable or disable BGs
			lcd_stat->bg_enable_a[0] = (lcd_stat->display_control_a & 0x100) ? true : false;
			lcd_stat->bg_enable_a[1] = (lcd_stat->display_control_a & 0x200) ? true : false;
			lcd_stat->bg_enable_a[2] = (lcd_stat->display_control_a & 0x400) ? true : false;
			lcd_stat->bg_enable_a[3] = (lcd_stat->display_control_a & 0x800) ? true : false;

			//Enable or disable windows
			lcd_stat->window_enable_a[0] = (lcd_stat->display_control_a & 0x2000) ? true : false;
			lcd_stat->window_enable_a[1] = (lcd_stat->display_control_a & 0x4000) ? true : false;
			lcd_stat->obj_win_enable_a = (lcd_stat->display_control_a & 0x8000) ? true : false;

			//Extended palettes
			lcd_stat->ext_pal_a = lcd_stat->display_control_a >> 30;

			//Update all BG controls
			lcd_stat->update_bg_control_a = true;

			//Calculate OBJ VRAM boundaries
			if(lcd_stat->display_control_a & 0x10) { lcd_stat->obj_boundary_a = 32 << ((lcd_stat->display_control_a >> 20) & 0x3); }
			else { lcd_stat->obj_boundary_a = 32; }

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

			//Forced Blank
			lcd_stat->forced_blank_b = (lcd_stat->display_control_b & 0x80) ? true : false;
			lcd_stat->display_mode_b = (lcd_stat->forced_blank_b) ? (lcd_stat->display_mode_b | 0x80) : (lcd_stat->display_mode_b & ~0x80);

			//Enable or disable BGs
			lcd_stat->bg_enable_b[0] = (lcd_stat->display_control_b & 0x100) ? true : false;
			lcd_stat->bg_enable_b[1] = (lcd_stat->display_control_b & 0x200) ? true : false;
			lcd_stat->bg_enable_b[2] = (lcd_stat->display_control_b & 0x400) ? true : false;
			lcd_stat->bg_enable_b[3] = (lcd_stat->display_control_b & 0x800) ? true : false;

			//Enable or disable windows
			lcd_stat->window_enable_b[0] = (lcd_stat->display_control_b & 0x2000) ? true : false;
			lcd_stat->window_enable_b[1] = (lcd_stat->display_control_b & 0x4000) ? true : false;
			lcd_stat->obj_win_enable_b = (lcd_stat->display_control_b & 0x8000) ? true : false;

			//Extended palettes
			lcd_stat->ext_pal_b = lcd_stat->display_control_b >> 30;

			//Update all BG controls
			lcd_stat->update_bg_control_b = true;

			//Calculate OBJ VRAM boundaries
			if(lcd_stat->display_control_b & 0x10) { lcd_stat->obj_boundary_b = 32 << ((lcd_stat->display_control_b >> 20) & 0x3); }
			else { lcd_stat->obj_boundary_b = 32; }

			break;

		//Display Status
		case NDS_DISPSTAT:
			if(access_mode)
			{	
				lcd_stat->display_stat_nds9 &= 0xFF07;
				lcd_stat->display_stat_nds9 |= (value & ~0x7);

				lcd_stat->vblank_irq_enable_a = (value & 0x8) ? true : false;
				lcd_stat->hblank_irq_enable_a = (value & 0x10) ? true : false;
				lcd_stat->vcount_irq_enable_a = (value & 0x20) ? true : false;

				//MSB of LYC is Bit 7 of DISPSTAT
				lcd_stat->lyc_nds9 &= ~0x100;
				if(value & 0x80) { lcd_stat->lyc_nds9 |= 0x100; }
			}

			else
			{	
				lcd_stat->display_stat_nds7 &= 0xFF07;
				lcd_stat->display_stat_nds7 |= (value & ~0x7);

				lcd_stat->vblank_irq_enable_b = (value & 0x8) ? true : false;
				lcd_stat->hblank_irq_enable_b = (value & 0x10) ? true : false;
				lcd_stat->vcount_irq_enable_b = (value & 0x20) ? true : false;

				//MSB of LYC is Bit 7 of DISPSTAT
				lcd_stat->lyc_nds7 &= ~0x100;
				if(value & 0x80) { lcd_stat->lyc_nds7 |= 0x100; }
			}
 
			break;

		//Display Status - Lower 8 bits of LYC
		case NDS_DISPSTAT+1:
			if(access_mode)
			{
				lcd_stat->display_stat_nds9 &= 0xFF;
				lcd_stat->display_stat_nds9 |= (value << 8);

				lcd_stat->lyc_nds9 &= ~0xFF;
				lcd_stat->lyc_nds9 |= value;
			}

			else
			{
				lcd_stat->display_stat_nds7 &= 0xFF;
				lcd_stat->display_stat_nds7 |= (value << 8);

				lcd_stat->lyc_nds7 &= ~0xFF;
				lcd_stat->lyc_nds7 |= value;
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

				switch(lcd_stat->bg_size_a[0])
				{
					case 0x0: lcd_stat->text_width_a[0] = 255; lcd_stat->text_height_a[0] = 255; break;
					case 0x1: lcd_stat->text_width_a[0] = 511; lcd_stat->text_height_a[0] = 255; break;
					case 0x2: lcd_stat->text_width_a[0] = 255; lcd_stat->text_height_a[0] = 511; break;
					case 0x3: lcd_stat->text_width_a[0] = 511; lcd_stat->text_height_a[0] = 511; break;
				}
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

				switch(lcd_stat->bg_size_b[0])
				{
					case 0x0: lcd_stat->text_width_b[0] = 255; lcd_stat->text_height_b[0] = 255; break;
					case 0x1: lcd_stat->text_width_b[0] = 511; lcd_stat->text_height_b[0] = 255; break;
					case 0x2: lcd_stat->text_width_b[0] = 255; lcd_stat->text_height_b[0] = 511; break;
					case 0x3: lcd_stat->text_width_b[0] = 511; lcd_stat->text_height_b[0] = 511; break;
				}
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

				u32 char_block = ((lcd_stat->bg_control_a[1] >> 2) & 0xF);
				u32 screen_block = ((lcd_stat->bg_control_a[1] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_a[1] = (char_base * 0x10000) + (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_a[1] = (screen_base * 0x10000) + (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_a[1] = (lcd_stat->bg_control_a[1] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_a[1] = (lcd_stat->bg_control_a[1] >> 14) & 0x3;

				switch(lcd_stat->bg_size_a[1])
				{
					case 0x0: lcd_stat->text_width_a[1] = 255; lcd_stat->text_height_a[1] = 255; break;
					case 0x1: lcd_stat->text_width_a[1] = 511; lcd_stat->text_height_a[1] = 255; break;
					case 0x2: lcd_stat->text_width_a[1] = 255; lcd_stat->text_height_a[1] = 511; break;
					case 0x3: lcd_stat->text_width_a[1] = 511; lcd_stat->text_height_a[1] = 511; break;
				}
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

				u32 char_block = ((lcd_stat->bg_control_b[1] >> 2) & 0xF);
				u32 screen_block = ((lcd_stat->bg_control_b[1] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_b[1] = (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_b[1] = (screen_block * 0x800);

				//Bit-depth
				lcd_stat->bg_depth_b[1] = (lcd_stat->bg_control_b[1] & 0x80) ? 1 : 0;

				//Screen size
				lcd_stat->bg_size_b[1] = (lcd_stat->bg_control_b[1] >> 14) & 0x3;

				switch(lcd_stat->bg_size_b[1])
				{
					case 0x0: lcd_stat->text_width_b[1] = 255; lcd_stat->text_height_b[1] = 255; break;
					case 0x1: lcd_stat->text_width_b[1] = 511; lcd_stat->text_height_b[1] = 255; break;
					case 0x2: lcd_stat->text_width_b[1] = 255; lcd_stat->text_height_b[1] = 511; break;
					case 0x3: lcd_stat->text_width_b[1] = 511; lcd_stat->text_height_b[1] = 511; break;
				}
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

				u32 char_block = ((lcd_stat->bg_control_a[2] >> 2) & 0xF);
				u32 screen_block = ((lcd_stat->bg_control_a[2] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_a[2] = (char_base * 0x10000) + (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_a[2] = (screen_base * 0x10000) + (screen_block * 0x800);
				lcd_stat->bg_bitmap_base_addr_a[0] = 0x6000000 + (screen_block * 0x4000);

				//Bit-depth
				lcd_stat->bg_depth_a[2] = (lcd_stat->bg_control_a[2] & 0x80) ? 1 : 0;

				//Affine overflow
				lcd_stat->bg_affine_a[0].overflow = (lcd_stat->bg_control_a[2] & 0x2000) ? true : false;

				//Screen size
				lcd_stat->bg_size_a[2] = (lcd_stat->bg_control_a[2] >> 14) & 0x3;

				switch(lcd_stat->bg_size_a[2])
				{
					case 0x0: lcd_stat->text_width_a[2] = 255; lcd_stat->text_height_a[2] = 255; break;
					case 0x1: lcd_stat->text_width_a[2] = 511; lcd_stat->text_height_a[2] = 255; break;
					case 0x2: lcd_stat->text_width_a[2] = 255; lcd_stat->text_height_a[2] = 511; break;
					case 0x3: lcd_stat->text_width_a[2] = 511; lcd_stat->text_height_a[2] = 511; break;
				}
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

				u32 char_block = ((lcd_stat->bg_control_b[2] >> 2) & 0xF);
				u32 screen_block = ((lcd_stat->bg_control_b[2] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_b[2] = (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_b[2] = (screen_block * 0x800);
				lcd_stat->bg_bitmap_base_addr_b[0] = 0x6200000 + (screen_block * 0x4000);

				//Bit-depth
				lcd_stat->bg_depth_b[2] = (lcd_stat->bg_control_b[2] & 0x80) ? 1 : 0;

				//Affine overflow
				lcd_stat->bg_affine_b[0].overflow = (lcd_stat->bg_control_b[2] & 0x2000) ? true : false;

				//Screen size
				lcd_stat->bg_size_b[2] = (lcd_stat->bg_control_b[2] >> 14) & 0x3;

				switch(lcd_stat->bg_size_b[2])
				{
					case 0x0: lcd_stat->text_width_b[2] = 255; lcd_stat->text_height_b[2] = 255; break;
					case 0x1: lcd_stat->text_width_b[2] = 511; lcd_stat->text_height_b[2] = 255; break;
					case 0x2: lcd_stat->text_width_b[2] = 255; lcd_stat->text_height_b[2] = 511; break;
					case 0x3: lcd_stat->text_width_b[2] = 511; lcd_stat->text_height_b[2] = 511; break;
				}
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

				u32 char_block = ((lcd_stat->bg_control_a[3] >> 2) & 0xF);
				u32 screen_block = ((lcd_stat->bg_control_a[3] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_a[3] = (char_base * 0x10000) + (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_a[3] = (screen_base * 0x10000) + (screen_block * 0x800);
				lcd_stat->bg_bitmap_base_addr_a[1] = 0x6000000 + (screen_block * 0x4000);

				//Bit-depth
				lcd_stat->bg_depth_a[3] = (lcd_stat->bg_control_a[3] & 0x80) ? 1 : 0;

				//Affine overflow
				lcd_stat->bg_affine_a[1].overflow = (lcd_stat->bg_control_a[3] & 0x2000) ? true : false;

				//Screen size
				lcd_stat->bg_size_a[3] = (lcd_stat->bg_control_a[3] >> 14) & 0x3;

				switch(lcd_stat->bg_size_a[3])
				{
					case 0x0: lcd_stat->text_width_a[3] = 255; lcd_stat->text_height_a[3] = 255; break;
					case 0x1: lcd_stat->text_width_a[3] = 511; lcd_stat->text_height_a[3] = 255; break;
					case 0x2: lcd_stat->text_width_a[3] = 255; lcd_stat->text_height_a[3] = 511; break;
					case 0x3: lcd_stat->text_width_a[3] = 511; lcd_stat->text_height_a[3] = 511; break;
				}
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

				u32 char_block = ((lcd_stat->bg_control_b[3] >> 2) & 0xF);
				u32 screen_block = ((lcd_stat->bg_control_b[3] >> 8) & 0x1F);

				lcd_stat->bg_base_tile_addr_b[3] = (char_block * 0x4000);
				lcd_stat->bg_base_map_addr_b[3] = (screen_block * 0x800);
				lcd_stat->bg_bitmap_base_addr_b[1] = 0x6200000 + (screen_block * 0x4000);

				//Bit-depth
				lcd_stat->bg_depth_b[3] = (lcd_stat->bg_control_b[3] & 0x80) ? 1 : 0;

				//Affine overflow
				lcd_stat->bg_affine_b[1].overflow = (lcd_stat->bg_control_b[3] & 0x2000) ? true : false;

				//Screen size
				lcd_stat->bg_size_b[3] = (lcd_stat->bg_control_b[3] >> 14) & 0x3;

				switch(lcd_stat->bg_size_b[3])
				{
					case 0x0: lcd_stat->text_width_b[3] = 255; lcd_stat->text_height_b[3] = 255; break;
					case 0x1: lcd_stat->text_width_b[3] = 511; lcd_stat->text_height_b[3] = 255; break;
					case 0x2: lcd_stat->text_width_b[3] = 255; lcd_stat->text_height_b[3] = 511; break;
					case 0x3: lcd_stat->text_width_b[3] = 511; lcd_stat->text_height_b[3] = 511; break;
				}
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

		//BG2 Scale/Rotation Parameter A - Engine A
		case NDS_BG2PA_A:
		case NDS_BG2PA_A+1:
			memory_map[address] = value;
			
			{
				u16 raw_value = ((memory_map[NDS_BG2PA_A+1] << 8) | memory_map[NDS_BG2PA_A]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_a[0].dx = -1.0 * p;
				}

				else { lcd_stat->bg_affine_a[0].dx = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_a[0].dx += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG2 Scale/Rotation Parameter B - Engine A
		case NDS_BG2PB_A:
		case NDS_BG2PB_A+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG2PB_A+1] << 8) | memory_map[NDS_BG2PB_A]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_a[0].dmx = -1.0 * p;
				}

				else { lcd_stat->bg_affine_a[0].dmx = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_a[0].dmx += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG2 Scale/Rotation Parameter C - Engine A
		case NDS_BG2PC_A:
		case NDS_BG2PC_A+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG2PC_A+1] << 8) | memory_map[NDS_BG2PC_A]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_a[0].dy = -1.0 * p;
				}

				else { lcd_stat->bg_affine_a[0].dy = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_a[0].dy += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG2 Scale/Rotation Parameter D - Engine A
		case NDS_BG2PD_A:
		case NDS_BG2PD_A+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG2PD_A+1] << 8) | memory_map[NDS_BG2PD_A]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_a[0].dmy = -1.0 * p;
				}

				else { lcd_stat->bg_affine_a[0].dmy = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_a[0].dmy += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG2 Scale/Rotation X Reference - Engine A
		case NDS_BG2X_A:
		case NDS_BG2X_A+1:
		case NDS_BG2X_A+2:
		case NDS_BG2X_A+3:
			memory_map[address] = value;

			{
				u32 x_raw = ((memory_map[NDS_BG2X_A+3] << 24) | (memory_map[NDS_BG2X_A+2] << 16) | (memory_map[NDS_BG2X_A+1] << 8) | (memory_map[NDS_BG2X_A]));

				//Note: The reference points are 19-bit signed 2's complement
				if(x_raw & 0x8000000) 
				{ 
					u32 x = ((x_raw >> 8) - 1);
					x = (~x & 0x7FFFF);
					lcd_stat->bg_affine_a[0].x_ref = -1.0 * x;
				}
				else { lcd_stat->bg_affine_a[0].x_ref = (x_raw >> 8) & 0x7FFFF; }
				if((x_raw & 0xFF) != 0) { lcd_stat->bg_affine_a[0].x_ref += (x_raw & 0xFF) / 256.0; }

				//Set current X position as the new reference point
				lcd_stat->bg_affine_a[0].x_pos = lcd_stat->bg_affine_a[0].x_ref;
			}

			break;

		//BG2 Scale/Rotation Y Reference - Engine A
		case NDS_BG2Y_A:
		case NDS_BG2Y_A+1:
		case NDS_BG2Y_A+2:
		case NDS_BG2Y_A+3:
			memory_map[address] = value;

			{
				u32 y_raw = ((memory_map[NDS_BG2Y_A+3] << 24) | (memory_map[NDS_BG2Y_A+2] << 16) | (memory_map[NDS_BG2Y_A+1] << 8) | (memory_map[NDS_BG2Y_A]));

				//Note: The reference points are 19-bit signed 2's complement
				if(y_raw & 0x8000000) 
				{ 
					u32 y = ((y_raw >> 8) - 1);
					y = (~y & 0x7FFFF);
					lcd_stat->bg_affine_a[0].y_ref = -1.0 * y;
				}
				else { lcd_stat->bg_affine_a[0].y_ref = (y_raw >> 8) & 0x7FFFF; }
				if((y_raw & 0xFF) != 0) { lcd_stat->bg_affine_a[0].y_ref += (y_raw & 0xFF) / 256.0; }

				//Set current Y position as the new reference point
				lcd_stat->bg_affine_a[0].y_pos = lcd_stat->bg_affine_a[0].y_ref;
			}

			break;

		//BG3 Scale/Rotation Parameter A - Engine A
		case NDS_BG3PA_A:
		case NDS_BG3PA_A+1:
			memory_map[address] = value;
			{
				u16 raw_value = ((memory_map[NDS_BG3PA_A+1] << 8) | memory_map[NDS_BG3PA_A]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_a[1].dx = -1.0 * p;
				}
				else { lcd_stat->bg_affine_a[1].dx = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_a[1].dx += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG3 Scale/Rotation Parameter B - Engine A
		case NDS_BG3PB_A:
		case NDS_BG3PB_A+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG3PB_A+1] << 8) | memory_map[NDS_BG3PB_A]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_a[1].dmx = -1.0 * p;
				}
				else { lcd_stat->bg_affine_a[1].dmx = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_a[1].dmx += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG3 Scale/Rotation Parameter C - Engine A
		case NDS_BG3PC_A:
		case NDS_BG3PC_A+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG3PC_A+1] << 8) | memory_map[NDS_BG3PC_A]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_a[1].dy = -1.0 * p;
				}

				else { lcd_stat->bg_affine_a[1].dy = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_a[1].dy += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG3 Scale/Rotation Parameter D - Engine A
		case NDS_BG3PD_A:
		case NDS_BG3PD_A+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG3PD_A+1] << 8) | memory_map[NDS_BG3PD_A]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_a[1].dmy = -1.0 * p;
				}

				else { lcd_stat->bg_affine_a[1].dmy = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_a[1].dmy += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG3 Scale/Rotation X Reference - Engine A
		case NDS_BG3X_A:
		case NDS_BG3X_A+1:
		case NDS_BG3X_A+2:
		case NDS_BG3X_A+3:
			memory_map[address] = value;

			{
				u32 x_raw = ((memory_map[NDS_BG3X_A+3] << 24) | (memory_map[NDS_BG3X_A+2] << 16) | (memory_map[NDS_BG3X_A+1] << 8) | (memory_map[NDS_BG3X_A]));

				//Note: The reference points are 19-bit signed 2's complement
				if(x_raw & 0x8000000) 
				{ 
					u32 x = ((x_raw >> 8) - 1);
					x = (~x & 0x7FFFF);
					lcd_stat->bg_affine_a[1].x_ref = -1.0 * x;
				}
				else { lcd_stat->bg_affine_a[1].x_ref = (x_raw >> 8) & 0x7FFFF; }
				if((x_raw & 0xFF) != 0) { lcd_stat->bg_affine_a[1].x_ref += (x_raw & 0xFF) / 256.0; }

				//Set current X position as the new reference point
				lcd_stat->bg_affine_a[1].x_pos = lcd_stat->bg_affine_a[1].x_ref;
			}

			break;

		//BG3 Scale/Rotation Y Reference - Engine A
		case NDS_BG3Y_A:
		case NDS_BG3Y_A+1:
		case NDS_BG3Y_A+2:
		case NDS_BG3Y_A+3:
			memory_map[address] = value;

			{
				u32 y_raw = ((memory_map[NDS_BG3Y_A+3] << 24) | (memory_map[NDS_BG3Y_A+2] << 16) | (memory_map[NDS_BG3Y_A+1] << 8) | (memory_map[NDS_BG3Y_A]));

				//Note: The reference points are 19-bit signed 2's complement
				if(y_raw & 0x8000000) 
				{ 
					u32 y = ((y_raw >> 8) - 1);
					y = (~y & 0x7FFFF);
					lcd_stat->bg_affine_a[1].y_ref = -1.0 * y;
				}
				else { lcd_stat->bg_affine_a[1].y_ref = (y_raw >> 8) & 0x7FFFF; }
				if((y_raw & 0xFF) != 0) { lcd_stat->bg_affine_a[1].y_ref += (y_raw & 0xFF) / 256.0; }

				//Set current Y position as the new reference point
				lcd_stat->bg_affine_a[1].y_pos = lcd_stat->bg_affine_a[1].y_ref;
			}

			break;

		//BG2 Scale/Rotation Parameter A - Engine B
		case NDS_BG2PA_B:
		case NDS_BG2PA_B+1:
			memory_map[address] = value;
			
			{
				u16 raw_value = ((memory_map[NDS_BG2PA_B+1] << 8) | memory_map[NDS_BG2PA_B]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_b[0].dx = -1.0 * p;
				}

				else { lcd_stat->bg_affine_b[0].dx = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_b[0].dx += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG2 Scale/Rotation Parameter B - Engine B
		case NDS_BG2PB_B:
		case NDS_BG2PB_B+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG2PB_B+1] << 8) | memory_map[NDS_BG2PB_B]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_b[0].dmx = -1.0 * p;
				}

				else { lcd_stat->bg_affine_b[0].dmx = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_b[0].dmx += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG2 Scale/Rotation Parameter C - Engine B
		case NDS_BG2PC_B:
		case NDS_BG2PC_B+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG2PC_B+1] << 8) | memory_map[NDS_BG2PC_B]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_b[0].dy = -1.0 * p;
				}

				else { lcd_stat->bg_affine_b[0].dy = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_b[0].dy += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG2 Scale/Rotation Parameter D - Engine B
		case NDS_BG2PD_B:
		case NDS_BG2PD_B+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG2PD_B+1] << 8) | memory_map[NDS_BG2PD_B]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_b[0].dmy = -1.0 * p;
				}

				else { lcd_stat->bg_affine_b[0].dmy = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_b[0].dmy += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG2 Scale/Rotation X Reference - Engine B
		case NDS_BG2X_B:
		case NDS_BG2X_B+1:
		case NDS_BG2X_B+2:
		case NDS_BG2X_B+3:
			memory_map[address] = value;

			{
				u32 x_raw = ((memory_map[NDS_BG2X_B+3] << 24) | (memory_map[NDS_BG2X_B+2] << 16) | (memory_map[NDS_BG2X_B+1] << 8) | (memory_map[NDS_BG2X_B]));

				//Note: The reference points are 19-bit signed 2's complement
				if(x_raw & 0x8000000) 
				{ 
					u32 x = ((x_raw >> 8) - 1);
					x = (~x & 0x7FFFF);
					lcd_stat->bg_affine_b[0].x_ref = -1.0 * x;
				}
				else { lcd_stat->bg_affine_b[0].x_ref = (x_raw >> 8) & 0x7FFFF; }
				if((x_raw & 0xFF) != 0) { lcd_stat->bg_affine_b[0].x_ref += (x_raw & 0xFF) / 256.0; }

				//Set current X position as the new reference point
				lcd_stat->bg_affine_b[0].x_pos = lcd_stat->bg_affine_b[0].x_ref;
			}

			break;

		//BG2 Scale/Rotation Y Reference - Engine B
		case NDS_BG2Y_B:
		case NDS_BG2Y_B+1:
		case NDS_BG2Y_B+2:
		case NDS_BG2Y_B+3:
			memory_map[address] = value;

			{
				u32 y_raw = ((memory_map[NDS_BG2Y_B+3] << 24) | (memory_map[NDS_BG2Y_B+2] << 16) | (memory_map[NDS_BG2Y_B+1] << 8) | (memory_map[NDS_BG2Y_B]));

				//Note: The reference points are 19-bit signed 2's complement
				if(y_raw & 0x8000000) 
				{ 
					u32 y = ((y_raw >> 8) - 1);
					y = (~y & 0x7FFFF);
					lcd_stat->bg_affine_b[0].y_ref = -1.0 * y;
				}
				else { lcd_stat->bg_affine_b[0].y_ref = (y_raw >> 8) & 0x7FFFF; }
				if((y_raw & 0xFF) != 0) { lcd_stat->bg_affine_b[0].y_ref += (y_raw & 0xFF) / 256.0; }

				//Set current Y position as the new reference point
				lcd_stat->bg_affine_b[0].y_pos = lcd_stat->bg_affine_b[0].y_ref;
			}

			break;

		//BG3 Scale/Rotation Parameter A - Engine B
		case NDS_BG3PA_B:
		case NDS_BG3PA_B+1:
			memory_map[address] = value;
			{
				u16 raw_value = ((memory_map[NDS_BG3PA_B+1] << 8) | memory_map[NDS_BG3PA_B]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_b[1].dx = -1.0 * p;
				}
				else { lcd_stat->bg_affine_b[1].dx = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_b[1].dx += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG3 Scale/Rotation Parameter B - Engine B
		case NDS_BG3PB_B:
		case NDS_BG3PB_B+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG3PB_B+1] << 8) | memory_map[NDS_BG3PB_B]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_b[1].dmx = -1.0 * p;
				}
				else { lcd_stat->bg_affine_b[1].dmx = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_b[1].dmx += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG3 Scale/Rotation Parameter C - Engine B
		case NDS_BG3PC_B:
		case NDS_BG3PC_B+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG3PC_B+1] << 8) | memory_map[NDS_BG3PC_B]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_b[1].dy = -1.0 * p;
				}

				else { lcd_stat->bg_affine_b[1].dy = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_b[1].dy += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG3 Scale/Rotation Parameter D - Engine B
		case NDS_BG3PD_B:
		case NDS_BG3PD_B+1:
			memory_map[address] = value;

			{
				u16 raw_value = ((memory_map[NDS_BG3PD_B+1] << 8) | memory_map[NDS_BG3PD_B]);
				
				//Note: The reference points are 8-bit signed 2's complement
				if(raw_value & 0x8000) 
				{ 
					u16 p = ((raw_value >> 8) - 1);
					p = (~p & 0xFF);
					lcd_stat->bg_affine_b[1].dmy = -1.0 * p;
				}

				else { lcd_stat->bg_affine_b[1].dmy = (raw_value >> 8); }
				if((raw_value & 0xFF) != 0) { lcd_stat->bg_affine_b[1].dmy += (raw_value & 0xFF) / 256.0; }
			}

			break;

		//BG3 Scale/Rotation X Reference - Engine B
		case NDS_BG3X_B:
		case NDS_BG3X_B+1:
		case NDS_BG3X_B+2:
		case NDS_BG3X_B+3:
			memory_map[address] = value;

			{
				u32 x_raw = ((memory_map[NDS_BG3X_B+3] << 24) | (memory_map[NDS_BG3X_B+2] << 16) | (memory_map[NDS_BG3X_B+1] << 8) | (memory_map[NDS_BG3X_B]));

				//Note: The reference points are 19-bit signed 2's complement
				if(x_raw & 0x8000000) 
				{ 
					u32 x = ((x_raw >> 8) - 1);
					x = (~x & 0x7FFFF);
					lcd_stat->bg_affine_b[1].x_ref = -1.0 * x;
				}
				else { lcd_stat->bg_affine_b[1].x_ref = (x_raw >> 8) & 0x7FFFF; }
				if((x_raw & 0xFF) != 0) { lcd_stat->bg_affine_b[1].x_ref += (x_raw & 0xFF) / 256.0; }

				//Set current X position as the new reference point
				lcd_stat->bg_affine_b[1].x_pos = lcd_stat->bg_affine_b[1].x_ref;
			}

			break;

		//BG3 Scale/Rotation Y Reference - Engine B
		case NDS_BG3Y_B:
		case NDS_BG3Y_B+1:
		case NDS_BG3Y_B+2:
		case NDS_BG3Y_B+3:
			memory_map[address] = value;

			{
				u32 y_raw = ((memory_map[NDS_BG3Y_B+3] << 24) | (memory_map[NDS_BG3Y_B+2] << 16) | (memory_map[NDS_BG3Y_B+1] << 8) | (memory_map[NDS_BG3Y_B]));

				//Note: The reference points are 19-bit signed 2's complement
				if(y_raw & 0x8000000) 
				{ 
					u32 y = ((y_raw >> 8) - 1);
					y = (~y & 0x7FFFF);
					lcd_stat->bg_affine_b[1].y_ref = -1.0 * y;
				}
				else { lcd_stat->bg_affine_b[1].y_ref = (y_raw >> 8) & 0x7FFFF; }
				if((y_raw & 0xFF) != 0) { lcd_stat->bg_affine_b[1].y_ref += (y_raw & 0xFF) / 256.0; }

				//Set current Y position as the new reference point
				lcd_stat->bg_affine_b[1].y_pos = lcd_stat->bg_affine_b[1].y_ref;
			}

			break;

		//Window 0 Horizontal Coordinates - Engine A
		case NDS_WIN0H_A:
		case NDS_WIN0H_A+1:
			memory_map[address] = value;
			lcd_stat->window_x_a[0][1] = memory_map[NDS_WIN0H_A+1];
			lcd_stat->window_x_a[0][0] = memory_map[NDS_WIN0H_A];

			//If the two X coordinates are the same, window should fail to draw
			//Set both to a pixel that the NDS cannot draw so the LCD won't render it
			if(lcd_stat->window_x_a[0][0] == lcd_stat->window_x_a[0][1]) { lcd_stat->window_x_a[0][0] = lcd_stat->window_x_a[0][1] = 256; }
			break;

		//Window 1 Horizontal Coordinates - Engine A
		case NDS_WIN1H_A:
		case NDS_WIN1H_A+1:
			memory_map[address] = value;
			lcd_stat->window_x_a[1][1] = memory_map[NDS_WIN1H_A+1];
			lcd_stat->window_x_a[1][0] = memory_map[NDS_WIN1H_A];

			//If the two X coordinates are the same, window should fail to draw
			//Set both to a pixel that the NDS cannot draw so the LCD won't render it
			if(lcd_stat->window_x_a[1][0] == lcd_stat->window_x_a[1][1]) { lcd_stat->window_x_a[1][0] = lcd_stat->window_x_a[1][1] = 256; }
			break;

		//Window 0 Vertical Coordinates - Engine A
		case NDS_WIN0V_A:
		case NDS_WIN0V_A+1:
			memory_map[address] = value;
			lcd_stat->window_y_a[0][1] = memory_map[NDS_WIN0V_A+1];
			lcd_stat->window_y_a[0][0] = memory_map[NDS_WIN0V_A];

			//If the two Y coordinates are the same, window should fail to draw
			//Set both to a pixel that the NDS cannot draw so the LCD won't render it
			if(lcd_stat->window_y_a[0][0] == lcd_stat->window_y_a[0][1]) { lcd_stat->window_y_a[0][0] = lcd_stat->window_y_a[0][1] = 256; }
			break;

		//Window 1 Vertical Coordinates - Engine A
		case NDS_WIN1V_A:
		case NDS_WIN1V_A+1:
			memory_map[address] = value;
			lcd_stat->window_y_a[1][1] = memory_map[NDS_WIN1V_A+1];
			lcd_stat->window_y_a[1][0] = memory_map[NDS_WIN1V_A];

			//If the two X coordinates are the same, window should fail to draw
			//Set both to a pixel that the NDS cannot draw so the LCD won't render it
			if(lcd_stat->window_y_a[1][0] == lcd_stat->window_y_a[1][1]) { lcd_stat->window_y_a[1][0] = lcd_stat->window_y_a[1][1] = 256; }
			break;

		//Window 0 In Enable Flags - Engine A
		case NDS_WININ_A:
			memory_map[address] = (value & 0x3F);
			lcd_stat->window_in_enable_a[0][0] = (value & 0x1) ? true : false;
			lcd_stat->window_in_enable_a[1][0] = (value & 0x2) ? true : false;
			lcd_stat->window_in_enable_a[2][0] = (value & 0x4) ? true : false;
			lcd_stat->window_in_enable_a[3][0] = (value & 0x8) ? true : false;
			lcd_stat->window_in_enable_a[4][0] = (value & 0x10) ? true : false;
			lcd_stat->window_in_enable_a[5][0] = (value & 0x20) ? true : false;
			break;

		//Window 1 In Enable Flags - Engine A
		case NDS_WININ_A+1:
			memory_map[address] = (value & 0x3F);
			lcd_stat->window_in_enable_a[0][1] = (value & 0x1) ? true : false;
			lcd_stat->window_in_enable_a[1][1] = (value & 0x2) ? true : false;
			lcd_stat->window_in_enable_a[2][1] = (value & 0x4) ? true : false;
			lcd_stat->window_in_enable_a[3][1] = (value & 0x8) ? true : false;
			lcd_stat->window_in_enable_a[4][1] = (value & 0x10) ? true : false;
			lcd_stat->window_in_enable_a[5][1] = (value & 0x20) ? true : false;
			break;

		//Window 0 Out Enable Flags - Engine A
		case NDS_WINOUT_A:
			memory_map[address] = (value & 0x3F);
			lcd_stat->window_out_enable_a[0][0] = (value & 0x1) ? true : false;
			lcd_stat->window_out_enable_a[1][0] = (value & 0x2) ? true : false;
			lcd_stat->window_out_enable_a[2][0] = (value & 0x4) ? true : false;
			lcd_stat->window_out_enable_a[3][0] = (value & 0x8) ? true : false;
			lcd_stat->window_out_enable_a[4][0] = (value & 0x10) ? true : false;
			lcd_stat->window_out_enable_a[5][0] = (value & 0x20) ? true : false;
			break;

		//Window 1 Out Enable Flags - Engine A
		case NDS_WINOUT_A+1:
			memory_map[address] = (value & 0x3F);
			lcd_stat->window_out_enable_a[0][1] = (value & 0x1) ? true : false;
			lcd_stat->window_out_enable_a[1][1] = (value & 0x2) ? true : false;
			lcd_stat->window_out_enable_a[2][1] = (value & 0x4) ? true : false;
			lcd_stat->window_out_enable_a[3][1] = (value & 0x8) ? true : false;
			lcd_stat->window_out_enable_a[4][1] = (value & 0x10) ? true : false;
			lcd_stat->window_out_enable_a[5][1] = (value & 0x20) ? true : false;
			break;

		//Window 0 Horizontal Coordinates - Engine B
		case NDS_WIN0H_B:
		case NDS_WIN0H_B+1:
			memory_map[address] = value;
			lcd_stat->window_x_b[0][1] = memory_map[NDS_WIN0H_B+1];
			lcd_stat->window_x_b[0][0] = memory_map[NDS_WIN0H_B];

			//If the two X coordinates are the same, window should fail to draw
			//Set both to a pixel that the NDS cannot draw so the LCD won't render it
			if(lcd_stat->window_x_b[0][0] == lcd_stat->window_x_b[0][1]) { lcd_stat->window_x_b[0][0] = lcd_stat->window_x_b[0][1] = 256; }
			break;

		//Window 1 Horizontal Coordinates - Engine B
		case NDS_WIN1H_B:
		case NDS_WIN1H_B+1:
			memory_map[address] = value;
			lcd_stat->window_x_b[1][1] = memory_map[NDS_WIN1H_B+1];
			lcd_stat->window_x_b[1][0] = memory_map[NDS_WIN1H_B];

			//If the two X coordinates are the same, window should fail to draw
			//Set both to a pixel that the NDS cannot draw so the LCD won't render it
			if(lcd_stat->window_x_b[1][0] == lcd_stat->window_x_b[1][1]) { lcd_stat->window_x_b[1][0] = lcd_stat->window_x_b[1][1] = 256; }
			break;

		//Window 0 Vertical Coordinates - Engine B
		case NDS_WIN0V_B:
		case NDS_WIN0V_B+1:
			memory_map[address] = value;
			lcd_stat->window_y_b[0][1] = memory_map[NDS_WIN0V_B+1];
			lcd_stat->window_y_b[0][0] = memory_map[NDS_WIN0V_B];

			//If the two Y coordinates are the same, window should fail to draw
			//Set both to a pixel that the NDS cannot draw so the LCD won't render it
			if(lcd_stat->window_y_b[0][0] == lcd_stat->window_y_b[0][1]) { lcd_stat->window_y_b[0][0] = lcd_stat->window_y_b[0][1] = 256; }
			break;

		//Window 1 Vertical Coordinates - Engine B
		case NDS_WIN1V_B:
		case NDS_WIN1V_B+1:
			memory_map[address] = value;
			lcd_stat->window_y_b[1][1] = memory_map[NDS_WIN1V_B+1];
			lcd_stat->window_y_b[1][0] = memory_map[NDS_WIN1V_B];

			//If the two X coordinates are the same, window should fail to draw
			//Set both to a pixel that the NDS cannot draw so the LCD won't render it
			if(lcd_stat->window_y_b[1][0] == lcd_stat->window_y_b[1][1]) { lcd_stat->window_y_b[1][0] = lcd_stat->window_y_b[1][1] = 256; }
			break;

		//Window 0 In Enable Flags - Engine B
		case NDS_WININ_B:
			memory_map[address] = (value & 0x3F);
			lcd_stat->window_in_enable_b[0][0] = (value & 0x1) ? true : false;
			lcd_stat->window_in_enable_b[1][0] = (value & 0x2) ? true : false;
			lcd_stat->window_in_enable_b[2][0] = (value & 0x4) ? true : false;
			lcd_stat->window_in_enable_b[3][0] = (value & 0x8) ? true : false;
			lcd_stat->window_in_enable_b[4][0] = (value & 0x10) ? true : false;
			lcd_stat->window_in_enable_b[5][0] = (value & 0x20) ? true : false;
			break;

		//Window 1 In Enable Flags - Engine B
		case NDS_WININ_B+1:
			memory_map[address] = (value & 0x3F);
			lcd_stat->window_in_enable_b[0][1] = (value & 0x1) ? true : false;
			lcd_stat->window_in_enable_b[1][1] = (value & 0x2) ? true : false;
			lcd_stat->window_in_enable_b[2][1] = (value & 0x4) ? true : false;
			lcd_stat->window_in_enable_b[3][1] = (value & 0x8) ? true : false;
			lcd_stat->window_in_enable_b[4][1] = (value & 0x10) ? true : false;
			lcd_stat->window_in_enable_b[5][1] = (value & 0x20) ? true : false;
			break;

		//Window 0 Out Enable Flags - Engine B
		case NDS_WINOUT_B:
			memory_map[address] = (value & 0x3F);
			lcd_stat->window_out_enable_b[0][0] = (value & 0x1) ? true : false;
			lcd_stat->window_out_enable_b[1][0] = (value & 0x2) ? true : false;
			lcd_stat->window_out_enable_b[2][0] = (value & 0x4) ? true : false;
			lcd_stat->window_out_enable_b[3][0] = (value & 0x8) ? true : false;
			lcd_stat->window_out_enable_b[4][0] = (value & 0x10) ? true : false;
			lcd_stat->window_out_enable_b[5][0] = (value & 0x20) ? true : false;
			break;

		//Window 1 Out Enable Flags - Engine B
		case NDS_WINOUT_B+1:
			memory_map[address] = (value & 0x3F);
			lcd_stat->window_out_enable_b[0][1] = (value & 0x1) ? true : false;
			lcd_stat->window_out_enable_b[1][1] = (value & 0x2) ? true : false;
			lcd_stat->window_out_enable_b[2][1] = (value & 0x4) ? true : false;
			lcd_stat->window_out_enable_b[3][1] = (value & 0x8) ? true : false;
			lcd_stat->window_out_enable_b[4][1] = (value & 0x10) ? true : false;
			lcd_stat->window_out_enable_b[5][1] = (value & 0x20) ? true : false;
			break;

		//SFX Control - Engine A
		case NDS_BLDCNT_A:
			memory_map[address] = value;
			lcd_stat->sfx_target_a[0][0] = (value & 0x1) ? true : false;
			lcd_stat->sfx_target_a[1][0] = (value & 0x2) ? true : false;
			lcd_stat->sfx_target_a[2][0] = (value & 0x4) ? true : false;
			lcd_stat->sfx_target_a[3][0] = (value & 0x8) ? true : false;
			lcd_stat->sfx_target_a[4][0] = (value & 0x10) ? true : false;
			lcd_stat->sfx_target_a[5][0] = (value & 0x20) ? true : false;

			switch(value >> 6)
			{
				case 0x0: lcd_stat->current_sfx_type_a = NDS_NORMAL; break;
				case 0x1: lcd_stat->current_sfx_type_a = NDS_ALPHA_BLEND; break;
				case 0x2: lcd_stat->current_sfx_type_a = NDS_BRIGHTNESS_UP; break;
				case 0x3: lcd_stat->current_sfx_type_a = NDS_BRIGHTNESS_DOWN; break;
			}			

			break;

		case NDS_BLDCNT_A+1:
			memory_map[address] = (value & 0x3F);
			lcd_stat->sfx_target_a[0][1] = (value & 0x1) ? true : false;
			lcd_stat->sfx_target_a[1][1] = (value & 0x2) ? true : false;
			lcd_stat->sfx_target_a[2][1] = (value & 0x4) ? true : false;
			lcd_stat->sfx_target_a[3][1] = (value & 0x8) ? true : false;
			lcd_stat->sfx_target_a[4][1] = (value & 0x10) ? true : false;
			lcd_stat->sfx_target_a[5][1] = (value & 0x20) ? true : false;
			break;

		//SFX Alpha Control - Engine A
		case NDS_BLDALPHA_A:
			memory_map[address] = value;

			if(value & 0x10) { lcd_stat->alpha_coef_a[0] = 1.0; }
			else { lcd_stat->alpha_coef_a[0] = (value & 0xF) / 16.0; }
			
			break;

		case NDS_BLDALPHA_A+1:
			memory_map[address] = value;

			if(value & 0x10) { lcd_stat->alpha_coef_a[1] = 1.0; }
			else { lcd_stat->alpha_coef_a[1] = (value & 0xF) / 16.0; }

			break;

		//SFX Brightness Control - Engine A
		case NDS_BLDY_A:
			if(memory_map[address] == value) { return ; }

			memory_map[address] = value;
			if(value & 0x10) { lcd_stat->brightness_coef_a = 1.0; }
			else { lcd_stat->brightness_coef_a = (value & 0xF) / 16.0; }

			break;

		//SFX Control - Engine A
		case NDS_BLDCNT_B:
			memory_map[address] = value;
			lcd_stat->sfx_target_b[0][0] = (value & 0x1) ? true : false;
			lcd_stat->sfx_target_b[1][0] = (value & 0x2) ? true : false;
			lcd_stat->sfx_target_b[2][0] = (value & 0x4) ? true : false;
			lcd_stat->sfx_target_b[3][0] = (value & 0x8) ? true : false;
			lcd_stat->sfx_target_b[4][0] = (value & 0x10) ? true : false;
			lcd_stat->sfx_target_b[5][0] = (value & 0x20) ? true : false;

			switch(value >> 6)
			{
				case 0x0: lcd_stat->current_sfx_type_b = NDS_NORMAL; break;
				case 0x1: lcd_stat->current_sfx_type_b = NDS_ALPHA_BLEND; break;
				case 0x2: lcd_stat->current_sfx_type_b = NDS_BRIGHTNESS_UP; break;
				case 0x3: lcd_stat->current_sfx_type_b = NDS_BRIGHTNESS_DOWN; break;
			}			

			break;

		case NDS_BLDCNT_B+1:
			memory_map[address] = (value & 0x3F);
			lcd_stat->sfx_target_b[0][1] = (value & 0x1) ? true : false;
			lcd_stat->sfx_target_b[1][1] = (value & 0x2) ? true : false;
			lcd_stat->sfx_target_b[2][1] = (value & 0x4) ? true : false;
			lcd_stat->sfx_target_b[3][1] = (value & 0x8) ? true : false;
			lcd_stat->sfx_target_b[4][1] = (value & 0x10) ? true : false;
			lcd_stat->sfx_target_b[5][1] = (value & 0x20) ? true : false;
			break;

		//SFX Alpha Control - Engine A
		case NDS_BLDALPHA_B:
			memory_map[address] = value;

			if(value & 0x10) { lcd_stat->alpha_coef_b[0] = 1.0; }
			else { lcd_stat->alpha_coef_b[0] = (value & 0xF) / 16.0; }

			break;

		case NDS_BLDALPHA_B+1:
			memory_map[address] = value;

			if(value & 0x10) { lcd_stat->alpha_coef_b[1] = 1.0; }
			else { lcd_stat->alpha_coef_b[1] = (value & 0xF) / 16.0; }

			break;

		//SFX Brightness Control - Engine A
		case NDS_BLDY_B:
			if(memory_map[address] == value) { return ; }

			memory_map[address] = value;
			if(value & 0x10) { lcd_stat->brightness_coef_b = 1.0; }
			else { lcd_stat->brightness_coef_b = (value & 0xF) / 16.0; }

			break;

		//WRAM Control
		case NDS_WRAMCNT:
			if(access_mode)
			{
				memory_map[address] = (value & 0x3);
				wram_mode = (value & 0x3);
			}

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

				//Set enable flag
				if(value & 0x80)
				{
					lcd_stat->vram_bank_enable[bank_id] = true;

					//Generate new OBJ Extended Palettes if necessary
					if((bank_id == 5) || (bank_id == 6))
					{
						lcd_stat->obj_ext_pal_update_a = true;
						lcd_stat->obj_ext_pal_update_list_a.resize(0x1000, true);
					}

					else if(bank_id == 8)
					{
						lcd_stat->obj_ext_pal_update_b = true;
						lcd_stat->obj_ext_pal_update_list_b.resize(0x1000, true);
					}
				}

				else { lcd_stat->vram_bank_enable[bank_id] = false; }

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

							case 0x7:
								pal_b_bg_slot[0] = lcd_stat->vram_bank_addr[7];
								pal_b_bg_slot[1] = lcd_stat->vram_bank_addr[7] + 0x2000;
								pal_b_bg_slot[2] = lcd_stat->vram_bank_addr[7] + 0x4000;
								pal_b_bg_slot[3] = lcd_stat->vram_bank_addr[7] + 0x6000;

							case 0x8:
								lcd_stat->vram_bank_addr[8] = 0x6600000;
								break;
						}

						break;

					//MST 3
					case 0x3:
						switch(bank_id)
						{
							case 0x8:
								pal_b_obj_slot[0] = lcd_stat->vram_bank_addr[7];
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

							case 0x4:
								pal_a_bg_slot[0] = lcd_stat->vram_bank_addr[4];
								pal_a_bg_slot[1] = lcd_stat->vram_bank_addr[4] + 0x2000;
								pal_a_bg_slot[2] = lcd_stat->vram_bank_addr[4] + 0x4000;
								pal_a_bg_slot[3] = lcd_stat->vram_bank_addr[4] + 0x6000;
								break;

							case 0x5:
							case 0x6:
								if(!offset)
								{
									pal_a_bg_slot[0] = lcd_stat->vram_bank_addr[bank_id];
									pal_a_bg_slot[1] = lcd_stat->vram_bank_addr[bank_id] + 0x2000;
								}

								else
								{
									pal_a_bg_slot[2] = lcd_stat->vram_bank_addr[bank_id];
									pal_a_bg_slot[3] = lcd_stat->vram_bank_addr[bank_id] + 0x2000;
								}

								break;
								
						}

						break;

					//MST 5
					case 0x5:
						switch(bank_id)
						{
							case 0x5:
								pal_a_obj_slot[0] = lcd_stat->vram_bank_addr[5];
								break;

							case 0x6:
								pal_a_obj_slot[0] = lcd_stat->vram_bank_addr[6];
								break;
						}

						break;
				}
			}

			break;

		//3D Display Control
		case NDS_DISP3DCNT:
		case NDS_DISP3DCNT+1:
		case NDS_DISP3DCNT+2:
		case NDS_DISP3DCNT+3:
			memory_map[address] = value;
			lcd_3D_stat->display_control = ((memory_map[NDS_DISP3DCNT+3] << 24) | (memory_map[NDS_DISP3DCNT+2] << 16) | (memory_map[NDS_DISP3DCNT+1] << 8) | memory_map[NDS_DISP3DCNT]);

			break;

		//Master Brightness
		case NDS_MASTER_BRIGHT:
		case NDS_MASTER_BRIGHT+1:
			memory_map[address] = value;
			lcd_stat->master_bright = ((memory_map[NDS_MASTER_BRIGHT+1] << 8) | memory_map[NDS_MASTER_BRIGHT]);

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
			//Write to NDS9 IF
			if(access_mode) { nds9_if &= ~(value << ((address & 0x3) << 3)); }

			//Write to NDS7 IF
			else { nds7_if &= ~(value << ((address & 0x3) << 3)); }

			break;

		//Timer 0 Reload Value
		case NDS_TM0CNT_L:
		case NDS_TM0CNT_L+1:
			if(access_mode)
			{
				nds9_timer->at(0).reload_value &= (address & 0x1) ? 0xFF : 0xFF00;
				nds9_timer->at(0).reload_value |= (address & 0x1) ? (value << 8) : value;
			}

			else
			{
				nds7_timer->at(0).reload_value &= (address & 0x1) ? 0xFF : 0xFF00;
				nds7_timer->at(0).reload_value |= (address & 0x1) ? (value << 8) : value;
			}

			break;

		//Timer 1 Reload Value
		case NDS_TM1CNT_L:
		case NDS_TM1CNT_L+1:
			if(access_mode)
			{
				nds9_timer->at(1).reload_value &= (address & 0x1) ? 0xFF : 0xFF00;
				nds9_timer->at(1).reload_value |= (address & 0x1) ? (value << 8) : value;
			}

			else
			{
				nds7_timer->at(1).reload_value &= (address & 0x1) ? 0xFF : 0xFF00;
				nds7_timer->at(1).reload_value |= (address & 0x1) ? (value << 8) : value;
			}

			break;

		//Timer 2 Reload Value
		case NDS_TM2CNT_L:
		case NDS_TM2CNT_L+1:
			if(access_mode)
			{
				nds9_timer->at(2).reload_value &= (address & 0x1) ? 0xFF : 0xFF00;
				nds9_timer->at(2).reload_value |= (address & 0x1) ? (value << 8) : value;
			}

			else
			{
				nds7_timer->at(2).reload_value &= (address & 0x1) ? 0xFF : 0xFF00;
				nds7_timer->at(2).reload_value |= (address & 0x1) ? (value << 8) : value;
			}

			break;

		//Timer 3 Reload Value
		case NDS_TM3CNT_L:
		case NDS_TM3CNT_L+1:
			if(access_mode)
			{
				nds9_timer->at(3).reload_value &= (address & 0x1) ? 0xFF : 0xFF00;
				nds9_timer->at(3).reload_value |= (address & 0x1) ? (value << 8) : value;
			}

			else
			{
				nds7_timer->at(3).reload_value &= (address & 0x1) ? 0xFF : 0xFF00;
				nds7_timer->at(3).reload_value |= (address & 0x1) ? (value << 8) : value;
			}

			break;

		//Timer 0 Control
		case NDS_TM0CNT_H:
		case NDS_TM0CNT_H+1:
			{
				//Grab pointer to NDS9 or NDS7 Timer 0
				nds_timer* timer = (access_mode) ? &nds9_timer->at(0) : &nds7_timer->at(0);

				bool prev_enable = (timer->cnt & 0x80) ?  true : false;
				timer->cnt &= (address & 0x1) ? 0xFF : 0xFF00;
				timer->cnt |= (address & 0x1) ? (value << 8) : value;

				timer->enable = (timer->cnt & 0x80) ?  true : false;
				timer->interrupt = (timer->cnt & 0x40) ? true : false;
				if(timer->enable && !prev_enable) { timer->counter = timer->reload_value; }

				switch(timer->cnt & 0x3)
				{
					case 0x0: timer->prescalar = 1; break;
					case 0x1: timer->prescalar = 64; break;
					case 0x2: timer->prescalar = 256; break;
					case 0x3: timer->prescalar = 1024; break;
				}

				if(timer->count_up) { timer->prescalar = 1; }

				timer->clock = (timer->prescalar - 1);
			}

			break;

		//Timer 1 Control
		case NDS_TM1CNT_H:
		case NDS_TM1CNT_H+1:
			{
				//Grab pointer to NDS9 or NDS7 Timer 1
				nds_timer* timer = (access_mode) ? &nds9_timer->at(1) : &nds7_timer->at(1);

				bool prev_enable = (timer->cnt & 0x80) ?  true : false;
				timer->cnt &= (address & 0x1) ? 0xFF : 0xFF00;
				timer->cnt |= (address & 0x1) ? (value << 8) : value;

				timer->count_up = (timer->cnt & 0x4) ? true : false;
				timer->enable = (timer->cnt & 0x80) ?  true : false;
				timer->interrupt = (timer->cnt & 0x40) ? true : false;
				if(timer->enable && !prev_enable) { timer->counter = timer->reload_value; }

				switch(timer->cnt & 0x3)
				{
					case 0x0: timer->prescalar = 1; break;
					case 0x1: timer->prescalar = 64; break;
					case 0x2: timer->prescalar = 256; break;
					case 0x3: timer->prescalar = 1024; break;
				}

				if(timer->count_up) { timer->prescalar = 1; }

				timer->clock = timer->prescalar; 
			}

			break;

		//Timer 2 Control
		case NDS_TM2CNT_H:
		case NDS_TM2CNT_H+1:
			{
				//Grab pointer to NDS9 or NDS7 Timer 2
				nds_timer* timer = (access_mode) ? &nds9_timer->at(2) : &nds7_timer->at(2);

				bool prev_enable = (timer->cnt & 0x80) ?  true : false;
				timer->cnt &= (address & 0x1) ? 0xFF : 0xFF00;
				timer->cnt |= (address & 0x1) ? (value << 8) : value;

				timer->count_up = (timer->cnt & 0x4) ? true : false;
				timer->enable = (timer->cnt & 0x80) ?  true : false;
				timer->interrupt = (timer->cnt & 0x40) ? true : false;
				if(timer->enable && !prev_enable) { timer->counter = timer->reload_value; }

				switch(timer->cnt & 0x3)
				{
					case 0x0: timer->prescalar = 1; break;
					case 0x1: timer->prescalar = 64; break;
					case 0x2: timer->prescalar = 256; break;
					case 0x3: timer->prescalar = 1024; break;
				}

				if(timer->count_up) { timer->prescalar = 1; }

				timer->clock = timer->prescalar; 
			}

			break;

		//Timer 3 Control
		case NDS_TM3CNT_H:
		case NDS_TM3CNT_H+1:
			{
				//Grab pointer to NDS9 or NDS7 Timer 3
				nds_timer* timer = (access_mode) ? &nds9_timer->at(3) : &nds7_timer->at(3);

				bool prev_enable = (timer->cnt & 0x80) ?  true : false;
				timer->cnt &= (address & 0x1) ? 0xFF : 0xFF00;
				timer->cnt |= (address & 0x1) ? (value << 8) : value;

				timer->count_up = (timer->cnt & 0x4) ? true : false;
				timer->enable = (timer->cnt & 0x80) ?  true : false;
				timer->interrupt = (timer->cnt & 0x40) ? true : false;
				if(timer->enable && !prev_enable) { timer->counter = timer->reload_value; }

				switch(timer->cnt & 0x3)
				{
					case 0x0: timer->prescalar = 1; break;
					case 0x1: timer->prescalar = 64; break;
					case 0x2: timer->prescalar = 256; break;
					case 0x3: timer->prescalar = 1024; break;
				}

				if(timer->count_up) { timer->prescalar = 1; }

				timer->clock = timer->prescalar; 
			}

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
				u16 irq_trigger = (nds9_ipc.cnt & 0x5);

				nds9_ipc.cnt &= 0xFFF3;
				nds9_ipc.cnt |= (value & 0xC);

				//Raise Send FIFO Empty IRQ when Bit 2 goes from 0 to 1
				if((irq_trigger != 0x5) && ((nds9_ipc.cnt & 0x5) == 0x5)) { nds9_if |= 0x20000; }

				//Clear Send FIFO
				if(nds9_ipc.cnt & 0x8)
				{
					while(!nds9_ipc.fifo.empty()) { nds9_ipc.fifo.pop(); }
					
					//Set SNDFIFO EMPTY Status on NDS9
					nds9_ipc.cnt |= 0x1;

					//Set RECVFIFO EMPTY Status on NDS7
					nds7_ipc.cnt |= 0x100;

					//Raise Send FIFO EMPTY IRQ if necessary
					if(nds9_ipc.cnt & 0x4) { nds9_if |= 0x20000; }

					//Set latest readable RECV value to zero
					nds9_ipc.fifo_latest = 0;

					//Bit 3 is write only, so effectively never set it
					nds9_ipc.cnt &= ~0x8;
				}
			}

			else
			{
				u16 irq_trigger = (nds7_ipc.cnt & 0x5);

				nds7_ipc.cnt &= 0xFFF3;
				nds7_ipc.cnt |= (value & 0xC);

				//Raise Send FIFO Empty IRQ when Bit 2 goes from 0 to 1
				if((irq_trigger != 0x5) && ((nds7_ipc.cnt & 0x5) == 0x5)) { nds7_if |= 0x20000; }

				//Clear Send FIFO
				if(nds7_ipc.cnt & 0x8)
				{
					while(!nds7_ipc.fifo.empty()) { nds7_ipc.fifo.pop(); }
					
					//Set SNDFIFO EMPTY Status on NDS7
					nds7_ipc.cnt |= 0x1;

					//Set RECVFIFO EMPTY Status on NDS9
					nds9_ipc.cnt |= 0x100;

					//Raise Send FIFO EMPTY IRQ if necessary
					if(nds7_ipc.cnt & 0x4) { nds7_if |= 0x20000; }

					//Set latest readable RECV value to zero
					nds7_ipc.fifo_latest = 0;

					//Bit 3 is write only, so effectively never set it
					nds7_ipc.cnt &= ~0x8;
				}
			}

			break;

		case NDS_IPCFIFOCNT+1:
			if(access_mode)
			{
				//Acknowledge IPCFIFO error bit
				if((value & 0x40) && (nds9_ipc.cnt & 0x4000)) { value &= ~0x40; }
				else if((value & 0x40) && ((nds9_ipc.cnt & 0x4000) == 0)) { value &= ~0x40; }

				u16 irq_trigger = (nds9_ipc.cnt & 0x500);

				nds9_ipc.cnt &= 0x3FF;
				nds9_ipc.cnt |= ((value & 0xC4) << 8);

				//Raise Receive FIFO Not Empty IRQ when Bit 10 goes from 0 to 1
				if((irq_trigger != 0x400) && ((nds9_ipc.cnt & 0x500) == 0x400)) { nds9_if |= 0x40000; }
			}

			else
			{
				//Acknowledge IPCFIFO error bit
				if((value & 0x40) && (nds7_ipc.cnt & 0x4000)) { value &= ~0x40; }
				else if((value & 0x40) && ((nds7_ipc.cnt & 0x4000) == 0)) { value &= ~0x40; }
	
				u16 irq_trigger = (nds7_ipc.cnt & 0x500);

				nds7_ipc.cnt &= 0x3FF;
				nds7_ipc.cnt |= ((value & 0xC4) << 8);

				//Raise Receive FIFO Not Empty IRQ when Bit 10 goes from 0 to 1
				if((irq_trigger != 0x400) && ((nds7_ipc.cnt & 0x500) == 0x400)) { nds7_if |= 0x40000; }
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

		case NDS_RTC:
			if(!access_mode)
			{
				memory_map[address] = (value & 0x77);

				nds7_rtc.cnt = (value & 0x77);
				nds7_rtc.io_direction = ((value >> 4) & 0x7);

				//Only change SIO, SCK, and CS when data direction is write
				if(nds7_rtc.io_direction & 0x1)
				{
					if(value & 0x1) { nds7_rtc.data |= 0x1; }
					else { nds7_rtc.data &= ~0x1; }
				}

				if(nds7_rtc.io_direction & 0x2)
				{
					if(value & 0x2) { nds7_rtc.data |= 0x2; }
					else { nds7_rtc.data &= ~0x2; }
				}

				if(nds7_rtc.io_direction & 0x4)
				{
					if(value & 0x4) { nds7_rtc.data |= 0x4; }
					else { nds7_rtc.data &= ~0x4; }
				}

				//Communicate with the RTC
				write_rtc();
			}

			break;

		//Division math via hardware
		//Note, all of the below addresses trigger a calculation
		case NDS_DIVCNT:
		case NDS_DIVCNT+1:
		case NDS_DIVCNT+2:
		case NDS_DIVCNT+3:
		case NDS_DIVNUMER:
		case NDS_DIVNUMER+1:
		case NDS_DIVNUMER+2:
		case NDS_DIVNUMER+3:
		case NDS_DIVNUMER+4:
		case NDS_DIVNUMER+5:
		case NDS_DIVNUMER+6:
		case NDS_DIVNUMER+7:
		case NDS_DIVDENOM:
		case NDS_DIVDENOM+1:
		case NDS_DIVDENOM+2:
		case NDS_DIVDENOM+3:
		case NDS_DIVDENOM+4:
		case NDS_DIVDENOM+5:
		case NDS_DIVDENOM+6:
		case NDS_DIVDENOM+7:
			if(access_mode)
			{
				memory_map[address] = value;

				u8 div_mode = memory_map[NDS_DIVCNT] & 0x3;
				u8 numer_sign = 0;
				u8 denom_sign = 0;

				//Grab numerator and denomenator
				nds9_math.div_numer = read_u32_fast(NDS_DIVNUMER+4);
				nds9_math.div_numer <<= 16;
				nds9_math.div_numer <<= 16;
				nds9_math.div_numer |= read_u32_fast(NDS_DIVNUMER);

				nds9_math.div_denom = read_u32_fast(NDS_DIVDENOM+4);
				nds9_math.div_denom <<= 16;
				nds9_math.div_denom <<= 16;
				nds9_math.div_denom |= read_u32_fast(NDS_DIVDENOM);

				//Set division by zero flag
				if(!nds9_math.div_denom)
				{
					memory_map[NDS_DIVCNT+1] |= 0x40;

					//Remainder = numerator
					write_u64_fast(NDS_DIVREMAIN, nds9_math.div_numer);
					
					u32 result_32 = nds9_math.div_numer;
					u64 result_64 = nds9_math.div_numer;
	
					switch(div_mode)
					{
						//32-bit result is positive or negative 1
						case 0x00:
							if(nds9_math.div_numer & 0x80000000) { result_32 = 0x1; }
							else { result_32 = 0xFFFFFFFF; }
							write_u64_fast(NDS_DIVRESULT, result_32);
							break;

						//64-bit result is positive or negative 1
						case 0x1:
						case 0x2:
							if(nds9_math.div_numer & 0x8000000000000000) { result_64 = 0x1; }
							else { result_64 = 0xFFFFFFFFFFFFFFFF; }
							write_u64_fast(NDS_DIVRESULT, result_64);
							break;
					}

					return;
				}

				else { memory_map[NDS_DIVCNT+1] &= ~0x40; }

				//Mode 0 32-bit - 32-bit
				if(div_mode == 0)
				{
					u32 result = 0;
					u32 remainder = 0;					

					//Determine signs
					numer_sign = (nds9_math.div_numer & 0x80000000) ? 1 : 0;
					denom_sign = (nds9_math.div_denom & 0x80000000) ? 1 : 0;

					//Convert 2s complement if negative
					if(numer_sign)
					{
						nds9_math.div_numer = ~nds9_math.div_numer;
						nds9_math.div_numer++;
						nds9_math.div_numer &= 0xFFFFFFFF;
					}

					if(denom_sign)
					{
						nds9_math.div_denom = ~nds9_math.div_denom;
						nds9_math.div_denom++;
						nds9_math.div_denom &= 0xFFFFFFFF;
					}

					result = nds9_math.div_numer / nds9_math.div_denom;
					remainder = nds9_math.div_numer % nds9_math.div_denom;

					//Convert result and remainder to 2s complement if necessary
					if(numer_sign != denom_sign)
					{
						result--;
						result = ~result;

						remainder--;
						remainder = ~remainder;
					}

					//Write results and remainder
					write_u64_fast(NDS_DIVRESULT, result);
					write_u64_fast(NDS_DIVREMAIN, remainder);
				}

				//Mode 1 64-bit - 32-bit
				else if((div_mode == 1) || (div_mode == 3))
				{
					u64 result = 0;
					u32 remainder = 0;					

					//Determine signs
					numer_sign = (nds9_math.div_numer & 0x8000000000000000) ? 1 : 0;
					denom_sign = (nds9_math.div_denom & 0x80000000) ? 1 : 0;

					//Convert 2s complement if negative
					if(numer_sign)
					{
						nds9_math.div_numer = ~nds9_math.div_numer;
						nds9_math.div_numer++;
					}

					if(denom_sign)
					{
						nds9_math.div_denom = ~nds9_math.div_denom;
						nds9_math.div_denom++;
						nds9_math.div_denom &= 0xFFFFFFFF;
					}

					result = nds9_math.div_numer / nds9_math.div_denom;
					remainder = nds9_math.div_numer % nds9_math.div_denom;

					//Convert result and remainder to 2s complement if necessary
					if(numer_sign != denom_sign)
					{
						result--;
						result = ~result;

						remainder--;
						remainder = ~remainder;
					}

					//Write results and remainder
					write_u64_fast(NDS_DIVRESULT, result);
					write_u64_fast(NDS_DIVREMAIN, remainder);
				}

				//Mode 2 64-bit - 64-bit
				else
				{
					u64 result = 0;
					u64 remainder = 0;					

					//Determine signs
					numer_sign = (nds9_math.div_numer & 0x8000000000000000) ? 1 : 0;
					denom_sign = (nds9_math.div_denom & 0x8000000000000000) ? 1 : 0;

					//Convert 2s complement if negative
					if(numer_sign)
					{
						nds9_math.div_numer = ~nds9_math.div_numer;
						nds9_math.div_numer++;
					}

					if(denom_sign)
					{
						nds9_math.div_denom = ~nds9_math.div_denom;
						nds9_math.div_denom++;
					}

					result = nds9_math.div_numer / nds9_math.div_denom;
					remainder = nds9_math.div_numer % nds9_math.div_denom;

					//Convert result and remainder to 2s complement if necessary
					if(numer_sign != denom_sign)
					{
						result--;
						result = ~result;

						remainder--;
						remainder = ~remainder;
					}

					//Write results and remainder
					write_u64_fast(NDS_DIVRESULT, result);
					write_u64_fast(NDS_DIVREMAIN, remainder);
				}
			}

			break;

		case NDS_SQRTCNT:
		case NDS_SQRTCNT+1:
		case NDS_SQRTCNT+2:
		case NDS_SQRTCNT+3:
		case NDS_SQRTPARAM:
		case NDS_SQRTPARAM+1:
		case NDS_SQRTPARAM+2:
		case NDS_SQRTPARAM+3:
		case NDS_SQRTPARAM+4:
		case NDS_SQRTPARAM+5:
		case NDS_SQRTPARAM+6:
		case NDS_SQRTPARAM+7:
			if(access_mode)
			{
				//TODO - 64-bit SQRT ops via sqrt() will have issues, but only with ridiculously large numbers
				memory_map[address] = value;

				u8 sqrt_mode = memory_map[NDS_SQRTCNT] & 0x1;
				
				//Grab SQRT parameter
				if(sqrt_mode)
				{
					nds9_math.sqrt_param = read_u32_fast(NDS_SQRTPARAM+4);
					nds9_math.sqrt_param <<= 16;
					nds9_math.sqrt_param <<= 16;
					nds9_math.sqrt_param |= read_u32_fast(NDS_SQRTPARAM);
				}

				else { nds9_math.sqrt_param = read_u32_fast(NDS_SQRTPARAM); }

				nds9_math.sqrt_result = sqrt(nds9_math.sqrt_param);
				write_u32_fast(NDS_SQRTRESULT, nds9_math.sqrt_result);
			}

			break;

		case NDS_AUXSPICNT:
			if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
			{
				nds_aux_spi.cnt &= 0xFF80;
				nds_aux_spi.cnt |= (value & 0x43);
			}

			break;

		case NDS_AUXSPICNT+1:
			if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
			{
				nds_aux_spi.cnt &= 0xFF;
				nds_aux_spi.cnt |= ((value & 0xE0) << 8);
			}

			break;

		case NDS_AUXSPIDATA:
			if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
			{
				nds_aux_spi.data = value;

				//Start AUXSPI transfer
				if(nds_aux_spi.cnt & 0xA000)
				{
					process_aux_spi_bus();
				}		
			}

			break;

		case NDS_ROMCNT:
		case NDS_ROMCNT+1:
		case NDS_ROMCNT+2:
		case NDS_ROMCNT+3:
			if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
			{
				memory_map[address] = value;

				u8 transfer_bit = (nds_card.cnt & 0x80000000) ? 1 : 0;
				nds_card.cnt = ((memory_map[NDS_ROMCNT+3] << 24) | (memory_map[NDS_ROMCNT+2] << 16) | (memory_map[NDS_ROMCNT+1] << 8) | memory_map[NDS_ROMCNT]);

				//Start cartridge transfer if Bit 31 goes from low to high
				if((transfer_bit == 0) && (nds_card.cnt & 0x80000000))
				{
					//Transfer rate is 6.7MHz or 4.2MHz
					nds_card.baud_rate = (nds_card.cnt & 0x8000000) ? 4 : 5;
					nds_card.transfer_clock = nds_card.baud_rate;
					
					if(((nds_card.cnt >> 24) & 0x7) == 0x7) { nds_card.transfer_size = 0x4; }
					else if((nds_card.cnt >> 24) & 0x7) { nds_card.transfer_size = (0x100 << ((nds_card.cnt >> 24) & 0x7)); }
					else { nds_card.transfer_size = 0; }

					nds_card.active_transfer = true;
					nds_card.state = 0;
					nds_card.transfer_count = 0;

					//Decrypt gamecard command
					if(key_level) { key1_decrypt(nds_card.cmd_lo, nds_card.cmd_hi); }

					//std::cout<<"CART TRANSFER -> 0x" << nds_card.cnt << " -- SIZE -> 0x" << nds_card.transfer_size << " -- CMD -> 0x" << nds_card.cmd_lo << nds_card.cmd_hi << "\n";
					process_card_bus();
				}

				if((nds_card.cnt >> 24) & 0x7) { nds_card.cnt |= 0x800000; }
				else { nds_card.cnt &= ~0x800000; }
			}

			break;

		case NDS_CARDCMD_LO:
		case NDS_CARDCMD_LO+1:
		case NDS_CARDCMD_LO+2:
		case NDS_CARDCMD_LO+3:
			if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
			{
				memory_map[address] = value;
				nds_card.cmd_lo = ((memory_map[NDS_CARDCMD_LO] << 24) | (memory_map[NDS_CARDCMD_LO+1] << 16) | (memory_map[NDS_CARDCMD_LO+2] << 8) | memory_map[NDS_CARDCMD_LO+3]);
			}

			break;

		case NDS_CARDCMD_HI:
		case NDS_CARDCMD_HI+1:
		case NDS_CARDCMD_HI+2:
		case NDS_CARDCMD_HI+3:
			if((access_mode && ((nds9_exmem & 0x800) == 0)) || (!access_mode && (nds7_exmem & 0x800)))
			{
				memory_map[address] = value;
				nds_card.cmd_hi = ((memory_map[NDS_CARDCMD_HI] << 24) | (memory_map[NDS_CARDCMD_HI+1] << 16) | (memory_map[NDS_CARDCMD_HI+2] << 8) | memory_map[NDS_CARDCMD_HI+3]);
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
				nds7_spi.data = value;
				process_spi_bus();
			}

			break;

		case NDS_EXMEM:
			if(access_mode)
			{
				nds9_exmem &= 0xFF00;
				nds9_exmem |= value;
				
				//Change Bit 7 of NDS EXMEMSTAT
				if(value & 0x80) { nds7_exmem |= 0x80; }
				else { nds7_exmem &= ~0x80; }
			}

			else
			{
				nds7_exmem &= 0xFF00;
				nds7_exmem |= (value & 0x7F);
			}

			break;

		case NDS_EXMEM+1:
			if(access_mode)
			{
				nds9_exmem &= 0xFF;
				nds9_exmem |= ((value & 0xE8) << 8);

				//Change Bits 8-15 of NDS EXMEMSTAT
				nds7_exmem &= 0xFF;
				nds7_exmem |= ((value & 0xE8) << 8);
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

		case NDS_POSTFLG:
			if(access_mode)
			{
				memory_map[address] &= ~0x2;
				memory_map[address] |= (value & 0x2);
			}

			break;

		case NDS_HALTCNT:
			if(!access_mode)
			{
				std::cout<<"MMU::Warning - NDS7 wrote to HALTCNT (unhandled) \n";
				memory_map[address] = (value & 0xC0);
			}

			break;

		case NDS_SOUNDXCNT:
		case NDS_SOUNDXCNT + 1:
		case NDS_SOUNDXCNT + 2:
		case NDS_SOUNDXCNT + 3:
			if(access_mode) { return; }
			memory_map[address | (apu_io_id << 8)] = value;
			apu_stat->channel[apu_io_id].cnt = read_u32_fast(NDS_SOUNDXCNT | (apu_io_id << 8));

			//Begin playing sound channel
			if(apu_stat->channel[apu_io_id].cnt & 0x80000000)
			{

				apu_stat->channel[apu_io_id].playing = true;
				apu_stat->channel[apu_io_id].volume = (apu_stat->channel[apu_io_id].cnt & 0x7F);
				u8 format = ((apu_stat->channel[apu_io_id].cnt >> 29) & 0x3);

				//Determine loop start offset and sample length
				switch(format)
				{
					//PCM8
					case 0x0:
						apu_stat->channel[apu_io_id].data_pos = apu_stat->channel[apu_io_id].data_src;
						apu_stat->channel[apu_io_id].samples = (apu_stat->channel[apu_io_id].length * 4) + (apu_stat->channel[apu_io_id].loop_start * 4);
						break;

					//PCM16
					case 0x1:
						apu_stat->channel[apu_io_id].data_pos = apu_stat->channel[apu_io_id].data_src;
						apu_stat->channel[apu_io_id].samples = (apu_stat->channel[apu_io_id].length * 2) + (apu_stat->channel[apu_io_id].loop_start * 2);
						break;

					//IMA-ADPCM
					case 0x2:
						apu_stat->channel[apu_io_id].data_pos = apu_stat->channel[apu_io_id].data_src;
						apu_stat->channel[apu_io_id].samples = ((apu_stat->channel[apu_io_id].length - 1) * 8) + ((apu_stat->channel[apu_io_id].loop_start - 1) * 8);

						//Grab header
						apu_stat->channel[apu_io_id].adpcm_header = read_u32(apu_stat->channel[apu_io_id].data_src);
						apu_stat->channel[apu_io_id].data_src += 4;

						//Set up initial ADPCM stuff
						apu_stat->channel[apu_io_id].adpcm_val = (apu_stat->channel[apu_io_id].adpcm_header & 0xFFFF);
						apu_stat->channel[apu_io_id].adpcm_index = ((apu_stat->channel[apu_io_id].adpcm_header >> 16) & 0x7F);
						apu_stat->channel[apu_io_id].adpcm_pos = 0;

						//Decode ADPCM audio
						apu_stat->channel[apu_io_id].decode_adpcm = true;

						break;

					//PSG-Noise
					case 0x3:
						if(apu_io_id < 8)
						{
							std::cout<<"MMU::Warning - Tried to play PSG-White noise on unsupported sound channel\n";
							apu_stat->channel[apu_io_id].playing = false;
						}

						break;
				}	
			}

			//Stop playing sound channel
			else { apu_stat->channel[apu_io_id].playing = false; }

			break;

		case NDS_SOUNDXSAD:
		case NDS_SOUNDXSAD + 1:
		case NDS_SOUNDXSAD + 2:
		case NDS_SOUNDXSAD + 3:
			if(access_mode) { return; }
			memory_map[address | (apu_io_id << 8)] = value;
			apu_stat->channel[apu_io_id].data_src = read_u32_fast(NDS_SOUNDXSAD | (apu_io_id << 8)) & 0x7FFFFFF;

			break;

		case NDS_SOUNDXTMR:
		case NDS_SOUNDXTMR + 1:
			if(access_mode) { return; }
			memory_map[address | (apu_io_id << 8)] = value;

			{
				s16 raw_freq = 0;
				u16 tmr = read_u16_fast(NDS_SOUNDXTMR | (apu_io_id << 8));

				if(tmr & 0x8000)
				{
					tmr--;
					tmr = ~tmr;

					raw_freq = -tmr;
				}

				else { raw_freq = tmr; }

				if(raw_freq) { apu_stat->channel[apu_io_id].output_frequency = -16756991 / raw_freq; }
			}

			break;

		case NDS_SOUNDXPNT:
		case NDS_SOUNDXPNT + 1:
			if(access_mode) { return; }
			memory_map[address | (apu_io_id << 8)] = value;
			apu_stat->channel[apu_io_id].loop_start = read_u16_fast(NDS_SOUNDXPNT | (apu_io_id << 8));

			break;

		case NDS_SOUNDXLEN:
		case NDS_SOUNDXLEN + 1:
		case NDS_SOUNDXLEN + 2:
		case NDS_SOUNDXLEN + 3:
			if(access_mode) { return; }
			memory_map[address | (apu_io_id << 8)] = value;
			apu_stat->channel[apu_io_id].length = read_u32_fast(NDS_SOUNDXLEN | (apu_io_id << 8)) & 0x3FFFFF;

			break;

		case NDS_SOUNDCNT:
		case NDS_SOUNDCNT + 1:
			if(access_mode) { return; }
			memory_map[address] = value;

			//Master Volume + Master Enabled
			if(memory_map[NDS_SOUNDCNT + 1] & 0x80)
			{
				apu_stat->main_volume = (memory_map[NDS_SOUNDCNT] & 0x7F);
			}

			else { apu_stat->main_volume = 0; }

			break;

		case NDS_SOUNDCNT + 2:
		case NDS_SOUNDCNT + 3:
			return;

		//GX - CLEAR_COLOR
		case 0x4000350:
		case 0x4000351:
		case 0x4000352:
		case 0x4000353:
			if(access_mode)
			{
				memory_map[address] = value;

				u16 color_bytes = (memory_map[0x4000351] << 8) | memory_map[0x4000350];

				u8 red = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 green = ((color_bytes & 0x1F) << 3);
				color_bytes >>= 5;

				u8 blue = ((color_bytes & 0x1F) << 3);

				lcd_3D_stat->rear_plane_color = 0xFF000000 | (red << 16) | (green << 8) | (blue);

				//Grab alpha value separately
				lcd_3D_stat->rear_plane_alpha = (memory_map[0x4000352] & 0x1F);
			}

			break;

		//SWAP_BUFFERS
		case 0x4000540:
			if(access_mode)
			{
				//std::cout<<"GX - SWAP BUFFERS\n";
				lcd_3D_stat->gx_state |= 0x80;
			}

			break;

		//VIEWPORT
		case 0x4000580:
		case 0x4000581:
		case 0x4000582:
		case 0x4000583:
			if(access_mode)
			{
				lcd_3D_stat->current_gx_command = 0x60;
				lcd_3D_stat->command_parameters[lcd_3D_stat->parameter_index++] = value;
				if(lcd_3D_stat->parameter_index == 4) { lcd_3D_stat->process_command = true; }
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

	//Trigger OBJ palette update in LCD - Engine A
	else if((address >= 0x5000200) && (address <= 0x50003FF))
	{
		lcd_stat->obj_pal_update_a = true;
		lcd_stat->obj_pal_update_list_a[(address & 0x1FF) >> 1] = true;
	}

	//Trigger OBJ palette update in LCD - Engine B
	else if((address >= 0x5000400) && (address <= 0x50005FF))
	{
		lcd_stat->bg_pal_update_b = true;
		lcd_stat->bg_pal_update_list_b[(address & 0x1FF) >> 1] = true;
	}

	//Trigger OBJ palette update in LCD - Engine B
	else if((address >= 0x5000600) && (address <= 0x50007FF))
	{
		lcd_stat->obj_pal_update_b = true;
		lcd_stat->obj_pal_update_list_b[(address & 0x1FF) >> 1] = true;
	}

	//Trigger Extended BG palette update in LCD - Engine A Slots 0 and 1
	else if((address >= pal_a_bg_slot[0]) && (address < (pal_a_bg_slot[0] + 0x4000)))
	{
		lcd_stat->bg_ext_pal_update_a = true;
		lcd_stat->bg_ext_pal_update_list_a[(address & 0x3FFF) >> 1] = true;
	}

	//Trigger Extended BG palette update in LCD - Engine A Slots 2 and 3
	else if((address >= pal_a_bg_slot[2]) && (address < (pal_a_bg_slot[2] + 0x4000)))
	{
		lcd_stat->bg_ext_pal_update_a = true;
		lcd_stat->bg_ext_pal_update_list_a[((address & 0x3FFF) >> 1) + 0x2000] = true;
	}

	//Trigger Extended BG palette update in LCD - Engine B Slots 0-3
	else if((address >= pal_b_bg_slot[0]) && (address < (pal_b_bg_slot[0] + 0x8000)))
	{
		lcd_stat->bg_ext_pal_update_b = true;
		lcd_stat->bg_ext_pal_update_list_b[(address & 0x7FFF) >> 1] = true;
	}

	//Trigger Extended OBJ palette update in LCD - Engine A, VRAM Bank F
	else if((address >= 0x6880000) && (address <= 0x6891FFF) && (lcd_stat->vram_bank_enable[5]))
	{
		lcd_stat->obj_ext_pal_update_a = true;
		lcd_stat->obj_ext_pal_update_list_a[(address & 0x1FFF) >> 1] = true;
	}

	//Trigger Extended OBJ palette update in LCD - Engine A, VRAM Bank G
	else if((address >= 0x6894000) && (address <= 0x6895FFF) && (lcd_stat->vram_bank_enable[6]))
	{
		lcd_stat->obj_ext_pal_update_a = true;
		lcd_stat->obj_ext_pal_update_list_a[(address & 0x1FFF) >> 1] = true;
	}

	//Trigger Extended OBJ palette update in LCD - Engine B, VRAM Bank I
	else if((address >= 0x68A0000) && (address <= 0x68A1FFF))
	{
		lcd_stat->obj_ext_pal_update_b = true;
		lcd_stat->obj_ext_pal_update_list_b[(address & 0x1FFF) >> 1] = true;
	}	

	//Trigger OAM update in LCD - Engine A & B
	else if((address >= 0x7000000) && (address <= 0x70007FF))
	{
		lcd_stat->oam_update = true;
		lcd_stat->oam_update_list[(address & 0x7FF) >> 3] = true;
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
	//Always force half-word alignment
	address &= ~0x1;

	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
}

/****** Writes 4 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void NTR_MMU::write_u32_fast(u32 address, u32 value)
{
	//Always force word alignment
	address &= ~0x3;

	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
	memory_map[address+2] = ((value >> 16) & 0xFF);
	memory_map[address+3] = ((value >> 24) & 0xFF);
}

/****** Writes 8 bytes into memory - No checks done on the read, used for known memory locations such as registers ******/
void NTR_MMU::write_u64_fast(u32 address, u64 value)
{
	memory_map[address] = (value & 0xFF);
	memory_map[address+1] = ((value >> 8) & 0xFF);
	memory_map[address+2] = ((value >> 16) & 0xFF);
	memory_map[address+3] = ((value >> 24) & 0xFF);

	value >>= 16;
	value >>= 16;

	memory_map[address+4] = (value & 0xFF);
	memory_map[address+5] = ((value >> 8) & 0xFF);
	memory_map[address+6] = ((value >> 16) & 0xFF);
	memory_map[address+7] = ((value >> 24) & 0xFF);
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

	//Copy 368 bytes from header to Main RAM on boot
	for(u32 x = 0; x < 0x170; x++) { write_u8((0x27FFE00 + x), cart_data[x]); }

	file.close();
	std::cout<<"MMU::" << filename << " loaded successfully. \n";

	parse_header();

	access_mode = 1;

	//Copy ARM9 binary from offset to RAM address
	for(u32 x = 0; x < header.arm9_size; x++)
	{
		if((header.arm9_rom_offset + x) >= file_size) { break; }
		write_u8((header.arm9_ram_addr + x), cart_data[header.arm9_rom_offset + x]);
	}

	access_mode = 0;

	//Copy ARM7 binary from offset to RAM address
	for(u32 x = 0; x < header.arm7_size; x++)
	{
		if((header.arm7_rom_offset + x) >= file_size) { break; }
		write_u8((header.arm7_ram_addr + x), cart_data[header.arm7_rom_offset + x]);
	}

	access_mode = 1;

	//Detect Slot-2 device if set to automatic
	if(current_slot2_device == SLOT2_AUTO)
	{
		//Detect PASSME device
		if(read_cart_u32(0xAC) == 0x53534150) { current_slot2_device = SLOT2_PASSME; }

		//Otherwise, set Slot2 to none
		else { current_slot2_device = SLOT2_NONE; }
	}

	//Handle Slot-2 device if necessary
	switch(current_slot2_device)
	{
		//GBA Cart
		case 4:
			if(!read_slot2_file(config::nds_slot2_file))
			{
				config::nds_slot2_device = 1;
				current_slot2_device = SLOT2_NONE;
			}

			break;
	}

	//Load cart save data
	std::string backup_file = filename + ".sav";
	load_backup(backup_file);

	return true;
}

/****** Read GBA ROM to memory for Slot-2 ******/
bool NTR_MMU::read_slot2_file(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::GBA ROM" << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Get the file size
	file.seekg(0, file.end);
	u32 file_size = file.tellg();
	file.seekg(0, file.beg);

	//Only read 32MB at most
	if(file_size > 0x2000000) { file_size = 0x2000000; }

	//Read data from the ROM file
	file.read(reinterpret_cast<char*> (&memory_map[0x8000000]), file_size);

	file.close();

	std::cout<<"MMU::GBA ROM" << filename << " loaded successfully. \n";

	//Determine GBA save type if any
	for(u32 x = 0x8000000; x < (0x8000000 + file_size); x+=1)
	{
		switch(memory_map[x])
		{
			//EEPROM
			case 0x45:
				if((memory_map[x+1] == 0x45) && (memory_map[x+2] == 0x50) && (memory_map[x+3] == 0x52) && (memory_map[x+4] == 0x4F) && (memory_map[x+5] == 0x4D))
				{
					gba_save_type = GBA_EEPROM;
				}
				
				break;

			//FLASH RAM
			case 0x46:
				if((memory_map[x+1] == 0x4C) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x53) && (memory_map[x+4] == 0x48) && (memory_map[x+5] == 0x5F))
				{
					gba_save_type = GBA_FLASH_64;
				}

				//64KB "FLASH512_Vnnn"
				else if((memory_map[x+1] == 0x4C) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x53) && (memory_map[x+4] == 0x48) && (memory_map[x+5] == 0x35)
				&& (memory_map[x+6] == 0x31) && (memory_map[x+7] == 0x32)) 
				{
					gba_save_type = GBA_FLASH_64;

				}

				//128KB "FLASH1M_V"
				else if((memory_map[x+1] == 0x4C) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x53) && (memory_map[x+4] == 0x48) && (memory_map[x+5] == 0x31)
				&& (memory_map[x+6] == 0x4D))
				{
					gba_save_type = GBA_FLASH_128;
				}

				break;

			//SRAM
			case 0x53:
				if((memory_map[x+1] == 0x52) && (memory_map[x+2] == 0x41) && (memory_map[x+3] == 0x4D))
				{
					gba_save_type = GBA_SRAM;
				}

				break;
		}
	}

	//Detect presence of save file and load it
	filename += ".sav";
	file.open(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::GBA save file" << filename << " could not be opened. Check file path or permissions. \n";
		return true;
	}

	//Get the file size
	file.seekg(0, file.end);
	file_size = file.tellg();
	file.seekg(0, file.beg);

	//Load SRAM
	if(gba_save_type == GBA_SRAM)
	{
		if(file_size > 0x8000) { std::cout<<"MMU::Warning - Irregular GBA SRAM backup save size\n"; }

		else if(file_size < 0x8000)
		{
			std::cout<<"MMU::Warning - GBA SRAM backup save size too small\n";
			file.close();
			return true;
		}

		//Read data from file
		file.read(reinterpret_cast<char*> (&memory_map[0xE000000]), 0x8000);
	}

	//Load 64KB FLASH RAM
	else if(gba_save_type == GBA_FLASH_64)
	{
		if(file_size > 0x10000) { std::cout<<"MMU::Warning - Irregular GBA FLASH RAM backup save size\n"; }

		else if(file_size < 0x10000) 
		{
			std::cout<<"MMU::Warning - GBA FLASH RAM backup save size too small\n";
			file.close();
			return true;
		}

		//Read data from file
		file.read(reinterpret_cast<char*> (&memory_map[0xE000000]), 0x10d000);
	}

	std::cout<<"MMU::GBA save file" << filename << " loaded successfully. \n";

	file.close();

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

	//Setup touchscreen calibration
	touchscreen.adc_x1 = read_u16(0x27FFCD8) & 0x1FFF;
	touchscreen.adc_y1 = read_u16(0x27FFCDA) & 0x1FFF;
	touchscreen.scr_x1 = read_u8(0x27FFCDC);
	touchscreen.scr_y1 = read_u8(0x27FFCDD);

	touchscreen.adc_x2 = read_u16(0x27FFCDE) & 0x1FFF;
	touchscreen.adc_y2 = read_u16(0x27FFCE0) & 0x1FFF;
	touchscreen.scr_x2 = read_u8(0x27FFCE2);
	touchscreen.scr_y2 = read_u8(0x27FFCE3);

	in_firmware = true;

	file.close();
	std::cout<<"MMU::NDS firmware file " << filename << " loaded successfully. \n";

	//Set default ID code to Firmware ID
	key_id = (firmware[0x8] | (firmware[0x9] << 8) | (firmware[0xA] << 16) | (firmware[0xB] << 24));

	return true;
}

/****** Load backup save data ******/
bool NTR_MMU::load_backup(std::string filename)
{
	//Use config save path if applicable
	if(!config::save_path.empty())
	{
		 filename = config::save_path + util::get_filename_from_path(filename);
	}

	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " save data could not be opened. Check file path or permissions. \n";
		return false;
	}

	//Clear save data
	save_data.clear();
	save_data.resize(0x10000, 0xFF);

	//Read data from file
	file.read(reinterpret_cast<char*> (&save_data[0]), 0x10000);
	file.close();

	return true;
}

/****** Save backup save data ******/
bool NTR_MMU::save_backup(std::string filename)
{
	//Check to see if any save-based writes were made, otherwise, don't update or create new save file
	if(!do_save) { return true; }

	//Use config save path if applicable
	if(!config::save_path.empty())
	{
		 filename = config::save_path + util::get_filename_from_path(filename);
	}

	std::ofstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " save data could not be written. Check file path or permissions. \n";
		return false;
	}

	//Write the data to a file
	file.write(reinterpret_cast<char*> (&save_data[0]), 0x10000);
	file.close();

	std::cout<<"MMU::Wrote save data file " << filename <<  "\n";

	return true;
}

/****** Start the DMA channels during HBlanking periods ******/
void NTR_MMU::start_hblank_dma()
{
	for(u32 x = 0; x < 4; x++)
	{
		//Repeat bits automatically enable DMAs
		if(dma[x].control & 0x200) { dma[x].enable = true; }

		u8 dma_type = ((dma[x].control >> 27) & 0x7);
		if(dma_type == 2) { dma[x].started = true; }
	}
}

/****** Start the DMA channels during VBlanking periods ******/
void NTR_MMU::start_vblank_dma()
{
	for(u32 x = 0; x < 8; x++)
	{
		//Repeat bits automatically enable DMAs
		if(dma[x].control & 0x200) { dma[x].enable = true; }

		u8 dma_type = ((dma[x].control >> 27) & 0x7);
		if(dma_type == 1) { dma[x].started = true; }
	}
}

/****** Start the DMA channels depending on timed conditions ******/
void NTR_MMU::start_dma(u8 dma_bits)
{
	for(u32 x = 0; x < 8; x++)
	{
		//Repeat bits automatically enable DMAs
		if(dma[x].control & 0x200) { dma[x].enable = true; }

		u8 dma_type = ((dma[x].control >> 27) & 0x7);
		if(dma_type == dma_bits) { dma[x].started = true; }
	}
}

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

	std::cout<<"MMU::Game Title - " << util::make_ascii_printable(header.title) << "\n";
	std::cout<<"MMU::Game Code - " << util::make_ascii_printable(header.game_code) << "\n";
	std::cout<<"MMU::Maker Code - " << util::make_ascii_printable(header.maker_code) << "\n";

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

	//Set default ID code to Game ID
	key_id = (cart_data[0xC] | (cart_data[0xD] << 8) | (cart_data[0xE] << 16) | (cart_data[0xF] << 24));

	//Calculate Chip ID 1
	u32 chip_id = 0xC2;
	chip_id |= ((((128 << cart_data[0x14]) / 1024) - 1) << 8);

	write_u32(0x27FFC00, chip_id);
	write_u32(0x27FF800, chip_id);
	write_u32(0x27FFC04, chip_id);
	write_u32(0x27FF804, chip_id);

	nds_card.chip_id = chip_id;
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

/****** Handles various AUXSPI Bus interactions ******/
void NTR_MMU::process_aux_spi_bus()
{
	bool do_command = true;

	//Exit read or write mode
	switch(nds_aux_spi.state)
	{
		case 0x05:
			//Continue Write Mode in certain cases when writing 0x5
			if(((nds_aux_spi.last_state == 0x82) || (nds_aux_spi.last_state == 0x8A)) && (nds_aux_spi.data != 0))
			{
				nds_aux_spi.state = nds_aux_spi.last_state;
				save_data[nds_aux_spi.access_addr++] = 0x5;
			}

			break;

		case 0x82:
		case 0x83:
		case 0x8A:
		case 0x8B:
			if(nds_aux_spi.data == 0x5)
			{
				//Auto-detect save type
				if((current_save_type == AUTO) && (nds_aux_spi.state == 0x83) && (nds_aux_spi.access_index == 1))
				{
					current_save_type = EEPROM_512;
					std::cout<<"MMU::Save Type Autodetected: EEPROM_512\n";
				}

				else if((current_save_type == AUTO) && (nds_aux_spi.state == 0x83) && (nds_aux_spi.access_index == 2))
				{
					current_save_type = EEPROM;
					std::cout<<"MMU::Save Type Autodetected: EEPROM\n";
				}

				else if((current_save_type == AUTO) && (nds_aux_spi.state == 0x83) && (nds_aux_spi.access_index == 3))
				{
					current_save_type = FRAM;
					std::cout<<"MMU::Save Type Autodetected: FRAM\n";
				}

				nds_aux_spi.last_state = nds_aux_spi.state;
				nds_aux_spi.state = 0;
			}

			break;
	}

	//Handle AUX SPI state - Command parameters or data 
	switch(nds_aux_spi.state)
	{
		//Write to status
		case 0x1:
			nds_aux_spi.eeprom_stat &= ~0xC;
			nds_aux_spi.eeprom_stat |= (nds_aux_spi.data & 0xC) | 0x2;
			nds_aux_spi.state = 0;
			do_command = false;
			break;

		//Write to EEPROM
		case 0x2:
		case 0xA:
			if((current_save_type == EEPROM_512) || (current_save_type == AUTO))
			{
				nds_aux_spi.access_addr = nds_aux_spi.data;
				nds_aux_spi.access_addr |= (nds_aux_spi.state == 0xA) ? 0x100 : 0;
				nds_aux_spi.state |= 0x80;
			}

			else
			{
				nds_aux_spi.access_index++;

				if(nds_aux_spi.access_index == 1) { nds_aux_spi.access_addr = (nds_aux_spi.data << 8); }

				else if(nds_aux_spi.access_index == 2)
				{
					nds_aux_spi.access_addr |= nds_aux_spi.data;
					nds_aux_spi.access_index = 0;
					nds_aux_spi.state |= 0x80;
				}

			}

			nds_aux_spi.data = 0xFF;

			do_command = false;
			do_save = true;
			break;

		//Read from EEPROM low
		case 0x3:
		case 0xB:
			if((current_save_type == EEPROM_512) || (current_save_type == AUTO))
			{
				nds_aux_spi.access_addr = nds_aux_spi.data;
				nds_aux_spi.access_addr |= (nds_aux_spi.state == 0xB) ? 0x100 : 0;
				nds_aux_spi.state |= 0x80;
			}

			else
			{
				nds_aux_spi.access_index++;

				if(nds_aux_spi.access_index == 1) { nds_aux_spi.access_addr = (nds_aux_spi.data << 8); }

				else if(nds_aux_spi.access_index == 2)
				{
					nds_aux_spi.access_addr |= nds_aux_spi.data;
					nds_aux_spi.access_index = 0;
					nds_aux_spi.state |= 0x80;
				}

			}

			nds_aux_spi.data = 0xFF;

			do_command = false;
			break;

		//Read from status register
		case 0x5:
			nds_aux_spi.data = nds_aux_spi.eeprom_stat;
			nds_aux_spi.state = 0;
			do_command = false;
			break;

		//Write EEPROM data low
		case 0x82:
		case 0x8A:
			save_data[nds_aux_spi.access_addr++] = nds_aux_spi.data;
			nds_aux_spi.access_index++;
			do_command = false;
			break;

		//Read EEPROM data low
		case 0x83:
		case 0x8B:
			nds_aux_spi.data = save_data[nds_aux_spi.access_addr++];
			nds_aux_spi.access_index++;
			do_command = false;
			break;
	}

	nds_aux_spi.backup_cmd = nds_aux_spi.data;

	//Handle AUX SPI save memory commands
	if(do_command)
	{
		switch(nds_aux_spi.backup_cmd)
		{
			//Write to status register
			case 0x1:
				if((current_save_type == EEPROM) || (current_save_type == AUTO)) { nds_aux_spi.data = nds_aux_spi.eeprom_stat; }
				else if(current_save_type == EEPROM_512) { nds_aux_spi.data = (nds_aux_spi.eeprom_stat | 0xF0); }
				nds_aux_spi.data = 0xFF;
				nds_aux_spi.state = 0x1;
				break;

			//Write to EEPROM low
			case 0x2:
				nds_aux_spi.data = 0xFF;
				nds_aux_spi.access_index = 0;
				nds_aux_spi.state = 0x2;
				break;

			//Read from EEPROM low
			case 0x3:
				nds_aux_spi.data = 0xFF;
				nds_aux_spi.access_index = 0;
				nds_aux_spi.state = 0x3;
				break;

			//Read from status register
			case 0x5:
				nds_aux_spi.data = 0xFF;
				nds_aux_spi.state = 0x5;
				break;

			//Write enable
			case 0x6:
				nds_aux_spi.data = 0xFF;
				break;

			//Write to EEPROM high
			case 0xA:
				nds_aux_spi.data = 0xFF;
				nds_aux_spi.access_index = 0;
				nds_aux_spi.state = 0xA;
				break;

			//Read from EEPROM high
			case 0xB:
				nds_aux_spi.data = 0xFF;
				nds_aux_spi.access_index = 0;
				nds_aux_spi.state = 0xB;
				break;

			//Read JEDEC ID
			case 0x9F:
				nds_aux_spi.data = 0xFF;
				nds_aux_spi.state = 0x9F;
				break;
		}
	}

	//Reset SPI for next transfer
	write_u16_fast(NDS_AUXSPIDATA, nds_aux_spi.data);
	nds_aux_spi.transfer_count = 0;
	nds_aux_spi.cnt &= ~0x80;
}

/****** Handles read and write data operations to the firmware ******/
void NTR_MMU::process_firmware()
{
	bool new_command = false;

	switch(firmware_state)
	{
		case 0x0000:
		case 0x0303:
			new_command = true;
			break;
	}

	//Check to see if a new command has been sent
	if((new_command) && (nds7_spi.data))
	{
		switch(nds7_spi.data)
		{
			//Read Data
			case 0x3:
				firmware_state = 0x0300;
				firmware_index = 0;
				return;

			//Unknown
			default:
				std::cout<<"MMU::Warning - Unknown Firmware command 0x" << (u32)nds7_spi.data << "\n";
				return;
		}
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
	}

	u16 touch_x = (g_pad->mouse_x << 4);
	u16 touch_y = (g_pad->mouse_y << 4);

	touch_x &= 0xFFF;
	touch_y &= 0xFFF;

	//Crude emulation of Z positioning, just enough to register difference between light and strong touches
	u16 touch_z1 = 0x0;
	u16 touch_z2 = config::touch_mode ? 0x1FF : 0xF;

	//Process various touchscreen states
	switch(touchscreen_state)
	{
		//Read Temp 0 Byte 1
		case 0x0:
			if(nds7_spi.data & 0x4) { }
			else { nds7_spi.data = 0x0; }

			touchscreen_state++;
			break;

		//Read Temp 0 Byte 2
		case 0x1:
			if(nds7_spi.data & 0x4) { }
			else { nds7_spi.data = 0x0; }

			touchscreen_state = 0;
			break;

		//Read Touch Y Byte 1
		case 0x2:
			if(nds7_spi.cnt & 0x800)
			{
				nds7_spi.data = ((touch_y << 3) & 0xFF);
				touchscreen_state++;
			}

			else { nds7_spi.data = ((touch_y >> 5) & 0xFF); }

			break;

		//Read Touch Y Byte 2
		case 0x3:
			nds7_spi.data = ((touch_y >> 5) & 0xFF);
			touchscreen_state = 2;
			break;

		//Read Z1 Position Byte 1
		case 0x6:
			nds7_spi.data = (touch_z1 >> 5);
			touchscreen_state++;
			break;

		//Read Z1 Position Byte 2
		case 0x7:
			nds7_spi.data = ((touch_z1 & 0x1F) << 3);
			touchscreen_state = 6;
			break;

		//Read Z2 Position Byte 1
		case 0x8:
			nds7_spi.data = (touch_z2 >> 5);
			touchscreen_state++;
			break;

		//Read Z2 Position Byte 2
		case 0x9:
			nds7_spi.data = ((touch_z2 & 0x1F) << 3);
			touchscreen_state = 8;
			break;

		//Read Touch X Byte 1
		case 0xA:
			if(nds7_spi.cnt & 0x800)
			{
				nds7_spi.data = ((touch_x << 3) & 0xFF);
				touchscreen_state++;
			}

			else { nds7_spi.data = ((touch_x >> 5) & 0xFF); }

			break;

		//Read Touch X Byte 2
		case 0xB:
			nds7_spi.data = ((touch_x >> 5) & 0xFF);
			touchscreen_state = 0xA;
			break;

		//Read AUX Input Byte 1
		case 0xC:
			if(nds7_spi.data & 0x4) { }
			else { nds7_spi.data = 0x0; }

			touchscreen_state++;
			break;

		//Read AUX Input Byte 2
		case 0xD:
			if(nds7_spi.data & 0x4) { }
			else { nds7_spi.data = 0x0; }

			touchscreen_state = 0xC;
			break;

		//Read Temp 1 Byte 1
		case 0xE:
			if(nds7_spi.data & 0x4) { }
			else { nds7_spi.data = 0x0; }

			touchscreen_state++;
			break;

		//Read Temp 1 Byte 2
		case 0xF:
			if(nds7_spi.data & 0x4) { }
			else { nds7_spi.data = 0x0; }

			touchscreen_state = 0xE;
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
	firmware[0x20] = 0xC0;
	firmware[0x21] = 0x7F;

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
	firmware[0x3FE58] = 0x00;
	firmware[0x3FE59] = 0x00;
	firmware[0x3FE5A] = 0x00;
	firmware[0x3FE5B] = 0x00;
	firmware[0x3FE5C] = 0x00;
	firmware[0x3FE5D] = 0x00;
	firmware[0x3FE5E] = 0xF0;
	firmware[0x3FE5F] = 0x0F;
	firmware[0x3FE60] = 0xF0;
	firmware[0x3FE61] = 0x0B;
	firmware[0x3FE62] = 0xFF;
	firmware[0x3FE63] = 0xBF;

	//User Settings Area 1 - Nickname length
	firmware[0x3FE1A] = 0x4;

	//User Settings CRC16
	firmware[0x3FE72] = 0x69;
	firmware[0x3FE73] = 0x1E;

	//Copy User Settings 0 to User Settings 1
	for(u32 x = 0; x < 0x100; x++)
	{
		firmware[0x3FF00 + x] = firmware[0x3FE00 + x];
	}

	//Copy firmware user settings to RAM
	for(u32 x = 0; x < 0x6C; x++)
	{
		memory_map[(0x23FFC80 + x)] = firmware[0x3FE00 + x];
	}

	//Setup touchscreen calibration
	touchscreen.adc_x1 = read_u16(0x27FFCD8) & 0x1FFF;
	touchscreen.adc_y1 = read_u16(0x27FFCDA) & 0x1FFF;
	touchscreen.scr_x1 = read_u8(0x27FFCDC);
	touchscreen.scr_y1 = read_u8(0x27FFCDD);

	touchscreen.adc_x2 = read_u16(0x27FFCDE) & 0x1FFF;
	touchscreen.adc_y2 = read_u16(0x27FFCE0) & 0x1FFF;
	touchscreen.scr_x2 = read_u8(0x27FFCE2);
	touchscreen.scr_y2 = read_u8(0x27FFCE3);
}
	
/****** Points the MMU to an lcd_data structure (FROM THE LCD ITSELF) ******/
void NTR_MMU::set_lcd_data(ntr_lcd_data* ex_lcd_stat) { lcd_stat = ex_lcd_stat; }

/****** Points the MMU to an lcd_3D_data structure (FROM THE LCD ITSELF) ******/
void NTR_MMU::set_lcd_3D_data(ntr_lcd_3D_data* ex_lcd_3D_stat) { lcd_3D_stat = ex_lcd_3D_stat; }

/****** Points the MMU to an apu_data structure (FROM THE APU ITSELF) ******/
void NTR_MMU::set_apu_data(ntr_apu_data* ex_apu_stat) { apu_stat = ex_apu_stat; }

/****** Points the MMU to the NDS7 Program Counter ******/
void NTR_MMU::set_nds7_pc(u32* ex_pc) { nds7_pc = ex_pc; }

/****** Points the MMU to the NDS9 Program Counter ******/
void NTR_MMU::set_nds9_pc(u32* ex_pc) { nds9_pc = ex_pc; }
