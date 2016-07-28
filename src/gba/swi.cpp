// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : swi.cpp
// Date : October 2, 2014
// Description : GBA ARM7 Software Interrupts
//
// Emulates the GBA's Software Interrupts via High Level Emulation

#include <cmath>

#include "arm7.h"

s16 sine_lut[256] = 
{
  0x0000, 0x0192, 0x0323, 0x04B5, 0x0645, 0x07D5, 0x0964, 0x0AF1,
  0x0C7C, 0x0E05, 0x0F8C, 0x1111, 0x1294, 0x1413, 0x158F, 0x1708,
  0x187D, 0x19EF, 0x1B5D, 0x1CC6, 0x1E2B, 0x1F8B, 0x20E7, 0x223D,
  0x238E, 0x24DA, 0x261F, 0x275F, 0x2899, 0x29CD, 0x2AFA, 0x2C21,
  0x2D41, 0x2E5A, 0x2F6B, 0x3076, 0x3179, 0x3274, 0x3367, 0x3453,
  0x3536, 0x3612, 0x36E5, 0x37AF, 0x3871, 0x392A, 0x39DA, 0x3A82,
  0x3B20, 0x3BB6, 0x3C42, 0x3CC5, 0x3D3E, 0x3DAE, 0x3E14, 0x3E71,
  0x3EC5, 0x3F0E, 0x3F4E, 0x3F84, 0x3FB1, 0x3FD3, 0x3FEC, 0x3FFB,
  0x4000, 0x3FFB, 0x3FEC, 0x3FD3, 0x3FB1, 0x3F84, 0x3F4E, 0x3F0E,
  0x3EC5, 0x3E71, 0x3E14, 0x3DAE, 0x3D3E, 0x3CC5, 0x3C42, 0x3BB6,
  0x3B20, 0x3A82, 0x39DA, 0x392A, 0x3871, 0x37AF, 0x36E5, 0x3612,
  0x3536, 0x3453, 0x3367, 0x3274, 0x3179, 0x3076, 0x2F6B, 0x2E5A,
  0x2D41, 0x2C21, 0x2AFA, 0x29CD, 0x2899, 0x275F, 0x261F, 0x24DA,
  0x238E, 0x223D, 0x20E7, 0x1F8B, 0x1E2B, 0x1CC6, 0x1B5D, 0x19EF,
  0x187D, 0x1708, 0x158F, 0x1413, 0x1294, 0x1111, 0x0F8C, 0x0E05,
  0x0C7C, 0x0AF1, 0x0964, 0x07D5, 0x0645, 0x04B5, 0x0323, 0x0192,
  0x0000, 0xFE6E, 0xFCDD, 0xFB4B, 0xF9BB, 0xF82B, 0xF69C, 0xF50F,
  0xF384, 0xF1FB, 0xF074, 0xEEEF, 0xED6C, 0xEBED, 0xEA71, 0xE8F8,
  0xE783, 0xE611, 0xE4A3, 0xE33A, 0xE1D5, 0xE075, 0xDF19, 0xDDC3,
  0xDC72, 0xDB26, 0xD9E1, 0xD8A1, 0xD767, 0xD633, 0xD506, 0xD3DF,
  0xD2BF, 0xD1A6, 0xD095, 0xCF8A, 0xCE87, 0xCD8C, 0xCC99, 0xCBAD,
  0xCACA, 0xC9EE, 0xC91B, 0xC851, 0xC78F, 0xC6D6, 0xC626, 0xC57E,
  0xC4E0, 0xC44A, 0xC3BE, 0xC33B, 0xC2C2, 0xC252, 0xC1EC, 0xC18F,
  0xC13B, 0xC0F2, 0xC0B2, 0xC07C, 0xC04F, 0xC02D, 0xC014, 0xC005,
  0xC000, 0xC005, 0xC014, 0xC02D, 0xC04F, 0xC07C, 0xC0B2, 0xC0F2,
  0xC13B, 0xC18F, 0xC1EC, 0xC252, 0xC2C2, 0xC33B, 0xC3BE, 0xC44A,
  0xC4E0, 0xC57E, 0xC626, 0xC6D6, 0xC78F, 0xC851, 0xC91B, 0xC9EE,
  0xCACA, 0xCBAD, 0xCC99, 0xCD8C, 0xCE87, 0xCF8A, 0xD095, 0xD1A6,
  0xD2BF, 0xD3DF, 0xD506, 0xD633, 0xD767, 0xD8A1, 0xD9E1, 0xDB26,
  0xDC72, 0xDDC3, 0xDF19, 0xE075, 0xE1D5, 0xE33A, 0xE4A3, 0xE611,
  0xE783, 0xE8F8, 0xEA71, 0xEBED, 0xED6C, 0xEEEF, 0xF074, 0xF1FB,
  0xF384, 0xF50F, 0xF69C, 0xF82B, 0xF9BB, 0xFB4B, 0xFCDD, 0xFE6E
};

