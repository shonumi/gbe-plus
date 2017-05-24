// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : swi.cpp
// Date : November 05, 2015
// Description : NDS ARM7-ARM9 Software Interrupts
//
// Emulates the NDS's Software Interrupts via High Level Emulation

#include <cmath>

#include "arm9.h"
#include "arm7.h"

/****** Process Software Interrupts - NDS9 ******/
void NTR_ARM9::process_swi(u32 comment)
{
	switch(comment)
	{
		//SoftReset
		case 0x0:
			std::cout<<"ARM9::SWI::SoftReset \n";
			swi_softreset();
			break;

		//WaitByLoop
		case 0x3:
			//std::cout<<"ARM9::SWI::WaitByLoop \n";
			swi_waitbyloop();
			break;

		//IntrWait
		case 0x4:
			//std::cout<<"ARM9::SWI::IntrWait \n";
			swi_intrwait();
			break;

		//VBlankIntrWait
		case 0x5:
			//std::cout<<"ARM9::SWI::VBlankIntrWait \n";
			swi_vblankintrwait();
			break;

		//Halt
		case 0x6:
			//std::cout<<"ARM9::SWI::Halt \n";
			swi_halt();
			break;

		//Div
		case 0x9:
			std::cout<<"ARM9::SWI::Div \n";
			swi_div();
			break;

		//CPUSet
		case 0xB:
			std::cout<<"ARM9::SWI::CPUSet \n";
			swi_cpuset();
			break;

		//Sqrt
		case 0xD:
			std::cout<<"ARM9::SWI::Sqrt \n";
			swi_sqrt();
			break;

		//IsDebugger
		case 0xF:
			//std::cout<<"ARM9::SWI::IsDebugger \n";
			swi_isdebugger();
			break;

		//LZ77UnCompReadByCallback
		case 0x12:
			std::cout<<"ARM9::SWI::LZ77UnCompReadByCallback \n";
			swi_lz77uncompvram();
			break;

		default:
			std::cout<<"SWI::Error - Unknown NDS9 BIOS function 0x" << std::hex << comment << "\n";
			running = false;
			break;
	}
}

/****** HLE implementation of SoftReset - NDS9 ******/
void NTR_ARM9::swi_softreset()
{
	//Reset IRQ, SVC, and SYS stack pointers
	reg.r13_svc = 0x0803FC0;
	reg.r13_irq = 0x0803FA0;
	reg.r13 = 0x0803EC0;

	//Set PC to return address at 0x27FFE24
	reg.r15 = mem->read_u32(0x27FFE24);

	//Switch to ARM or THUMB mode as necessary
	if(reg.r15 & 0x1) { arm_mode = THUMB; }
	else { arm_mode = ARM; }	

	needs_flush = true;
	in_interrupt = false;

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

	//Clear top 0x200 bytes of the DTCM
	for(u16 x = 0; x < 0x200; x++) { mem->memory_map[mem->nds9_irq_handler + 0x3E00 + x] = 0; }

	//Clear internal input
	mem->g_pad->clear_input();

	//TODO - Flush caches + write buffer, setup CP15
}

/****** HLE implementation of WaitByLoop - NDS9 ******/
void NTR_ARM9::swi_waitbyloop()
{
	//Setup the initial value for swi_waitbyloop_count - R0
	swi_waitbyloop_count = get_reg(0) & 0x7FFFFFFF;
	swi_waitbyloop_count >>= 2;

	//Set CPU idle state to 2
	idle_state = 2;
}

/****** HLE implementation of Halt - NDS9 ******/
void NTR_ARM9::swi_halt()
{
	//Set CPU idle state to 1
	idle_state = 1;

	//Destroy R0
	set_reg(0, 0);
}

/****** HLE implementation of IntrWait - NDS9 ******/
void NTR_ARM9::swi_intrwait()
{
	//NDS9 version is slightly bugged. When R0 == 0, it simply waits for any new interrupt, then leaves
	//Normally, it should return immediately if the flags in R1 are already set in IF
	//R0 == 1 will wait for the specified flags in R1

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Grab old IF, set current one to zero
	mem->nds9_old_if = mem->nds9_if;
	mem->nds9_if = 0;

	//Grab old IE, set current one to R1
	mem->nds9_old_ie = mem->nds9_ie;
	mem->nds9_ie = reg.r1;

	//Set CPU idle state to 3
	idle_state = 3;
}

