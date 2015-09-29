// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : mmu.cpp
// Date : May 09, 2015
// Description : Game Boy (Color) memory manager unit
//
// Handles reading and writing bytes to memory locations
// Used to switch ROM and RAM banks
// Also loads ROM and BIOS files

#include "mmu.h"
#include "common/cgfx_common.h"

/****** MMU Constructor ******/
DMG_MMU::DMG_MMU() 
{
	reset();
}

/****** MMU Deconstructor ******/
DMG_MMU::~DMG_MMU() 
{ 
	save_backup(config::save_file);
	memory_map.clear();
	std::cout<<"MMU::Shutdown\n"; 
}

/****** MMU Reset ******/
void DMG_MMU::reset()
{
	memory_map.clear();
	memory_map.resize(0x10000, 0);

	rom_bank = 1;
	ram_bank = 0;
	wram_bank = 1;
	vram_bank = 0;
	bank_bits = 0;
	bank_mode = 0;
	ram_banking_enabled = false;

	in_bios = config::use_bios;
	bios_type = 1;
	bios_size = 0x100;

	cart.rom_size = 0;
	cart.ram_size = 0;
	cart.mbc_type = ROM_ONLY;
	cart.battery = false;
	cart.ram = false;
	cart.rtc = false;
	cart.rtc_enabled = false;
	cart.rtc_latch_1 = cart.rtc_latch_2 = 0xFF;

	//Resize various banks
	read_only_bank.resize(0x200);
	for(int x = 0; x < 0x200; x++) { read_only_bank[x].resize(0x4000, 0); }

	random_access_bank.resize(0x10);
	for(int x = 0; x < 0x10; x++) { random_access_bank[x].resize(0x2000, 0); }

	working_ram_bank.resize(0x8);
	for(int x = 0; x < 0x8; x++) { working_ram_bank[x].resize(0x1000, 0); }

	video_ram.resize(0x2);
	for(int x = 0; x < 0x2; x++) { video_ram[x].resize(0x2000, 0); }

	g_pad = NULL;

	std::cout<<"MMU::Initialized\n";
}

/****** Read byte from memory ******/
u8 DMG_MMU::read_u8(u16 address) 
{ 
	//Read from BIOS
	if(in_bios)
	{
		//GBC BIOS reads from 0x00 to 0xFF and 0x200 to 0x900
		//0x100 - 0x1FF is reserved for the Nintendo logo + checksum + first lines of game code
		//For the latter, just read from the cartridge ROM
		if((bios_size == 0x900) && (address > 0x100) && (address < 0x200)) { return memory_map[address]; }
		
		else if(address == 0x100) 
		{ 
			in_bios = false; 
			std::cout<<"MMU::Exiting BIOS \n";

			//For DMG on GBC games, we switch back to DMG Mode (we just take the colors the BIOS gives us)
			if((bios_size == 0x900) && (memory_map[ROM_COLOR] != 0x80) && (memory_map[ROM_COLOR] != 0xC0)) { config::gb_type = 1; }
		}

		else if(address < bios_size) { return bios[address]; }
	}

	//Read using ROM Banking
	if((address >= 0x4000) && (address <= 0x7FFF) && (cart.mbc_type != ROM_ONLY)) { return mbc_read(address); }

	//Read using RAM Banking
	if((address >= 0xA000) && (address <= 0xBFFF) && (cart.ram) && (cart.mbc_type != ROM_ONLY)) { return mbc_read(address); }

	//Read from VRAM, GBC uses banking
	if((address >= 0x8000) && (address <= 0x9FFF))
	{
		//GBC read from VRAM Bank 1
		if((vram_bank == 1) && (config::gb_type == 2)) { return video_ram[1][address - 0x8000]; }
		
		//GBC read from VRAM Bank 0 - DMG read normally, also from Bank 0, though it doesn't use banking technically
		else { return video_ram[0][address - 0x8000]; }
	}

	//In GBC mode, read from Working RAM using Banking
	if((address >= 0xC000) && (address <= 0xDFFF) && (config::gb_type == 2)) 
	{
		//Read from Bank 0 always when address is within 0xC000 - 0xCFFF
		if((address >= 0xC000) && (address <= 0xCFFF)) { return working_ram_bank[0][address - 0xC000]; }
			
		//Read from selected Bank when address is within 0xD000 - 0xDFFF
		else if((address >= 0xD000) && (address <= 0xDFFF)) { return working_ram_bank[wram_bank][address - 0xD000]; }
	}

	//Read background color palette data
	if(address == REG_BCPD)
	{ 
		u8 hi_lo = (memory_map[REG_BCPS] & 0x1);
		u8 color = (memory_map[REG_BCPS] >> 1) & 0x3;
		u8 palette = (memory_map[REG_BCPS] >> 3) & 0x7;

		//Read lower-nibble of color
		if(hi_lo == 0) 
		{ 
			return (lcd_stat->bg_colors_raw[color][palette] & 0xFF);
		}

		//Read upper-nibble of color
		else
		{
			return (lcd_stat->bg_colors_raw[color][palette] >> 8);
		}
	}

	//Read sprite color palette data
	if(address == REG_OCPD) 
	{ 
		u8 hi_lo = (memory_map[REG_OCPS] & 0x1);
		u8 color = (memory_map[REG_OCPS] >> 1) & 0x3;
		u8 palette = (memory_map[REG_OCPS] >> 3) & 0x7;

		//Read lower-nibble of color
		if(hi_lo == 0) 
		{ 
			return (lcd_stat->obj_colors_raw[color][palette] & 0xFF);
		}

		//Read upper-nibble of color
		else
		{
			return (lcd_stat->obj_colors_raw[color][palette] >> 8);
		}
	}

	//Read from P1
	else if(address == 0xFF00) { return g_pad->read(); }

	//Read normally
	return memory_map[address]; 

}