/****** Process Software Interrupts ******/
void ARM7::process_swi(u32 comment)
{
	switch(comment)
	{
		//SoftReset
		case 0x0:
			swi_softreset();
			std::cout<<"SWI::Soft Reset \n";
			break;

		//RegisterRAMReset
		case 0x1:
			std::cout<<"SWI::Register RAM Reset \n";
			swi_registerramreset();
			break;

		//Halt
		case 0x2:
			//std::cout<<"SWI::Halt \n";
			swi_halt();
			break;

		//Stop-Sleep
		case 0x3:
			std::cout<<"SWI::Stop-Sleep \n";
			swi_sleep();
			break;

		//IntrWait
		case 0x4:
			std::cout<<"SWI::Interrupt Wait \n";
			break;

		//VBlankIntrWait
		case 0x5:
			//std::cout<<"SWI::VBlank Interrupt Wait \n";
			swi_vblankintrwait();
			break;

		//Div
		case 0x6:
			//std::cout<<"SWI::Divide \n";
			swi_div();
			break;

		//DivARM
		case 0x7:
			std::cout<<"SWI::Divide ARM \n";
			swi_divarm();
			break;

		//Sqrt
		case 0x8:
			std::cout<<"SWI::Square Root \n";
			swi_sqrt();
			break;

		//ArcTan
		case 0x9:
			std::cout<<"SWI::ArcTan \n";
			swi_arctan();
			break;

		//ArcTan2
		case 0xA:
			std::cout<<"SWI::ArcTan2 \n";
			swi_arctan2();
			break;

		//CPUSet
		case 0xB:
			std::cout<<"SWI::CPU Set \n";
			swi_cpuset();
			break;

		//CPUFastSet
		case 0xC:
			std::cout<<"SWI::CPU Fast Set \n";
			swi_cpufastset();
			break;

		//GetBIOSChecksum
		case 0xD:
			std::cout<<"SWI::Get BIOS Checksum \n";
			swi_getbioschecksum();
			break;

		//BGAffineSet
		case 0xE:
			std::cout<<"SWI::BG Affine Set \n";
			swi_bgaffineset();
			break;

		//OBJAffineSet
		case 0xF:
			std::cout<<"SWI::OBJ Affine Set \n";
			swi_objaffineset();
			break;

		//BitUnPack
		case 0x10:
			std::cout<<"SWI::BitUnpack \n";
			swi_bitunpack();
			break;

		//LZ77UnCompWram
		case 0x11:
			std::cout<<"SWI::LZ77 Uncompress Work RAM \n";
			swi_lz77uncompvram();
			break;

		//LZ77UnCompVram
		case 0x12:
			std::cout<<"SWI::LZ77 Uncompress Video RAM \n";
			swi_lz77uncompvram();
			break;

		//HuffUnComp
		case 0x13:
			std::cout<<"SWI::Huffman Uncompress \n";
			swi_huffuncomp();
			break;

		//RLUnCompWram
		case 0x14:
			std::cout<<"SWI::Run Length Uncompress Work RAM \n";
			swi_rluncompvram();
			break;

		//RLUnCompVram
		case 0x15:
			std::cout<<"SWI::Run Length Uncompress Video RAM \n";
			swi_rluncompvram();
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
			//std::cout<<"SWI::Midi Key to Frequency \n";
			swi_midikey2freq();
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
			std::cout<<"SWI::Error - Unknown BIOS function 0x" << std::hex << comment << "\n";
			break;
	}
}

