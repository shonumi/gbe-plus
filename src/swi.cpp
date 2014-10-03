// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : swi.cpp
// Date : October 2, 2014
// Description : GBA ARM7 Software Interrupts
//
// Emulates the GBA's Software Interrupts via High Level Emulation

#include "arm7.h"

/****** Process Software Interrupts ******/
void ARM7::process_swi(u8 comment)
{
	switch(comment)
	{
		//SoftReset
		case 0x0:
			std::cout<<"SWI::Soft Reset (not implemented yet) \n";
			break;

		//RegisterRAMReset
		case 0x1:
			std::cout<<"SWI::Register RAM Reset (not implemented yet) \n";
			break;

		//Halt
		case 0x2:
			std::cout<<"SWI::Halt (not implemented yet) \n";
			break;

		//Stop-Sleep
		case 0x3:
			std::cout<<"SWI::Stop-Sleep (not implemented yet) \n";
			break;

		//IntrWait
		case 0x4:
			std::cout<<"SWI::Interrupt Wait (not implemented yet) \n";
			break;

		//VBlankIntrWait
		case 0x5:
			std::cout<<"SWI::VBlank Interrupt Wait (not implemented yet) \n";
			break;

		//Div
		case 0x6:
			std::cout<<"SWI::Divide (not implemented yet) \n";
			break;

		//DivARM
		case 0x7:
			std::cout<<"SWI::Divide ARM (not implemented yet) \n";
			break;

		//Sqrt
		case 0x8:
			std::cout<<"SWI::Square Root (not implemented yet) \n";
			break;

		//ArcTan
		case 0x9:
			std::cout<<"SWI::ArcTan (not implemented yet) \n";
			break;

		//ArcTan2
		case 0xA:
			std::cout<<"SWI::ArcTan2 (not implemented yet) \n";
			break;

		//CPUSet
		case 0xB:
			std::cout<<"SWI::CPU Set (not implemented yet) \n";
			break;

		//CPUFastSet
		case 0xC:
			std::cout<<"SWI::CPU Fast Set (not implemented yet) \n";
			break;

		//GetBIOSChecksum
		case 0xD:
			std::cout<<"SWI::Get BIOS Checksum (not implemented yet) \n";
			break;

		//BGAffineSet
		case 0xE:
			std::cout<<"SWI::BG Affine Set (not implemented yet) \n";
			break;

		//OBJAffineSet
		case 0xF:
			std::cout<<"SWI::OBJ Affine Set (not implemented yet) \n";
			break;

		//BitUnPack
		case 0x10:
			std::cout<<"SWI::Bit Unpack (not implemented yet) \n";
			break;

		//LZ77UnCompWram
		case 0x11:
			std::cout<<"SWI::LZ77 Uncompress Work RAM (not implemented yet) \n";
			break;

		//LZ77UnCompVram
		case 0x12:
			std::cout<<"SWI::LZ77 Uncompress Video RAM (not implemented yet) \n";
			break;

		//HuffUnComp
		case 0x13:
			std::cout<<"SWI::Huffman Uncompress (not implemented yet) \n";
			break;

		//RLUnCompWram
		case 0x14:
			std::cout<<"SWI::Run Length Uncompress Work RAM (not implemented yet) \n";
			break;

		//RLUnCompVram
		case 0x15:
			std::cout<<"SWI::Run Length Uncompress Video RAM (not implemented yet) \n";
			break;

		//Diff8bitUnFilterWram
		case 0x16:
			std::cout<<"SWI::Diff8bitUnFilterWram (not implemented yet) \n";
			break;

		//Diff8bitUnFilterVram
		case 0x17:
			std::cout<<"SWI::Diff8bitUnFilterVram (not implemented yet) \n";
			break;

		//Diff16bitUnFilter
		case 0x18:
			std::cout<<"SWI::Diff16bitUnFilter (not implemented yet) \n";
			break;

		//SoundBias
		case 0x19:
			std::cout<<"SWI::Sound Bias (not implemented yet) \n";
			break;

		//SoundDriverInit
		case 0x1A:
			std::cout<<"SWI::Sound Driver Init (not implemented yet) \n";
			break;

		//SoundDriverMode
		case 0x1B:
			std::cout<<"SWI::Sound Driver Mode (not implemented yet) \n";
			break;

		//SoundDriverMain
		case 0x1C:
			std::cout<<"SWI::Sound Driver Main (not implemented yet) \n";
			break;

		//SoundDriverVSync
		case 0x1D:
			std::cout<<"SWI::Sound Driver VSync (not implemented yet) \n";
			break;

		//SoundChannelClear
		case 0x1E:
			std::cout<<"SWI::Sound Channel Clear (not implemented yet) \n";
			break;

		//MidiKey2Freq
		case 0x1F:
			std::cout<<"SWI::Midi Key to Frequency (not implemented yet) \n";
			break;

		//Undocumented Sound Function 0
		case 0x20:
			std::cout<<"SWI::Undocumented Sound Function 0 \n";
			break;

		//Undocumented Sound Function 1
		case 0x21:
			std::cout<<"SWI::Undocumented Sound Function 1 \n";
			break;

		//Undocumented Sound Function 2
		case 0x22:
			std::cout<<"SWI::Undocumented Sound Function 2 \n";
			break;

		//Undocumented Sound Function 3
		case 0x23:
			std::cout<<"SWI::Undocumented Sound Function 3 \n";
			break;

		//Undocumented Sound Function 4
		case 0x24:
			std::cout<<"SWI::Undocumented Sound Function 4 \n";
			break;

		//Multiboot
		case 0x25:
			std::cout<<"SWI::Multiboot (not implemented yet) \n";
			break;

		//HardReset
		case 0x26:
			std::cout<<"SWI::Hard Reset (not implemented yet) \n";
			break;

		//CustomHalt
		case 0x27:
			std::cout<<"SWI::Custom Halt (not implemented yet) \n";
			break;

		//SoundDriverVSyncOff
		case 0x28:
			std::cout<<"SWI::Sound Driver VSync Off (not implemented yet) \n";
			break;

		//SoundDriverVSyncOn
		case 0x29:
			std::cout<<"SWI::Sound Driver VSync On (not implemented yet) \n";
			break;

		//SoundGetJumpList
		case 0x2A:
			std::cout<<"SWI::Sound Get Jump List (not implemented yet) \n";
			break;

		default:
			std::cout<<"SWI::Error - Unknown BIOS function 0x" << std::hex << (int)comment << "\n";
			break;
	}
} 