/****** Read signed byte from memory ******/
s8 DMG_MMU::read_s8(u16 address) 
{
	u8 temp = read_u8(address);
	s8 s_temp = (s8)temp;
	return s_temp;
}

/****** Read word from memory ******/
u16 DMG_MMU::read_u16(u16 address) 
{
	return (read_u8(address+1) << 8) | read_u8(address);
}

/****** Write Byte To Memory ******/
void DMG_MMU::write_u8(u16 address, u8 value) 
{
	if(cart.mbc_type != ROM_ONLY) { mbc_write(address, value); }

	//Write to VRAM, GBC uses banking
	if((address >= 0x8000) && (address <= 0x9FFF))
	{
		//GBC write to VRAM Bank 1
		if((vram_bank == 1) && (config::gb_type == 2)) 
		{
			previous_value = video_ram[1][address - 0x8000];
			video_ram[1][address - 0x8000] = value;
		}
		
		//GBC write to VRAM Bank 0 - DMG read normally, also from Bank 0, though it doesn't use banking technically
		else 
		{
			previous_value = video_ram[0][address - 0x8000];
			video_ram[0][address - 0x8000] = value;
		}
	}

	//NR11 - Duty Cycle
	else if(address == NR11)
	{
		memory_map[address] = value;
		apu_stat->channel[0].duty_cycle = (value >> 6) & 0x3;

		switch(apu_stat->channel[0].duty_cycle)
		{
			case 0x0: 
				apu_stat->channel[0].duty_cycle_start = 0;
				apu_stat->channel[0].duty_cycle_end = 1;
				break;

			case 0x1: 
				apu_stat->channel[0].duty_cycle_start = 0;
				apu_stat->channel[0].duty_cycle_end = 2;
				break;

			case 0x2: 
				apu_stat->channel[0].duty_cycle_start = 0;
				apu_stat->channel[0].duty_cycle_end = 4;
				break;

			case 0x3: 
				apu_stat->channel[0].duty_cycle_start = 0;
				apu_stat->channel[0].duty_cycle_end = 6;
				break;
		}
	}

	//NR12 - Envelope, Volume
	else if(address == NR12)
	{
		memory_map[address] = value;
		u8 current_step = apu_stat->channel[0].envelope_step;
		u8 next_step = (value & 0x07) ? 1 : 0;
		u8 next_volume = (value >> 4);
		u8 next_direction = (value & 0x08) ? 1 : 0;

		//Envelope timer is not reset unless sound is initializes
		//Envelope timer does start if it is turned off at first, but turned on after sound initializes
		if((current_step == 0) && (next_step != 0)) 
		{
			apu_stat->channel[0].volume = (value >> 4);
			apu_stat->channel[0].envelope_direction = (value & 0x08) ? 1 : 0;
			apu_stat->channel[0].envelope_step = (value & 0x07);
			apu_stat->channel[0].envelope_counter = 0; 
		}

		//Turn off sound channel if envelope volume is 0 and mode is subtraction
		if((next_direction == 0) && (next_volume == 0)) { apu_stat->channel[0].playing = false; }
	}

	//NR13 - Frequency LO
	else if(address == NR13)
	{
		memory_map[address] = value;
		
		//If sweep is active, do not update frequency
		//This emulates the sweep's shadow registers
		if(!apu_stat->channel[0].sweep_on)
		{
			apu_stat->channel[0].raw_frequency = ((memory_map[NR14] << 8) | memory_map[NR13]) & 0x7FF;
			apu_stat->channel[0].output_frequency = (131072.0 / (2048 - apu_stat->channel[0].raw_frequency));
		}
	}

	//NR14 - Frequency HI, Initial
	else if(address == NR14)
	{
		memory_map[address] = value;

		//If sweep is active, do not update frequency
		//This emulates the sweep's shadow registers
		//However, always update frequency when initializing sound
		if((!apu_stat->channel[0].sweep_on) || (value & 0x80))
		{
			apu_stat->channel[0].raw_frequency = ((memory_map[NR14] << 8) | memory_map[NR13]) & 0x7FF;
			apu_stat->channel[0].output_frequency = (131072.0 / (2048 - apu_stat->channel[0].raw_frequency));
		}

		//Check initial flag to start playing sound, check length flag
		if(value & 0x80) { apu_stat->channel[0].playing = true; }
		apu_stat->channel[0].length_flag = (value & 0x40) ? true : false;

		//Initialize sound
		//Updates various sound parameters
		if(value & 0x80) 
		{
			//Duty cycle
			switch((memory_map[NR11] >> 6))
			{
				case 0x0:
					apu_stat->channel[0].duty_cycle_start = 1;
					apu_stat->channel[0].duty_cycle_end = 2;
					break;

				case 0x1:
					apu_stat->channel[0].duty_cycle_start = 0;
					apu_stat->channel[0].duty_cycle_end = 2;
					break;

				case 0x2:
					apu_stat->channel[0].duty_cycle_start = 0;
					apu_stat->channel[0].duty_cycle_end = 4;
					break;

				case 0x3:
					apu_stat->channel[0].duty_cycle_start = 2;
					apu_stat->channel[0].duty_cycle_end = 8;
					break;
			}

			//Duration
			if(!apu_stat->channel[0].length_flag) { apu_stat->channel[0].duration = 5000; }
		
			else 
			{
				apu_stat->channel[0].duration = (memory_map[NR11] & 0x3F);
				apu_stat->channel[0].duration = ((64 - apu_stat->channel[0].duration) / 256.0) * 1000.0;
			}

			//Volume & Envelope
			apu_stat->channel[0].volume = (memory_map[NR12] >> 4);
			apu_stat->channel[0].envelope_direction = (memory_map[NR12] & 0x08) ? 1 : 0;
			apu_stat->channel[0].envelope_step = (memory_map[NR12] & 0x07);

			//Sweep
			apu_stat->channel[0].sweep_direction = (memory_map[NR10] & 0x08) ? 1 : 0;
			apu_stat->channel[0].sweep_time = ((memory_map[NR10] >> 4) & 0x7);
			apu_stat->channel[0].sweep_shift = (memory_map[NR10] & 0x7);

			if((apu_stat->channel[0].sweep_shift != 0) || (apu_stat->channel[0].sweep_time != 0)) { apu_stat->channel[0].sweep_on = true; }
			else { apu_stat->channel[0].sweep_on = false; }

			//Internal APU time-keeping
			apu_stat->channel[0].frequency_distance = 0;
			apu_stat->channel[0].sample_length = (apu_stat->channel[0].duration * apu_stat->sample_rate)/1000;
			apu_stat->channel[0].envelope_counter = 0;
			apu_stat->channel[0].sweep_counter = 0;
		}
	}

	//NR21 - Duration, Duty Cycle
	else if(address == NR21)
	{
		memory_map[address] = value;
		apu_stat->channel[1].duty_cycle = (value >> 6) & 0x3;

		switch(apu_stat->channel[1].duty_cycle)
		{
			case 0x0: 
				apu_stat->channel[1].duty_cycle_start = 0;
				apu_stat->channel[1].duty_cycle_end = 1;
				break;

			case 0x1: 
				apu_stat->channel[1].duty_cycle_start = 0;
				apu_stat->channel[1].duty_cycle_end = 2;
				break;

			case 0x2: 
				apu_stat->channel[1].duty_cycle_start = 0;
				apu_stat->channel[1].duty_cycle_end = 4;
				break;

			case 0x3: 
				apu_stat->channel[1].duty_cycle_start = 0;
				apu_stat->channel[1].duty_cycle_end = 6;
				break;
		}
	}

	//NR22 - Envelope, Volume
	else if(address == NR22)
	{
		memory_map[address] = value;

		u8 current_step = apu_stat->channel[1].envelope_step;
		u8 next_step = (value & 0x07) ? 1 : 0;
		u8 next_volume = (value >> 4);
		u8 next_direction = (value & 0x08) ? 1 : 0;

		//Envelope timer is not reset unless sound initializes
		//Envelope timer does start if it is turned off at first, but turned on after sound initializes
		if((current_step == 0) && (next_step != 0)) 
		{
			apu_stat->channel[1].volume = (value >> 4);
			apu_stat->channel[1].envelope_direction = (value & 0x08) ? 1 : 0;
			apu_stat->channel[1].envelope_step = (value & 0x07);
			apu_stat->channel[1].envelope_counter = 0; 
		}

		//Turn off sound channel if envelope volume is 0 and mode is subtraction
		if((next_direction == 0) && (next_volume == 0)) { apu_stat->channel[1].playing = false; }
	}

	//NR23 - Frequency LO
	else if(address == NR23)
	{
		memory_map[address] = value;
		
		apu_stat->channel[1].raw_frequency = ((memory_map[NR24] << 8) | memory_map[NR23]) & 0x7FF;
		apu_stat->channel[1].output_frequency = (131072.0 / (2048 - apu_stat->channel[1].raw_frequency));
	}

	//NR24 - Frequency HI, Initial
	else if(address == NR24)
	{
		memory_map[address] = value;

		//Frequency
		apu_stat->channel[1].raw_frequency = ((memory_map[NR24] << 8) | memory_map[NR23]) & 0x7FF;
		apu_stat->channel[1].output_frequency = (131072.0 / (2048 - apu_stat->channel[1].raw_frequency));

		//Check initial flag to start playing sound, check length flag
		if(value & 0x80) { apu_stat->channel[1].playing = true; }
		apu_stat->channel[1].length_flag = (value & 0x40) ? true : false;

		//Initialize sound
		//Updates various sound parameters
		if(value & 0x80) 
		{
			//Duty cycle
			switch((memory_map[NR21] >> 6))
			{
				case 0x0:
					apu_stat->channel[1].duty_cycle_start = 1;
					apu_stat->channel[1].duty_cycle_end = 2;
					break;

				case 0x1:
					apu_stat->channel[1].duty_cycle_start = 0;
					apu_stat->channel[1].duty_cycle_end = 2;
					break;

				case 0x2:
					apu_stat->channel[1].duty_cycle_start = 0;
					apu_stat->channel[1].duty_cycle_end = 4;
					break;

				case 0x3:
					apu_stat->channel[1].duty_cycle_start = 2;
					apu_stat->channel[1].duty_cycle_end = 8;
					break;
			}

			//Duration
			if(!apu_stat->channel[1].length_flag) { apu_stat->channel[1].duration = 5000; }
		
			else 
			{
				apu_stat->channel[1].duration = (memory_map[NR21] & 0x3F);
				apu_stat->channel[1].duration = ((64 - apu_stat->channel[1].duration) / 256.0) * 1000.0;
			}

			//Volume & Envelope
			apu_stat->channel[1].volume = (memory_map[NR22] >> 4);
			apu_stat->channel[1].envelope_direction = (memory_map[NR22] & 0x08) ? 1 : 0;
			apu_stat->channel[1].envelope_step = (memory_map[NR22] & 0x07);

			//Internal APU time-keeping
			apu_stat->channel[1].frequency_distance = 0;
			apu_stat->channel[1].sample_length = (apu_stat->channel[1].duration * apu_stat->sample_rate)/1000;
			apu_stat->channel[1].envelope_counter = 0;
			apu_stat->channel[1].sweep_counter = 0;
		}
	}

	//NR31 - Duration
	else if(address == NR31)
	{
		memory_map[address] = value;

		//Duration
		if(!apu_stat->channel[2].length_flag) { apu_stat->channel[2].duration = 5000; }
		
		else 
		{
			apu_stat->channel[2].duration = memory_map[NR31];
			apu_stat->channel[2].duration = ((256 - apu_stat->channel[2].duration) / 256.0) * 1000.0;
		}
		
		apu_stat->channel[2].sample_length = (apu_stat->channel[2].duration * apu_stat->sample_rate)/1000;
	}

	//NR32 - Volume
	else if(address == NR32)
	{
		memory_map[address] = value;

		switch((value >> 5) & 0x3)
		{
			//0%
			case 0x0: apu_stat->channel[2].volume = 4; break;
	
			//100%
			case 0x1: apu_stat->channel[2].volume = 0; break;

			//50%
			case 0x2: apu_stat->channel[2].volume = 1; break;

			//25%
			case 0x3: apu_stat->channel[2].volume = 2; break;
		}
	}

	//NR33 - Frequency LO
	else if(address == NR33)
	{
		memory_map[address] = value;
		apu_stat->channel[2].raw_frequency = ((memory_map[NR34] << 8) | memory_map[NR33]) & 0x7FF;
		apu_stat->channel[2].output_frequency = (131072.0 / (2048 - apu_stat->channel[2].raw_frequency));
		apu_stat->channel[2].output_frequency /= 2.0;
	}

	//NR34 - Frequency HI, Initial
	else if(address == NR34)
	{
		memory_map[address] = value;

		//Check initial flag to start playing sound, check length flag
		if(value & 0x80) { apu_stat->channel[2].playing = true; }
		apu_stat->channel[2].length_flag = (value & 0x40) ? true : false;

		//Frequency
		apu_stat->channel[2].raw_frequency = ((memory_map[NR34] << 8) | memory_map[NR33]) & 0x7FF;
		apu_stat->channel[2].output_frequency = (131072.0 / (2048 - apu_stat->channel[2].raw_frequency));
		apu_stat->channel[2].output_frequency /= 2.0;

		if(value & 0x80)
		{
			//Internal APU time-keeping
			apu_stat->channel[2].frequency_distance = 0;
		}
	}

	//NR42 - Envelope, Volume
	else if(address == NR42)
	{
		memory_map[address] = value;

		u8 current_step = apu_stat->channel[3].envelope_step;
		u8 next_step = (value & 0x07) ? 1 : 0;
		u8 next_volume = (value >> 4);
		u8 next_direction = (value & 0x08) ? 1 : 0;

		//Envelope timer is not reset unless sound initializes
		//Envelope timer does start if it is turned off at first, but turned on after sound initializes
		if((current_step == 0) && (next_step != 0)) 
		{
			apu_stat->channel[3].volume = (value >> 4);
			apu_stat->channel[3].envelope_direction = (value & 0x08) ? 1 : 0;
			apu_stat->channel[3].envelope_step = (value & 0x07);
			apu_stat->channel[3].envelope_counter = 0; 
		}

		//Turn off sound channel if envelope volume is 0 and mode is subtraction
		if((next_direction == 0) && (next_volume == 0)) { apu_stat->channel[3].playing = false; }
	}

	//NR43 - Polynomial Counter
	else if(address == NR43)
	{
		memory_map[address] = value;

		//Dividing ratio
		switch(value & 0x7)
		{
			case 0x0: apu_stat->noise_dividing_ratio = 0.5; break;
			case 0x1: apu_stat->noise_dividing_ratio = 1.0; break;
			case 0x2: apu_stat->noise_dividing_ratio = 2.0; break;
			case 0x3: apu_stat->noise_dividing_ratio = 3.0; break;
			case 0x4: apu_stat->noise_dividing_ratio = 4.0; break;
			case 0x5: apu_stat->noise_dividing_ratio = 5.0; break;
			case 0x6: apu_stat->noise_dividing_ratio = 6.0; break;
			case 0x7: apu_stat->noise_dividing_ratio = 7.0; break;
		}

		//Prescalar
		apu_stat->noise_prescalar = 2 << (value >> 4);

		//LSFR Stages
		if(value & 0x8) { apu_stat->noise_stages = 7; }
		else { apu_stat->noise_stages = 15; }

		apu_stat->channel[3].output_frequency = (524288/apu_stat->noise_dividing_ratio)/apu_stat->noise_prescalar;
	}

	//NR44 - Initial
	else if(address == NR44)
	{
		memory_map[address] = value;

		//Check initial flag to start playing sound, check length flag
		if(value & 0x80) { apu_stat->channel[3].playing = true; }
		apu_stat->channel[3].length_flag = (value & 0x40) ? true : false;

		if(value & 0x80)
		{
			//Duration
			if(!apu_stat->channel[3].length_flag) { apu_stat->channel[3].duration = 5000; }
		
			else 
			{
				apu_stat->channel[3].duration = (memory_map[NR41] & 0x3F);
				apu_stat->channel[3].duration = ((64 - apu_stat->channel[3].duration) / 256.0) * 1000.0;
			}

			apu_stat->channel[3].sample_length = (apu_stat->channel[3].duration * apu_stat->sample_rate)/1000;

			//Volume & Envelope
			apu_stat->channel[3].volume = (memory_map[NR42] >> 4);
			apu_stat->channel[3].envelope_direction = (memory_map[NR42] & 0x08) ? 1 : 0;
			apu_stat->channel[3].envelope_step = (memory_map[NR42] & 0x07);

			//Internal APU time-keeping
			apu_stat->channel[3].frequency_distance = 0;
			apu_stat->channel[3].envelope_counter = 0;
			apu_stat->noise_7_stage_lsfr = 0x40;
			apu_stat->noise_15_stage_lsfr = 0x4000;
		}
	}

	//NR51 SO1-SO2 Sound Channel Output
	//Note, in mono-sound mode, these effectively control per-channel sound output
	else if(address == NR51)
	{
		memory_map[address] = value;
		apu_stat->channel[0].so1_output = (value & 0x1) ? true : false;
		apu_stat->channel[1].so1_output = (value & 0x2) ? true : false;
		apu_stat->channel[2].so1_output = (value & 0x4) ? true : false;
		apu_stat->channel[3].so1_output = (value & 0x8) ? true : false;
		apu_stat->channel[0].so2_output = (value & 0x10) ? true : false;
		apu_stat->channel[1].so2_output = (value & 0x20) ? true : false;
		apu_stat->channel[2].so2_output = (value & 0x40) ? true : false;
		apu_stat->channel[3].so2_output = (value & 0x80) ? true : false;
	}

	//NR52 Sound On/Off
	else if(address == NR52)
	{
		//Sound on
		if(value & 0x80) 
		{
			memory_map[address] |= 0x80;
			apu_stat->sound_on = true;
		}

		//Sound off
		else
		{
			memory_map[address] &= ~0x80;
			apu_stat->sound_on = false;
		}
	}

	//BGP
	else if(address == REG_BGP)
	{
		memory_map[address] = value;

		//Determine Background/Window Palette - From lightest to darkest
		lcd_stat->bgp[0] = value & 0x3;
		lcd_stat->bgp[1] = (value >> 2) & 0x3;
		lcd_stat->bgp[2] = (value >> 4) & 0x3;
		lcd_stat->bgp[3] = (value >> 6) & 0x3;

		//Update CGFX
		if((cgfx::load_cgfx) || (cgfx::auto_dump_bg))
		{
			for(int x = 0; x < 384; x++)
			{
				cgfx_stat->bg_update_list[x] = true;
			}
			
			cgfx_stat->update_bg = true;
		}
	}

	//OBP0
	else if(address == REG_OBP0)
	{
		memory_map[address] = value;

		//Determine Sprite Palettes - From lightest to darkest
		lcd_stat->obp[0][0] = value  & 0x3;
		lcd_stat->obp[1][0] = (value >> 2) & 0x3;
		lcd_stat->obp[2][0] = (value >> 4) & 0x3;
		lcd_stat->obp[3][0] = (value >> 6) & 0x3;
	}

	//OBP1
	else if(address == REG_OBP1)
	{
		memory_map[address] = value;

		//Determine Sprite Palettes - From lightest to darkest
		lcd_stat->obp[0][1] = value  & 0x3;
		lcd_stat->obp[1][1] = (value >> 2) & 0x3;
		lcd_stat->obp[2][1] = (value >> 4) & 0x3;
		lcd_stat->obp[3][1] = (value >> 6) & 0x3;
	}

	//Current scanline
	else if(address == REG_LY) 
	{ 
		memory_map[REG_LY] = 0;
		lcd_stat->current_scanline = 0;
	}

	//LCDC
	else if(address == REG_LCDC)
	{
		memory_map[address] = value;

		lcd_stat->on_off = lcd_stat->lcd_enable;
		u8 old_size = lcd_stat->obj_size;

		lcd_stat->lcd_control = value;
		lcd_stat->lcd_enable = (value & 0x80) ? true : false;
		lcd_stat->window_map_addr = (value & 0x40) ? 0x9C00 : 0x9800;
		lcd_stat->window_enable = (value & 0x20) ? true : false;
		lcd_stat->bg_tile_addr = (value & 0x10) ? 0x8000 : 0x8800;
		lcd_stat->bg_map_addr = (value & 0x8) ? 0x9C00 : 0x9800;
		lcd_stat->obj_size = (value & 0x4) ? 16 : 8;
		lcd_stat->obj_enable = (value & 0x2) ? true : false;
		lcd_stat->bg_enable = (value & 0x1) ? true : false;

		//Check to see if the LCD was turned off/on while on/off (VBlank only?)
		if(lcd_stat->on_off != lcd_stat->lcd_enable) { lcd_stat->on_off = true; }
		else { lcd_stat->on_off = false; }

		//Update all sprites if the OBJ size changes
		if(old_size != lcd_stat->obj_size)
		{
			lcd_stat->oam_update = true;
		
			for(int x = 0; x < 40; x++) { lcd_stat->oam_update_list[x] = true; }
		}
	}

	//STAT
	else if(address == REG_STAT)
	{
		u8 read_only_bits = (memory_map[REG_STAT] & 0x7);

		memory_map[address] = (value & ~0x7);
		memory_map[address] |= read_only_bits;
	}

	//Scroll Y
	else if(address == REG_SY)
	{
		memory_map[address] = value;
		lcd_stat->bg_scroll_y = value;
	}

	//Scroll X
	else if(address == REG_SX)
	{
		memory_map[address] = value;
		lcd_stat->bg_scroll_x = value;
	}

	//Window Y
	else if(address == REG_WY)
	{
		memory_map[address] = value;
		lcd_stat->window_y = value;
	}

	//Window X
	else if(address == REG_WX)
	{
		memory_map[address] = value;
		lcd_stat->window_x = (value - 7);
	}	

	//DMA transfer
	else if(address == REG_DMA) 
	{
		memory_map[address] = value;
		u16 dma_orig = value << 8;
		u16 dma_dest = 0xFE00;
		while (dma_dest < 0xFEA0) { write_u8(dma_dest++, read_u8(dma_orig++)); }
	}

	//Internal RAM - Write to ECHO RAM as well
	else if((address >= 0xC000) && (address <= 0xDFFF)) 
	{
		//DMG mode - Normal writes
		if(config::gb_type != 2)
		{
			memory_map[address] = value;
			if(address + 0x2000 < 0xFDFF) { memory_map[address + 0x2000] = value; }
		}

		//GBC mode - Use banks
		else if(config::gb_type == 2)
		{
			//Write to Bank 0 always when address is within 0xC000 - 0xCFFF
			if((address >= 0xC000) && (address <= 0xCFFF)) { working_ram_bank[0][address - 0xC000] = value; }
			
			//Write to selected Bank when address is within 0xD000 - 0xDFFF
			else if((address >= 0xD000) && (address <= 0xDFFF)) { working_ram_bank[wram_bank][address - 0xD000] = value; }
		}
	}

	//ECHO RAM - Write to Internal RAM as well
	else if((address >= 0xE000) && (address <= 0xFDFF))
	{
		memory_map[address] = value;
		memory_map[address - 0x2000] = value;
	}

	//OAM - Direct writes
	else if((address >= 0xFE00) && (address < 0xFEA0))
	{
		memory_map[address] = value;
		lcd_stat->oam_update = true;
		lcd_stat->oam_update_list[(address & ~0xFE00) >> 2] = true;
	}

	//P1 - Joypad register
	else if(address == REG_P1) { g_pad->column_id = (value & 0x30); memory_map[REG_P1] = g_pad->read(); }

	//HDMA transfer
	else if(address == REG_HDMA5)
	{
		//Halt Horizontal DMA transfer if one is already in progress and 0 is now written to Bit 7
		if(((value & 0x80) == 0) && (lcd_stat->hdma_in_progress)) 
		{ 
			lcd_stat->hdma_in_progress = false;
			lcd_stat->hdma_current_line = 0;
			value = 0x80;
		}

		//If not halting a current HDMA transfer, start a new one, determine its type
		else 
		{
			lcd_stat->hdma_in_progress = true;
			lcd_stat->hdma_current_line = 0;
			lcd_stat->hdma_type = (value & 0x80) ? 1 : 0;
			value &= ~0x80;
		}

		memory_map[address] = value;
	}

	//VBK - Update VRAM bank
	else if(address == REG_VBK) 
	{ 
		vram_bank = value & 0x1; 
		memory_map[address] = value; 
	}

	//KEY1 - Double-Normal speed switch
	else if(address == REG_KEY1)
	{
		value &= 0x1;
		if(value == 1) { memory_map[address] |= 0x1; }
		else { memory_map[address] &= ~0x1; }
	}

	//BCPD - Update background color palettes
	else if(address == REG_BCPD)
	{
		memory_map[address] = value;
		lcd_stat->update_bg_colors = true;
	}

	//OCPD - Update sprite color palettes
	else if(address == REG_OCPD)
	{
		memory_map[address] = value;
		lcd_stat->update_obj_colors = true;
	}

	//SVBK - Update Working RAM bank
	else if(address == REG_SVBK) 
	{
		wram_bank = value & 0x7;
		if(wram_bank == 0) { wram_bank = 1; }
		memory_map[address] = value;
	}

	else if(address > 0x7FFF) { memory_map[address] = value; }

	//CGFX processing - Check for BG updates
	if((cgfx::load_cgfx || cgfx::auto_dump_bg) && (address >= 0x8000) && (address <= 0x97FF))
	{
		//If the last VRAM value is the same, do not update
		//Some games GBC games will spam VRAM addresses with the same data
		if(previous_value == value) { return; }

		//DMG BG Tile data update
		if(config::gb_type == 1)
		{
			cgfx_stat->bg_update_list[(address & ~0x8000) >> 4] = true;
			cgfx_stat->update_bg = true;
		}

		//GBC BG Tile Data update
		else if(config::gb_type == 2)
		{
			cgfx_stat->update_bg = true;

			u8 tile_number = (address & ~0x8800) >> 4;
			tile_number = (lcd_stat->bg_map_addr == 0x8800) ? lcd_stat->unsigned_tile_lut[tile_number] : tile_number;
			cgfx_stat->bg_tile_update_list[tile_number] = true;
		}
	}
}