/****** HLE implementation of SoftReset ******/
void ARM7::swi_softreset()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Reset IRQ, SVC, and SYS stack pointers
	reg.r13_svc = 0x03007FE0;
	reg.r13_irq = 0x03007FA0;
	reg.r13 = 0x03007F00;

	//Set PC to start of GamePak ROM or 256KB WRAM
	u8 flag = mem->read_u8(0x3007FFA);
	if(flag == 0) { reg.r15 = 0x8000000; }
	else { reg.r15 = 0x2000000; }
	needs_flush = true;

	//Set registers R0-R12 to zero
	for(int x = 0; x <= 12; x++) { set_reg(x, 0); }

	//Set R14_svc, R14_irq to zero, R14 to the return address
	reg.r14_svc = 0;
	reg.r14_irq = 0;

	//Set SPSR_svc and SPSR_irq to zero
	reg.spsr_svc = 0;
	reg.spsr_irq = 0;

	//Set mode to SYS
	current_cpu_mode = SYS;
	reg.cpsr &= ~0x1F;
	reg.cpsr |= 0x1F;

	//Clear top 0x200 bytes of the 32KB WRAM
	for(int x = 0x3007E00; x < 0x3008000; x++) { mem->memory_map[x] = 0; }

	arm_mode = ARM;
	in_interrupt = false;

	//Clear internal input
	mem->g_pad->clear_input();
}

/****** HLE implementation of RegisterRAMReset ******/
void ARM7::swi_registerramreset()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab reset flags - R0
	u8 reset_flags = (get_reg(0) & 0xFF);
	u32 x = 0;

	//Clear 256K WRAM
	if(reset_flags & 0x1)
	{
		for(x = 0x2000000; x < 0x2040000; x++) { mem->write_u8(x, 0x0); }
	}

	//Clear 32KB WRAM, excluding the stack
	if(reset_flags & 0x2)
	{
		for(x = 0x3000000; x < 0x3007E00; x++) { mem->write_u8(x, 0x0); }
	}

	//Clear Palette VRAM
	if(reset_flags & 0x4)
	{
	 	for(x = 0x5000000; x < 0x5000400; x++) { mem->write_u8(x, 0x0); }
	}

	//Clear VRAM
	if(reset_flags & 0x8)
	{
		for(x = 0x6000000; x < 0x6018000; x++) { mem->write_u8(x, 0x0); }
	}

	//Clear OAM
	if(reset_flags & 0x10)
	{
		for(x = 0x7000000; x < 0x7000400; x++) { mem->write_u8(x, 0x0); }
	}

	//Reset SIO
	if(reset_flags & 0x20)
	{
		mem->write_u16(0x4000134, 0x8000);
		for(x = 0x4000136; x <  0x400015A; x++) { mem->write_u8(x, 0x0); }
	}

	//Reset Sound Registers
	if(reset_flags & 0x40)
	{
		for(x = 0x4000060; x < 0x40000B0; x++) { mem->write_u8(x, 0x0); }
	}

	//Reset all other registers (mainly display and timer registers?)
	if(reset_flags & 0x80)
	{
		for(x = 0x4000000; x < 0x4000060; x++) { mem->write_u8(x, 0x0); }
		for(x = 0x4000100; x < 0x4000110; x++) { mem->write_u8(x, 0x0); }
		mem->write_u16(DISPCNT, 0x80);
	}
}

/****** HLE implementation of Halt ******/
void ARM7::swi_halt()
{
	bios_read_state = BIOS_SWI_FINISH;

	u16 if_check, ie_check = 0; 
	bool halt = true;

	//Run controllers until an interrupt happens
	while(halt)
	{
		clock();

		if_check = mem->read_u16(REG_IF);
		ie_check = mem->read_u16(REG_IE);

		//Match up bits in IE and IF
		for(int x = 0; x < 14; x++)
		{
			//When there is a match, exit HALT mode
			if((ie_check & (1 << x)) && (if_check & (1 << x))) { halt = false; }
		}
	}
}

/****** HLE implementation of Sleep ******/
void ARM7::swi_sleep()
{
	bios_read_state = BIOS_SWI_FINISH;
	sleep = true;
	running = false;
}

/****** HLE implementation of Div ******/
void ARM7::swi_div()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab the numerator - R0
	s32 num = get_reg(0);
	
	//Grab the denominator - R1
	s32 den = get_reg(1);

	s32 result = 0;
	s32 modulo = 0;

	//Do NOT divide by 0
	if(den == 0) { std::cout<<"SWI::Warning - Div tried to divide by zero (ignoring operation) \n"; return; }

	//R0 = result of division
	result = num/den;
	set_reg(0, result);

	//R1 = mod of inputs
	modulo = num % den;
	set_reg(1, modulo);

	//R3 = absolute value of division
	if(result < 0) { result *= -1; }
	set_reg(3, result);
}

