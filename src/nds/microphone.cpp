// GB Enhanced+ Copyright Daniel Baxter 2025
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : microphone.cpp
// Date : September 9, 2025
// Description : NDS Microphone Device Emulation
//
// Handles output of various microphone devices for the NDS

#include "mmu.h" 

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

	else
	{
		barcode_hi = wcs.barcode.substr(0, 6);
		barcode_lo = wcs.barcode.substr(6);
	}

	//Generate initial ACK signal + 1st 6 characters
	wantame_scanner_set_pulse(10, 10);
	wantame_scanner_set_pulse(10, 10);

	//Convert barcode characters to hexadecimal format + calculate check digit
	for(u32 x = 0; x < 6; x++)
	{
		u8 code_val = barcode_hi[x];
		u64 temp = 0;

		code_val -= 0x20;		
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