/****** Write word to memory ******/
void DMG_MMU::write_u16(u16 address, u16 value)
{
	write_u8(address, (value & 0xFF));
	write_u8((address+1), (value >> 8));
}

/****** Determines which if any MBC to read from ******/
u8 DMG_MMU::mbc_read(u16 address)
{
	switch(cart.mbc_type)
	{
		case MBC1:
			return mbc1_read(address);
			break;

		case MBC2:
			return mbc2_read(address);
			break;

		case MBC3:
			return mbc3_read(address);
			break;

		case MBC5:
			return mbc5_read(address);
			break;
	}
}

/****** Determines which if any MBC to write to ******/
void DMG_MMU::mbc_write(u16 address, u8 value)
{
	switch(cart.mbc_type)
	{
		case MBC1:
			mbc1_write(address, value);
			break;

		case MBC2:
			mbc2_write(address, value);
			break;

		case MBC3:
			mbc3_write(address, value);
			break;

		case MBC5:
			mbc5_write(address, value);
			break;
	}
}

/****** Read binary file to memory ******/
bool DMG_MMU::read_file(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::" << filename << " could not be opened. Check file path or permissions. \n";
		return false;
	}

	u8* ex_mem = &memory_map[0];

	//Read 32KB worth of data from ROM file
	file.read((char*)ex_mem, 0x8000);

	//Determine MBC type
	switch(memory_map[ROM_MBC])
	{
		case 0x0: 
			cart.mbc_type = ROM_ONLY;

			std::cout<<"MMU::Cartridge Type - ROM Only \n";
			break;

		case 0x1:
			cart.mbc_type = MBC1;

			std::cout<<"MMU::Cartridge Type - MBC1 \n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x2: 
			cart.mbc_type = MBC1;
			cart.ram = true;

			std::cout<<"MMU::Cartridge Type - MBC1 + RAM \n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x3:
			cart.mbc_type = MBC1;
			cart.ram = true;
			cart.battery = true;

			std::cout<<"MMU::Cartridge Type - MBC1 + RAM + Battery \n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x5:
			cart.mbc_type = MBC2;
			cart.ram = true;

			std::cout<<"MMU::Cartridge Type - MBC2 \n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x6:
			cart.mbc_type = MBC2;
			cart.ram = true;
			cart.battery = true;

			std::cout<<"MMU::Cartridge Type - MBC2 + Battery\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x8:
			cart.mbc_type = ROM_ONLY;
			cart.ram = true;

			std::cout<<"MMU::Cartridge Type - ROM + RAM\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x10:
			cart.mbc_type = MBC3;
			cart.ram = true;
			cart.battery = true;
			cart.rtc = true;

			std::cout<<"MMU::Cartridge Type - MBC3 + RAM + Battery + Timer\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";

			grab_time();

			break;

		case 0x11:
			cart.mbc_type = MBC3;

			std::cout<<"MMU::Cartridge Type - MBC3\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x12:
			cart.mbc_type = MBC3;
			cart.ram = true;

			std::cout<<"MMU::Cartridge Type - MBC3 + RAM\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x13:
			cart.mbc_type = MBC3;
			cart.ram = true;
			cart.battery = true;

			std::cout<<"MMU::Cartridge Type - MBC3 + RAM + Battery\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x19:
			cart.mbc_type = MBC5;

			std::cout<<"MMU::Cartridge Type - MBC5\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x1A:
			cart.mbc_type = MBC5;
			cart.ram = true;

			std::cout<<"MMU::Cartridge Type - MBC5 + RAM\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x1B:
			cart.mbc_type = MBC5;
			cart.ram = true;
			cart.battery = true;

			std::cout<<"MMU::Cartridge Type - MBC5 + RAM + Battery\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x1C:
			cart.mbc_type = MBC5;

			std::cout<<"MMU::Cartridge Type - MBC5 + Rumble\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;
			
		case 0x1D:
			cart.mbc_type = MBC5;
			cart.ram = true;

			std::cout<<"MMU::Cartridge Type - MBC5 + RAM + Rumble\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		case 0x1E:
			cart.mbc_type = MBC5;
			cart.ram = true;
			cart.battery = true;

			std::cout<<"MMU::Cartridge Type - MBC5 + RAM + Battery + Rumble\n";
			cart.rom_size = 32 << memory_map[ROM_ROMSIZE];
			std::cout<<"MMU::ROM Size - " << cart.rom_size << "KB\n";
			break;

		default:
			std::cout<<"Catridge Type - 0x" << std::hex << (int)memory_map[ROM_MBC] << "\n";
			std::cout<<"MMU::MBC type currently unsupported \n";
			return false;
	}

	//Read additional ROM data to banks
	if(cart.mbc_type != ROM_ONLY)
	{
		//Use a file positioner
		u32 file_pos = 0x8000;
		u8 bank_count = 0;

		while(file_pos < (cart.rom_size * 1024))
		{
			u8* ex_rom = &read_only_bank[bank_count][0];
			file.read((char*)ex_rom, 0x4000);
			file_pos += 0x4000;
			bank_count++;
		}
	}

	file.close();
	std::cout<<"MMU::" << filename << " loaded successfully. \n";

	//Determine if cart is DMG or GBC and which system GBE will try to emulate
	//Only necessary for Auto system detection.
	//For now, even if forcing GBC, when encountering DMG carts, revert to DMG mode, dunno how the palettes work yet
	//When using the DMG bootrom or GBC BIOS, those files determine emulated system type later
	if((config::gb_type == 0) || (config::gb_type == 2))
	{
		//Always use GBC mode when booting from the GBC bootrom
		if((config::gb_type == 2) && (config::use_bios)) { config::gb_type = 2; }

		else if(memory_map[ROM_COLOR] == 0) { config::gb_type = 1; }
		else if(memory_map[ROM_COLOR] == 0x80) { config::gb_type = 2; }
		else if(memory_map[ROM_COLOR] == 0xC0) { config::gb_type = 2; }

		//If another value is present, this is a DMG game
		//The value is likely part of the ASCII title
		else { config::gb_type = 1; }
	}

	//Manually HLE MMIO
	if(!in_bios) 
	{
		write_u8(REG_LCDC, 0x91);
		write_u8(REG_BGP, 0xFC);
		write_u8(REG_P1, 0xFF);
		write_u8(REG_DIV, 0xAF);
		write_u8(REG_TAC, 0xF8);
		write_u8(0xFF10, 0x80);
		write_u8(0xFF11, 0xBF);
   		write_u8(0xFF12, 0xF3); 
  		write_u8(0xFF14, 0xBF); 
   		write_u8(0xFF16, 0x3F); 
   		write_u8(0xFF17, 0x00); 
   		write_u8(0xFF19, 0xBF); 
   		write_u8(0xFF1A, 0x7F); 
   		write_u8(0xFF1B, 0xFF); 
   		write_u8(0xFF1C, 0x9F); 
   		write_u8(0xFF1E, 0xBF); 
   		write_u8(0xFF20, 0xFF); 
   		write_u8(0xFF21, 0x00); 
   		write_u8(0xFF22, 0x00); 
   		write_u8(0xFF23, 0xBF); 
   		write_u8(0xFF24, 0x77); 
   		write_u8(0xFF25, 0xF3);
		write_u8(0xFF26, 0xF1);

		//Some sound registers are set, however, don't actually play sound
		for(int x = 0; x < 4; x++) { apu_stat->channel[x].playing = false; }
	}

	//Manually set some I/O registers
	//Some I/O registers are 0xFF on DMG units, 0x0 on GBC/GBA units
	if(config::gb_type < 2)
	{
		write_u8(REG_OBP0, 0xFF);
		write_u8(REG_OBP1, 0xFF);
	}

	//Load backup save data if applicable
        load_backup(config::save_file);

	return true;
}