/****** HLE implementation of DivARM ******/
void ARM7::swi_divarm()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab the numerator - R1
	s32 num = get_reg(1);
	
	//Grab the denominator - R0
	s32 den = get_reg(0);

	s32 result = 0;
	s32 modulo = 0;

	//Do NOT divide by 0
	if(den == 0) { std::cout<<"SWI::Warning - DivARM tried to divide by zero (ignoring operation) \n"; return; }

	//R0 = result of division
	result = num/den;
	set_reg(0, result);

	//R1 = mod of inputs
	modulo = num % den;
	set_reg(1, modulo);

	//R3 = absolute value of division
	if(result < 0) { result *= -1; }
	set_reg(3, result);
}

/****** HLE implementation of Sqrt ******/
void ARM7::swi_sqrt()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab input
	u32 input = get_reg(0);

	//Set result of operation
	u16 result = sqrt(input);
	set_reg(0, result);
}

/****** HLE implementation of ArcTan ******/
void ARM7::swi_arctan()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab the tangent - R0
	s32 tan = get_reg(0);

	s32 a =  -((tan * tan) >> 14);
	s32 b = ((0xA9 * a) >> 14) + 0x390;
	b = ((b * a) >> 14) + 0x91C;
	b = ((b * a) >> 14) + 0xFB6;
	b = ((b * a) >> 14) + 0x16AA;
	b = ((b * a) >> 14) + 0x2081;
	b = ((b * a) >> 14) + 0x3651;
	b = ((b * a) >> 14) + 0xA2F9;
	a = (b * tan) >> 16;

	//Return arctangent
	set_reg(0, a);
}

/****** HLE implementation of ArcTan2 ******/
void ARM7::swi_arctan2()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab X - R0
	s32 x = get_reg(0);

	//Grab Y - R1
	s32 y = get_reg(1);

	u32 result = 0;

	if (y == 0) { result = ((x >> 16) & 0x8000); }
	
	else 
	{
		if (x == 0) { result = ((y >> 16) & 0x8000) + 0x4000; } 

		else 
		{
			if ((abs(x) > abs(y)) || ((abs(x) == abs(y)) && (!((x < 0) && (y < 0))))) 
			{
        			set_reg(1, x);
        			set_reg(0, (y << 14));
		
        			swi_div();
        			swi_arctan();

        			if (x < 0) { result = 0x8000 + get_reg(0); }
				else { result = (((y >> 16) & 0x8000) << 1) + get_reg(0); }
			}

			else 
			{
        			set_reg(0, (x << 14));
	
        			swi_div();
        			swi_arctan();

        			result = (0x4000 + ((y >> 16) & 0x8000)) - get_reg(0);
			}
    		}
  	}
	
	//Return corrected arctangent
  	set_reg(0, result);
}

/****** HLE implementation of CPUFastSet ******/
void ARM7::swi_cpufastset()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	src_addr &= ~0x3;
	dest_addr &= ~0x3;

	//Abort read/writes to the BIOS
	if(src_addr <= 0x3FFF) { return; }
	if(dest_addr <= 0x3FFF) { return; }

	//Grab transfer control options - R2
	u32 transfer_control = get_reg(2);

	//Transfer size - Bits 0-20 of R2
	u32 transfer_size = (transfer_control & 0x1FFFFF);

	//Determine if the transfer operation is copy or fill - Bit 24 of R2
	u8 copy_fill = (transfer_control & 0x1000000) ? 1 : 0;

	u32 temp = 0;

	while(transfer_size != 0)
	{
		//Copy from source to destination
		if(copy_fill == 0)
		{
			temp = mem->read_u32(src_addr);
			mem->write_u32(dest_addr, temp);
			
			src_addr += 4;
			dest_addr += 4;

			transfer_size--;
		}

		//Fill first entry from source with destination
		else
		{
			temp = mem->read_u32(src_addr);
			mem->write_u32(dest_addr, temp);
			
			dest_addr += 4;
			
			transfer_size--;
		}
	}

	//Write-back R0, R1
	set_reg(0, src_addr);
	set_reg(1, dest_addr);
}