/****** HLE implementation of VBlankIntrWait - NDS9 ******/
void NTR_ARM9::swi_vblankintrwait()
{
	//This is basically the IntrWait SWI, but R0 and R1 are both set to 1
	reg.r0 = 1;
	reg.r1 = 1;

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Grab old IF, set current one to zero
	mem->nds9_old_if = mem->nds9_if;
	mem->nds9_if = 0;

	//Grab old IE, set current one to R1
	mem->nds9_old_ie = mem->nds9_ie;
	mem->nds9_ie = reg.r1;

	//Set CPU idle state to 3
	idle_state = 3;
}

/****** HLE implementation of Div - NDS9 ******/
void NTR_ARM9::swi_div()
{
	//Grab the numerator - R0
	s32 num = get_reg(0);
	
	//Grab the denominator - R1
	s32 den = get_reg(1);

	s32 result = 0;
	s32 modulo = 0;

	//Do NOT divide by 0
	if(den == 0) { std::cout<<"ARM9::SWI::Warning - Div tried to divide by zero (ignoring operation) \n"; return; }

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

/****** HLE implementation of CPUSet - NDS9 ******/
void NTR_ARM9::swi_cpuset()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

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

/****** HLE implementation of Sqrt - NDS9 ******/
void NTR_ARM9::swi_sqrt()
{
	//Grab input
	u32 input = get_reg(0);

	//Set result of operation
	u16 result = sqrt(input);
	set_reg(0, result);
}

/****** HLE implementation of IsDebugger - NDS9 ******/
void NTR_ARM9::swi_isdebugger()
{
	//Always act as if a retail NDS, set RO to zero
	set_reg(0, 0);

	//Destroy value at 0x27FFFF8 (halfword)
	mem->write_u16(0x27FFFF8, 0x0);
}

/****** HLE implementation of LZ77UnCompReadByCallback - NDS9 ******/
void NTR_ARM9::swi_lz77uncompvram()
{
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

/****** Process Software Interrupts - NDS7 ******/
void NTR_ARM7::process_swi(u32 comment)
{
	switch(comment)
	{
		//SoftReset
		case 0x0:
			std::cout<<"ARM7::SWI::SoftReset \n";
			swi_softreset();
			break;

		//WaitByLoop
		case 0x3:
			//std::cout<<"ARM7::SWI::WaitByLoop \n";
			swi_waitbyloop();
			break;

		//IntrWait
		case 0x4:
			//std::cout<<"ARM9::SWI::IntrWait \n";
			swi_intrwait();
			break;

		//VBlankIntrWait
		case 0x5:
			//std::cout<<"ARM7::SWI::VBlankIntrWait \n";
			swi_vblankintrwait();
			break;

		//Halt
		case 0x6:
			////std::cout<<"ARM7::SWI::Halt \n";
			swi_halt();
			break;

		//Div
		case 0x9:
			std::cout<<"ARM7::SWI::Div \n";
			swi_div();
			break;

		//CPUSet
		case 0xB:
			std::cout<<"ARM7::SWI::CPUSet \n";
			swi_cpuset();
			break;

		//Sqrt
		case 0xD:
			std::cout<<"ARM7::SWI::Sqrt \n";
			swi_sqrt();
			break;

		//GetCRC16
		case 0xE:
			//std::cout<<"ARM7::SWI::GetCRC16 \n";
			swi_getcrc16();
			break;

		//LZ77UnCompReadByCallback
		case 0x12:
			std::cout<<"ARM7::SWI::LZ77UnCompReadByCallback \n";
			swi_lz77uncompvram();
			break;
			
		default:
			std::cout<<"SWI::Error - Unknown NDS7 BIOS function 0x" << std::hex << comment << "\n";
			running = false;
			break;
	}
}

/****** HLE implementation of SoftReset - NDS7 ******/
void NTR_ARM7::swi_softreset()
{
	//Reset IRQ, SVC, and SYS stack pointers
	reg.r13_svc = 0x380FFDC;
	reg.r13_irq = 0x380FFB0;
	reg.r13 = 0x380FF00;

	//Set PC to return address at 0x27FFE34
	reg.r15 = mem->read_u32(0x27FFE34);

	//Switch to ARM or THUMB mode as necessary
	if(reg.r15 & 0x1) { arm_mode = THUMB; }
	else { arm_mode = ARM; }	

	needs_flush = true;
	in_interrupt = false;

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

	//Clear top 0x200 bytes of some RAM
	for(int x = 0x380FE00; x < 3810000; x++) { mem->memory_map[x] = 0; }

	//Clear internal input
	mem->g_pad->clear_input();
}

/****** HLE implementation of GetCRC16 - NDS7 ******/
void NTR_ARM7::swi_getcrc16()
{
	//R0 = Initial CRC value
	//R1 = Start address of data to look at
	//R2 = Length of data to look at in bytes
	u16 crc = get_reg(0);
	u32 data_addr = get_reg(1) & ~0x1;
	u32 length = get_reg(2) & ~0x1;

	//LUT for CRC
	u16 table[] = { 0xC0C1, 0xC181, 0xC301, 0xC601, 0xCC01, 0xD801, 0xF001, 0xA001 };

	//Cycle through all the data to get the CRC16
	for(u32 x = 0; x < length; x++)
	{
		u16 data_byte = mem->memory_map[data_addr++];
		crc = crc ^ data_byte;

		for(u32 y = 0; y < 8; y++)
		{
			
			if(crc & 0x1)
			{
				crc >>= 1;
				crc = crc ^ (table[y] << (7 - y));
			}

			else { crc >>= 1; }
		}
	}

	set_reg(0, crc);
}

/****** HLE implementation of WaitByLoop - NDS7 ******/
void NTR_ARM7::swi_waitbyloop()
{
	//Setup the initial value for swi_waitbyloop_count - R0
	swi_waitbyloop_count = get_reg(0) & 0x7FFFFFFF;

	//Set CPU idle state to 2
	idle_state = 2;
}

/****** HLE implementation of IntrWait - NDS7 ******/
void NTR_ARM7::swi_intrwait()
{
	//When R0 == 0, SWI will exit if any flags checked in R1 are already set
	//When R0 == 1, SWI will discard current IF flags and wait for the specified flags in R1

	if((reg.r0 == 0) && (reg.r1 & mem->nds7_if)) { return; } 

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Grab old IF, set current one to zero if necessary
	mem->nds7_old_if = mem->nds7_if;
	mem->nds7_if = (reg.r0) ? 0 : mem->nds7_if;

	//Grab old IE, set current one to R1
	mem->nds7_old_ie = mem->nds7_ie;
	mem->nds7_ie = reg.r1;

	//Set CPU idle state to 3
	idle_state = 3;
}

/****** HLE implementation of VBlankIntrWait - NDS7 ******/
void NTR_ARM7::swi_vblankintrwait()
{
	//This is basically the IntrWait SWI, but R0 and R1 are both set to 1
	reg.r0 = 1;
	reg.r1 = 1;

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Grab old IF, set current one to zero
	mem->nds7_old_if = mem->nds7_if;
	mem->nds7_if = 0;

	//Grab old IE, set current one to R1
	mem->nds7_old_ie = mem->nds7_ie;
	mem->nds7_ie = reg.r1;

	//Set CPU idle state to 3
	idle_state = 3;
}

/****** HLE implementation of Halt - NDS7 ******/
void NTR_ARM7::swi_halt()
{
	//Set CPU idle state to 1
	idle_state = 1;
}

/****** HLE implementation of Div - NDS7 ******/
void NTR_ARM7::swi_div()
{
	//Grab the numerator - R0
	s32 num = get_reg(0);
	
	//Grab the denominator - R1
	s32 den = get_reg(1);

	s32 result = 0;
	s32 modulo = 0;

	//Do NOT divide by 0
	if(den == 0) { std::cout<<"ARM7::SWI::Warning - Div tried to divide by zero (ignoring operation) \n"; return; }

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

/****** HLE implementation of CPUSet - NDS7 ******/
void NTR_ARM7::swi_cpuset()
{
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

/****** HLE implementation of Sqrt - NDS7 ******/
void NTR_ARM7::swi_sqrt()
{
	//Grab input
	u32 input = get_reg(0);

	//Set result of operation
	u16 result = sqrt(input);
	set_reg(0, result);
}

/****** HLE implementation of LZ77UnCompReadByCallback - NDS7 ******/
void NTR_ARM7::swi_lz77uncompvram()
{
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