/****** Read GB BIOS ******/
bool DMG_MMU::read_bios(std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);

	if(!file.is_open()) 
	{
		std::cout<<"MMU::BIOS file " << filename << " could not be opened. Check file path or permissions. \n";
		return false; 
	}

	//Get BIOS file size
	file.seekg(0, file.end);
	bios_size = file.tellg();
	file.seekg(0, file.beg);

	//Check the file size before reading
	if((bios_size == 0x100) || (bios_size == 0x900))
	{
		//Resize BIOS memory
		bios.resize(bios_size);

		u8* ex_bios = &bios[0];

		//Read BIOS data from file
		file.read((char*)ex_bios, bios_size);
		file.close();

		//When using the BIOS, set the emulated system type - DMG or GBC respectively
		if(bios_size == 0x100) { config::gb_type = 1; }
		else if(bios_size == 0x900) { config::gb_type = 2; }

		std::cout<<"MMU::BIOS file " << filename << " loaded successfully. \n";

		return true;
	}

	else
	{
		std::cout<<"MMU::BIOS file " << filename << " has an incorrect file size : (" << bios_size << " bytes) \n";
		return false;
	}	
}

/****** Load backup save data ******/
bool DMG_MMU::load_backup(std::string filename)
{
	//Load Saved RAM if available
	if(cart.battery)
	{
		std::ifstream sram(filename.c_str(), std::ios::binary);

		if(!sram.is_open()) 
		{ 
			std::cout<<"MMU::" << filename << " save data could not be opened. Check file path or permissions. \n";
			return false;
		}

		else 
		{
			for(int x = 0; x < 0x10; x++)
			{
				u8* ex_ram = &random_access_bank[x][0];
				sram.read((char*)ex_ram, 0x2000); 
			}
		}

		sram.close();
	
		std::cout<<"MMU::Loaded save data file " << filename <<  "\n";
	}

	return true;
}