/****** HLE implementation of CPUSet ******/
void ARM7::swi_cpuset()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Abort read/writes to the BIOS
	if(src_addr <= 0x3FFF) { return; }
	if(dest_addr <= 0x3FFF) { return; }

	//Grab transfer control options - R2
	u32 transfer_control = get_reg(2);

	//Transfer size - Bits 0-20 of R2
	u32 transfer_size = (transfer_control & 0x1FFFFF);

	//Determine if the transfer operation is copy or fill - Bit 24 of R2
	u8 copy_fill = (transfer_control & 0x1000000) ? 1 : 0;

	//Determine if the transfer operation is 16 or 32-bit - Bit 26 of R2
	u8 transfer_type = (transfer_control & 0x4000000) ? 1 : 0;

	src_addr &= (transfer_type == 0) ? ~0x1 : ~0x3;
	dest_addr &= (transfer_type == 0) ? ~0x1 : ~0x3; 

	u32 temp_32 = 0;
	u16 temp_16 = 0;

	while(transfer_size != 0)
	{
		//Copy from source to destination
		if(copy_fill == 0)
		{
			//16-bit transfer
			if(transfer_type == 0)
			{
				temp_16 = mem->read_u16(src_addr);
				mem->write_u16(dest_addr, temp_16);
			
				src_addr += 2;
				dest_addr += 2;
			}

			//32-bit transfer
			else
			{
				temp_32 = mem->read_u32(src_addr);
				mem->write_u32(dest_addr, temp_32);
			
				src_addr += 4;
				dest_addr += 4;
			}

			transfer_size--;
		}

		//Fill first entry from source with destination
		else
		{
			//16-bit transfer
			if(transfer_type == 0)
			{
				temp_16 = mem->read_u16(src_addr);
				mem->write_u16(dest_addr, temp_16);
			
				dest_addr += 2;
			}

			//32-bit transfer
			else
			{
				temp_32 = mem->read_u32(src_addr);
				mem->write_u32(dest_addr, temp_32);
			
				dest_addr += 4;
			}
			
			transfer_size--;
		}
	}

	//Write-back R0, R1
	set_reg(0, src_addr);
	set_reg(1, dest_addr);
}

