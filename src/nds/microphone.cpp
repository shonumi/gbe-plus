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

	apu_stat->mic_out = wcs.data[wcs.index++];

	if(wcs.index < wcs.data.size())
	{
		wcs.index = 0;
	} 
}

void NTR_MMU::wantame_scanner_set_barcode(u32 barcode)
{
	wcs.barcode = barcode;
}

void NTR_MMU::wantame_scanner_set_pulse(u32 lo, u32 hi)
{
	for(u32 x = 0; x < lo; x++) { wcs.data.push_back(0x00); }
	for(u32 x = 0; x < hi; x++) { wcs.data.push_back(0x24); }
}