/****** Save backup save data ******/
bool DMG_MMU::save_backup(std::string filename)
{
	if(cart.battery)
	{
		std::ofstream sram(filename.c_str(), std::ios::binary);

		if(!sram.is_open()) 
		{ 
			std::cout<<"MMU::" << filename << " save data could not be written. Check file path or permissions. \n";
			return false;
		}

		else 
		{
			for(int x = 0; x < 0x10; x++)
			{
				sram.write(reinterpret_cast<char*> (&random_access_bank[x][0]), 0x2000); 
			}

			//Add RTC data
			if(cart.rtc) 
			{
				grab_time();
			
				for(int x = 0; x < 5; x++)
				{
					sram.write(reinterpret_cast<char*> (&cart.rtc_reg[x]), 0x1);
				}
			} 

			sram.close();

			std::cout<<"MMU::Wrote save data file " << filename <<  "\n";
		}
	}

	return true;
}

/****** Points the MMU to an lcd_data structure (FROM THE LCD ITSELF) ******/
void DMG_MMU::set_lcd_data(dmg_lcd_data* ex_lcd_stat) { lcd_stat = ex_lcd_stat; }

/****** Points the MMU to an cgfx_data structure (FROM THE LCD ITSELF) ******/
void DMG_MMU::set_cgfx_data(dmg_cgfx_data* ex_cgfx_stat) { cgfx_stat = ex_cgfx_stat; }

/****** Points the MMU to an apu_data structure (FROM THE APU ITSELF) ******/
void DMG_MMU::set_apu_data(dmg_apu_data* ex_apu_stat) { apu_stat = ex_apu_stat; }
