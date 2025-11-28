// GB Enhanced+ Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : microphone.cpp
// Date : September 9, 2025
// Description : NDS Microphone Device Emulation
//
// Handles output of various microphone devices for the NDS

#include "mmu.h"
#include "common/util.h"

void NTR_MMU::wantame_scanner_process()
{
	if(wcs.data.empty())
	{
		apu_stat->mic_out = 0;
	}

	else
	{
		if(wcs.index >= wcs.data.size())
		{
			wcs.index = 0;
			wcs.data.clear();
			apu_stat->mic_out = 0;

			return;
		}

		apu_stat->mic_out = wcs.data[wcs.index++];
	}
}

void NTR_MMU::wantame_scanner_set_barcode()
{
	//Use latest barcode (e.g. from GUI)
	if(!config::raw_barcode.empty()) { wcs.barcode = config::raw_barcode; }

	wcs.data.clear();
	std::string barcode_hi = "";
	std::string barcode_lo = "";

	u64 sum = 0;
	u32 sum_shift = 35;
	u16 check_digit = 0x69;

	u16 barcode_msb = 0;
	u32 barcode_lsb = 0;

	//Verify Code-128 C barcode length. *ALWAYS* Fixed 12-chars for Wantame
	if(wcs.barcode.length() != 12)
	{
		return;
	}

	//Generate initial ACK signal + 1st 6 characters
	wantame_scanner_set_pulse(10, 10);
	wantame_scanner_set_pulse(10, 10);

	//Convert barcode characters to hexadecimal format + calculate check digit
	//Code-128 C uses pairs, i.e. 2 characters of barcode -> 1 value
	for(u32 x = 0, index = 0; x < 6; x++)
	{
		std::string bc_str = "";
		bc_str += wcs.barcode[index++];
		bc_str += wcs.barcode[index++];

		u32 code_val = 0;
		util::from_str(bc_str, code_val);

		u64 temp = 0;
		temp = code_val;
		temp <<= sum_shift;

		sum += temp;
		sum_shift -= 7;

		check_digit += (code_val * (x + 1));
	}

	check_digit = (check_digit % 0x67);
	barcode_msb = (sum >> 32);
	barcode_lsb = (sum & 0xFFFFFFFF);

	for(u32 x = 0, bit = 0x200; x < 10; x++)
	{
		if(barcode_msb & bit) { wantame_scanner_set_pulse(6, 3); }
		else { wantame_scanner_set_pulse(3, 6); }

		bit >>= 1;
	}

	for(u32 x = 0, bit = 0x80000000; x < 32; x++)
	{
		if(barcode_lsb & bit) { wantame_scanner_set_pulse(6, 3); }
		else { wantame_scanner_set_pulse(3, 6); }

		bit >>= 1;
	}

	for(u32 x = 0, bit = 0x40; x < 7; x++)
	{
		if(check_digit & bit) { wantame_scanner_set_pulse(6, 3); }
		else { wantame_scanner_set_pulse(3, 6); }

		bit >>= 1;
	}
}

void NTR_MMU::wantame_scanner_set_pulse(u32 lo, u32 hi)
{
	for(u32 x = 0; x < lo; x++) { wcs.data.push_back(0x00); }
	for(u32 x = 0; x < hi; x++) { wcs.data.push_back(0x24); }
}

bool NTR_MMU::wantame_scanner_load_barcode(std::string filename)
{
	std::ifstream barcode(filename.c_str(), std::ios::binary);

	if(!barcode.is_open()) 
	{ 
		std::cout<<"MMU::Wantame barcode data could not be read. Check file path or permissions. \n";
		return false;
	}

	//Get file size
	barcode.seekg(0, barcode.end);
	u32 barcode_size = barcode.tellg();
	barcode.seekg(0, barcode.beg);

	if(barcode_size != 12)
	{
		std::cout<<"MMU::Wantame barcode data file is incorrect size. \n";
		return false;
	}

	char ex_data[12];

	barcode.read((char*)ex_data, barcode_size); 
	barcode.close();

	wcs.barcode.assign(ex_data, 12);
	config::raw_barcode = wcs.barcode;
	std::cout<<"MMU::Loaded Wantame barcode data.\n";
	
	return true;
}

void NTR_MMU::wave_scanner_process()
{
	if(wave_scanner.data.empty())
	{
		apu_stat->mic_out = 0;
	}

	else
	{
		if(wave_scanner.index >= wave_scanner.data.size())
		{
			wave_scanner.index = 0;
			wave_scanner.data.clear();
			apu_stat->mic_out = 0;

			return;
		}

		apu_stat->mic_out = wave_scanner.data[wave_scanner.index++];
	}
}

void NTR_MMU::wave_scanner_set_data()
{
	wave_scanner.data.clear();

	//Generate initial ACK signal + 1st 6 characters
	wave_scanner_set_pulse(10, 10);

	u32 my_data = 0x4241292A;
	u32 bit = 0x80000000;

	for(u32 x = 0; x < 32; x++)
	{
		if(my_data & bit) { wave_scanner_set_pulse(6, 8); }
		else { wave_scanner_set_pulse(3, 8); }

		bit >>= 1;
	}
}

void NTR_MMU::wave_scanner_set_pulse(u32 hi, u32 lo)
{
	for(u32 x = 0; x < hi; x++) { wave_scanner.data.push_back(0x58); }
	for(u32 x = 0; x < lo; x++) { wave_scanner.data.push_back(0x00); }
}