/****** HLE implementation of IntrWait ******/
void ARM7::swi_intrwait()
{
	//Force IME on, Force IRQ bit in CPSR
	mem->write_u16(REG_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	u16 if_check, ie_check, old_if, current_if = 0;
	bool fire_interrupt = false;

	//Grab old IF, set current one to zero
	old_if = mem->read_u16_fast(REG_IF);
	mem->write_u16_fast(REG_IF, 0x0);

	//Grab the interrupts to check from R1
	if_check = reg.r1;

	//If R0 == 0, exit the SWI immediately if one of the IF flags to check is already set
	if((reg.r0 == 0) && (old_if & if_check)) { fire_interrupt = true; }

	//Run controllers until an interrupt is generated
	while(!fire_interrupt)
	{
		clock();

		current_if = mem->read_u16_fast(REG_IF);
		ie_check = mem->read_u16_fast(REG_IE);
		
		//Match up bits in IE and IF
		for(int x = 0; x < 14; x++)
		{
			//When there is a match check to see if IntrWait can quit
			if((ie_check & (1 << x)) && (if_check & (1 << x)))
			{	
				//If R0 == 1, quit when the IF flags match the ones specified in R1
				if(current_if & if_check) { fire_interrupt = true; }
			}
		}
	}

	//Restore old IF, also OR in any new flags that were set
	mem->write_u16_fast(REG_IF, old_if | current_if);

	//Artificially hold PC at current location
	//This SWI will be fetched, decoded, and executed again until it hits VBlank 
	reg.r15 -= (arm_mode == ARM) ? 4 : 2;
}

/****** HLE implementation of VBlankIntrWait ******/
void ARM7::swi_vblankintrwait()
{
	bios_read_state = BIOS_SWI_FINISH;

	swi_vblank_wait = true;

	//Set R0 and R1 to 1
	set_reg(0, 1);
	set_reg(1, 1);

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u16(REG_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	u16 if_check, ie_check = 0;
	bool fire_interrupt = false;
	bool is_vblank = false;

	if_check = mem->read_u16_fast(REG_IF);
	is_vblank = (if_check & 0x1) ? true : false;

	//Run controllers until an interrupt is generated
	while(!fire_interrupt && !is_vblank)
	{
		clock();

		if_check = mem->read_u16_fast(REG_IF);
		ie_check = mem->read_u16_fast(REG_IE);
		
		//Match up bits in IE and IF, execute any non-VBlank interrupts
		for(int x = 0; x < 14; x++)
		{
			//When there is a match, jump to interrupt vector
			if((ie_check & (1 << x)) && (if_check & (1 << x)))
			{
				fire_interrupt = true;
			}
		}

		//Execute VBlank interrupts
		is_vblank = (if_check & 0x1) ? true : false;
	}

	//Artificially hold PC at current location
	//This SWI will be fetched, decoded, and executed again until it hits VBlank 
	reg.r15 -= (arm_mode == ARM) ? 4 : 2;
}

/****** HLE implementation of BitUnPack ******/
void ARM7::swi_bitunpack()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1;
	u32 dest_addr = get_reg(1);

	//Grab pointer to unpack info - R2;
	u32 unpack_info_addr = get_reg(2);

	//Grab the length
	u16 length = mem->read_u16(unpack_info_addr);
	unpack_info_addr += 2;

	//Grab the source width
	u8 src_width = mem->read_u8(unpack_info_addr);
	unpack_info_addr++;

	//Grab the destination width
	u8 dest_width = mem->read_u8(unpack_info_addr);
	unpack_info_addr++;

	if(src_width > dest_width)
	{
		std::cout<<"SWI::ERROR - BitUnPack source width is greater than destination width\n";
		return;
	}

	u8 bit_mask = 0;

	switch(src_width)
	{
		case 1: bit_mask = 0x1; break;
		case 2: bit_mask = 0x3; break;
		case 4: bit_mask = 0xF; break;
		case 8: bit_mask = 0xFF; break;
		default: std::cout<<"SWI::ERROR - Invalid source width\n"; return;
	}

	//Grab the data offset and zero flag
	u32 data_offset = mem->read_u32(unpack_info_addr);
	u8 zero_flag = (data_offset & 0x80000000) ? 1 : 0;
	data_offset &= ~0x80000000;

	u8 src_byte = 0;
	u8 src_count = 0;
	u32 result = 0;

	//Decompress bytes from source addr
	while(length > 0)
	{
		result = 0;

		//Cycle through the byte and expand to destination width
		for(u8 x = 0; x < 32; x += dest_width)
		{
			//Grab new source byte
			if((src_count % 8) == 0) 
			{
				src_byte = mem->read_u8(src_addr++);
				length--;
			}

			//Grab the slice
			u32 slice = (src_byte & bit_mask);
			src_byte >>= src_width;
			src_count += src_width;

			if(slice != 0) { slice += data_offset; }
			else if ((slice == 0) && (zero_flag == 1)) { slice += data_offset; }

			//OR the slice to the final result
			result |= (slice << x);
		}

		//Write result to the destination address
		mem->write_u32(dest_addr, result);
		dest_addr += 4;
	}
}			

/****** HLE implementation of LZ77UnCompVram ******/
void ARM7::swi_lz77uncompvram()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab data header
	u32 data_header = mem->read_u32(src_addr);

	//Grab compressed data size in bytes
	u32 data_size = (data_header >> 8);

	//Pointer to current address of compressed data that needs to be processed
	//When uncompression starts, move 5 bytes from source address (header + flag)
	u32 data_ptr = (src_addr + 4);

	u8 temp = 0;

	//Uncompress data
	while(data_size > 0)
	{
		//Grab flag data
		u8 flag_data = mem->read_u8(data_ptr++);

		//Process 8 blocks
		for(int x = 7; x >= 0; x--)
		{
			u8 block_type = (flag_data & (1 << x)) ? 1 : 0;

			//Block Type 0 - Uncompressed
			if(block_type == 0)
			{
				temp = mem->read_u8(data_ptr++);
				mem->write_u8(dest_addr++, temp);
				
				data_size--;
				if(data_size == 0) { return; }
			}


			//Block Type 1 - Compressed
			else
			{
				u16 compressed_block = mem->read_u16(data_ptr);
				data_ptr += 2;

				u16 distance = ((compressed_block & 0xF) << 8);
				distance |= (compressed_block >> 8);

				u8 length = ((compressed_block >> 4) & 0xF) + 3;

				//Copy length+3 Bytes from dest_addr-length-1 to dest_addr
				for(int y = 0; y < length; y++)
				{
					temp = mem->read_u8(dest_addr - distance - 1);
					mem->write_u8(dest_addr, temp);
					
					dest_addr++;
					data_size--;
					if(data_size == 0) { return; }
				}
			}
		}
	}
}

/****** HLE implementation of HuffUnComp ******/
void ARM7::swi_huffuncomp()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab data header
	u32 data_header = mem->read_u32(src_addr);

	//Grab data bit-size
	u8 bit_size = (data_header & 0xF);

	if((bit_size != 4) && (bit_size != 8)) { std::cout<<"SWI::Warning - HuffUnComp has irregular data size : " << (int)bit_size << "\n"; }

	//Grab compressed data size in bytes - Data comes in units of 32-bits, 4 bytes
	u32 data_size = (data_header >> 8);
	while((data_size & 0x3) != 0) { data_size++; }

	//Pointer to current address that needs to be processed
	//When uncompression start, points to data after the header (the first Tree Size attribute)
	u32 data_ptr = (src_addr + 4);

	u8 data_shift = 0;
	u32 temp = 0;

	//Grab Tree Size
	u16 tree_size = mem->read_u8(data_ptr++);
	tree_size = ((tree_size + 1) * 2) - 1;

	//Grab the root node
	u32 root_node_addr = data_ptr;
	u32 node_position = 0;

	//Grab the 32-bit compressed bitstream
	u32 bitstream = mem->read_u32(data_ptr + tree_size);
	u32 bitstream_addr = (data_ptr + tree_size);
	u32 bitstream_mask = 0x80000000;

	bool is_data_node = false;

	//Uncompress data
	while(data_size > 0)
	{
		//Begin parsing the nodes, starting with root node
		while(bitstream_mask != 0)
		{
			u8 node = mem->read_u8(data_ptr);
			u8 node_offset = (node & 0x3F);
			u8 node_0_type = (node & 0x80) ? 1 : 0;
			u8 node_1_type = (node & 0x40) ? 1 : 0;

			//If this node is a data node, read data
			if(is_data_node) 
			{
				//Grab data from the node
				u8 data = 0;

				if(data_size == 4) { data = node & 0xF; }
				else { data = node; }

				//Add data to 32-bit value
				temp |= (data << data_shift);
				data_shift += bit_size;

				//Transfer completed 32-bit value to memory
				if(data_shift >= 32) 
				{
					mem->write_u32(dest_addr, temp);
					dest_addr += 4;
					data_size -= 4;
					temp = 0;
					data_shift = 0;

					if(data_size == 0) { return; } 
				}

				//Return to root node
				data_ptr = root_node_addr;
				is_data_node = false;
				node_position = 0;
			}

			//If this node is a child node, continue along the binary trees
			else
			{
				if(node_position == 0) { node_position++; }
				else { node_position += ((node_offset + 1) * 2); }
			
				//Read bitstream bit, decide if child_node.0 or child_node.1 should be looked at
				u8 bitstream_bit = (bitstream & bitstream_mask) ? 1 : 0;
				bitstream_mask >>= 1;

				//Go offset for child_node.1
				if(bitstream_bit == 1) 
				{
					data_ptr = (root_node_addr + node_position + 1);
					if(node_1_type == 1) { is_data_node = true; }
					else { is_data_node = false; }
				}

				//Go offset for child_node.0
				else 
				{
					data_ptr = (root_node_addr + node_position);
					if(node_0_type == 1) { is_data_node = true; }
					else { is_data_node = false; }
				}
				
			}
		}

		//After this 32-bit bitstream is complete, move onto the next bitstream
		bitstream_addr += 4;
		bitstream = mem->read_u32(bitstream_addr);
		bitstream_mask = 0x80000000;
	}
}

/****** HLE implementation of RLUnCompVram ******/
void ARM7::swi_rluncompvram()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab data header
	u32 data_header = mem->read_u32(src_addr);

	u32 data_size = (data_header >> 8);

	//Data pointer to compressed data. Points to first flag.
	u32 data_ptr = (src_addr + 4);

	//Uncompress data
	while(data_size > 0)
	{
		u8 flag = mem->read_u8(data_ptr++);

		u8 data_length = (flag & 0x7F);

		//Adjust data length, +1 for uncompressed data, +3 for compressed data
		if(flag & 0x80) { data_length += 3; }
		else { data_length += 1; }

		//Output the specified byte the amount of times in data_length
		for(int x = 0; x < data_length; x++)
		{
			u8 data_byte = 0;

			//Compressed
			if(flag & 0x80) { data_byte = mem->read_u8(data_ptr); }

			//Uncompressed
			else { data_byte = mem->read_u8(data_ptr++); }
				
			mem->write_u8(dest_addr++, data_byte);
			data_size--;

			if(data_size == 0) { return; }
		}

		//Manually adjust data pointer for compressed data to point to next flag
		if(flag & 0x80) { data_ptr++; }
	}
}		
	
/****** HLE implementation of GetBIOSChecksum ******/
void ARM7::swi_getbioschecksum()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Return checksum in R0
	set_reg(0, 0xBAAE187F);
}

/****** HLE implementation of BGAffineSet ******/
void ARM7::swi_bgaffineset()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab source data field address - R0
	u32 src_addr = get_reg(0);

	//Grab destination field address - R1
	u32 dest_addr = get_reg(1);

	//Grab the number of calculations to perform - R2
	u32 calc_count = get_reg(2);

	//Perform the desired number of calculations
	while(calc_count-- > 0)
	{
		//Grab original data's central X-Y coordinates
		s32 center_x = mem->read_u32(src_addr); src_addr += 4;
		s32 center_y = mem->read_u32(src_addr); src_addr += 4;

		//Grab the display's central X-Y coordinates
		s16 display_x = mem->read_u16(src_addr); src_addr += 2;
		s16 display_y = mem->read_u16(src_addr); src_addr += 2;

		//Grab the X-Y scaling ratios
		s16 scale_x = mem->read_u16(src_addr); src_addr += 2;
		s16 scale_y = mem->read_u16(src_addr); src_addr += 2;

		//Grab the angle of rotation, add 4 to keep data structure aligned by word-size
		u16 theta = mem->read_u16(src_addr); src_addr += 4;
		theta >>= 8;

  		s32 cos_angle = sine_lut[(theta+0x40)&255];
    		s32 sin_angle = sine_lut[theta];

		//Calculate differences in X-Y coordinates for this line and the next
		s16 diff_x1 = (scale_x * cos_angle) >> 14;
		s16 diff_x2 = (scale_x * sin_angle) >> 14;
		s16 diff_y1 = (scale_y * sin_angle) >> 14;
		s16 diff_y2 = (scale_y * cos_angle) >> 14;

		//Write to destination data structure
		mem->write_u16(dest_addr, diff_x1); dest_addr += 2;
		mem->write_u16(dest_addr, -diff_x2); dest_addr += 2;
		mem->write_u16(dest_addr, diff_y1); dest_addr += 2;
		mem->write_u16(dest_addr, diff_y2); dest_addr += 2;

		//Calculate start X-Y coordinates
		s32 start_x = (center_x - diff_x1 * display_x + diff_x2 * display_y);
		s32 start_y = (center_y - diff_y1 * display_x - diff_y2 * display_y);

		mem->write_u32(dest_addr, start_x); dest_addr += 4;
		mem->write_u32(dest_addr, start_y); dest_addr += 4;
	}
} 

/****** HLE implementation of OBJAffineSet ******/
void ARM7::swi_objaffineset()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab source data field address - R0
	u32 src_addr = get_reg(0);

	//Grab destination field address - R1
	u32 dest_addr = get_reg(1);

	//Grab the number of calculations to perform - R2
	u32 calc_count = get_reg(2);

	//Grab offset to parameter address - R3
	s32 offset = get_reg(3);

	//Perform the desired number of calculations
	while(calc_count-- > 0)
	{
		//Grab the X-Y scaling ratios
		s16 scale_x = mem->read_u16(src_addr); src_addr += 2;
		s16 scale_y = mem->read_u16(src_addr); src_addr += 2;

		//Grab the angle of rotation, add 4 to keep data structure aligned by word-size
		u16 theta = mem->read_u16(src_addr); src_addr += 4;
		theta >>= 8;

  		s32 cos_angle = sine_lut[(theta+0x40)&255];
    		s32 sin_angle = sine_lut[theta];

		//Calculate differences in X-Y coordinates for this line and the next
		s16 diff_x1 = (scale_x * cos_angle) >> 14;
		s16 diff_x2 = (scale_x * sin_angle) >> 14;
		s16 diff_y1 = (scale_y * sin_angle) >> 14;
		s16 diff_y2 = (scale_y * cos_angle) >> 14;

		//Write to destination data structure
		mem->write_u16(dest_addr, diff_x1); dest_addr += offset;
		mem->write_u16(dest_addr, -diff_x2); dest_addr += offset;
		mem->write_u16(dest_addr, diff_y1); dest_addr += offset;
		mem->write_u16(dest_addr, diff_y2); dest_addr += offset;
	}
}

/****** HLE implementation of MidiKey2Freq ******/
void ARM7::swi_midikey2freq()
{
	bios_read_state = BIOS_SWI_FINISH;

	//Grab input frequency
	u32 frequency = mem->read_u32(get_reg(0) + 4);

	//Grab Midi key and fine adjustment value - R1 and R2
	u8 mk = get_reg(1);
	u8 fp = get_reg(2);

	double temp_frequency = (180.0 - mk) - (fp / 256.0);

	temp_frequency = pow(2.0, temp_frequency/12.0);
	set_reg(0, frequency/temp_frequency);
}