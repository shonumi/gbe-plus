// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : arm_instr.cpp
// Date : May 23, 2014
// Description : ARM7 THUMB instructions
//
// Emulates an ARM7 ARM instructions with equivalent C++

#include "arm7.h"

/****** ARM.3 - Branch and Exchange ******/
void ARM7::branch_exchange(u32 current_arm_instruction)
{
	//Grab source register - Bits 0-2
	u8 src_reg = (current_arm_instruction & 0x7);

	//Valid registers : 0-14
	if(src_reg <= 14)
	{
		u32 result = get_reg(src_reg);
		u8 op = (current_arm_instruction >> 4) & 0xF;

		//Switch to THUMB mode if necessary
		if(result & 0x1) 
		{ 
			arm_mode = THUMB; 
			result &= ~0x1;
			//std::cout<<"\n\n ** Switching to THUMB Mode ** \n\n";
		}

		switch(op)
		{
			//Branch
			case 0x1:
				//Clock CPU and controllers - 1N
				clock(reg.r15, true);

				reg.r15 = result;
				needs_flush = true;

				//Clock CPU and controllers - 2S
				clock(reg.r15, false);
				clock((reg.r15 + 4), false);

				break;

			//Branch and Link
			case 0x11:
				//Clock CPU and controllers - 1N
				clock(reg.r15, true);

				set_reg(14, (reg.r15 - 4));
				reg.r15 = result;
				needs_flush = true;

				//Clock CPU and controllers - 2S
				clock(reg.r15, false);
				clock((reg.r15 + 4), false);

				break;

			default:
				std::cout<<"CPU::Error - ARM.3 invalid Branch and Exchange opcode : 0x" << std::hex << op << "\n";
				running = false;
				break;
		}
	}

	else { std::cout<<"CPU::Error - ARM.3 Branch and Exchange - Invalid operand : R15\n"; running = false; }
}  

/****** ARM.4 - Branch and Branch with Link ******/
void ARM7::branch_link(u32 current_arm_instruction)
{
	//Grab signed offset
	s32 offset = (current_arm_instruction & 0xFFFFFF);

	u8 op = (current_arm_instruction >> 24) & 0x1;

	switch(op)
	{
		//Branch
		case 0x0:
			//Clock CPU and controllers - 1N
			clock(reg.r15, true);

			reg.r15 += (offset * 4);
			needs_flush = true;

			//Clock CPU and controllers - 2S
			clock(reg.r15, false);
			clock((reg.r15 + 4), false);

			break;

		//Branch and Link
		case 0x1:
			//Clock CPU and controllers - 1N
			clock(reg.r15, true);

			set_reg(14, (reg.r15 - 4));
			reg.r15 +=  (offset * 4);
			needs_flush = true;

			//Clock CPU and controllers - 2S
			clock(reg.r15, false);
			clock((reg.r15 + 4), false);

			break;
	}
}

/****** ARM.5 Data Processing ******/
void ARM7::data_processing(u32 current_arm_instruction)
{
	//Determine if an immediate value or a register should be used as the operand
	bool use_immediate = (current_arm_instruction & 0x2000000) ? true : false;

	//Determine if condition codes should be updated
	bool set_condition = (current_arm_instruction & 0x100000) ? true : false;
	
	u8 op = (current_arm_instruction >> 21) & 0xF;

	//Grab source register
	u8 src_reg = (current_arm_instruction >> 16) & 0xF;

	//Grab destination register
	u8 dest_reg = (current_arm_instruction >> 12) & 0xF;

	//When use_immediate is 0, determine whether the register should be shifted by another register or an immediate
	bool shift_immediate = (current_arm_instruction & 0x10) ? false : true;

	u32 result = 0;
	u32 input = get_reg(src_reg);
	u32 operand = 0;
	u8 shift_out = 0;

	//Use immediate as operand
	if(use_immediate)
	{
		operand = (current_arm_instruction & 0xFF);
		u8 offset = (current_arm_instruction >> 8) & 0xF;
		
		//Shift immediate - ROR special case - Carry flag not affected
		rotate_right_special(operand, offset);
	}

	//Use register as operand
	else
	{
		operand = get_reg(current_arm_instruction & 0xF);
		u8 shift_type = (current_arm_instruction >> 5) & 0x3;
		u8 offset = 0;

		//Shift the register-operand by an immediate
		if(shift_immediate)
		{
			offset = (current_arm_instruction >> 7) & 0x1F;
		}

		//Shift the register-operand by another register
		else
		{
			offset = get_reg((current_arm_instruction >> 8) & 0xF);
			
			//Valid registers to shift by are R0-R14
			if(((current_arm_instruction >> 8) & 0xF) == 0xF) { std::cout<< "CPU::Error - ARM.5 Data Processing - Shifting Register-Operand by PC \n"; running = false; }
		}

		//Shift the register
		switch(shift_type)
		{
			//LSL
			case 0x0:
				shift_out = logical_shift_left(operand, offset);
				break;

			//LSR
			case 0x1:
				shift_out = logical_shift_right(operand, offset);
				break;

			//ASR
			case 0x2:
				shift_out = arithmetic_shift_right(operand, offset);
				break;

			//ROR
			case 0x3:
				shift_out = rotate_right(operand, offset);
				break;
		}
		
		//Clock CPU and controllers - 1I
		clock();

		//After shifting by register with the PC as the destination register, PC jumps 4 bytes ahead
		if((dest_reg == 15) && (!shift_immediate)) { reg.r15 += 4; }
	}		

	//TODO - When op is 0x8 through 0xB, make sure Bit 20 is 1 (rather force it? Unsure)
	//TODO - 2nd Operand for TST/TEQ/CMP/CMN must be R0 (rather force it to be R0)
	//TODO - See GBATEK - S=1, with unused Rd bits=1111b

	//Clock CPU and controllers - 1N
	if(dest_reg == 15) { clock(reg.r15, true); set_condition = false; reg.cpsr = get_spsr(); }

	switch(op)
	{
		//AND
		case 0x0:
			result = (input & operand);
			set_reg(dest_reg, result);

			//Update condition codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;

		//XOR
		case 0x1:
			result = (input ^ operand);
			set_reg(dest_reg, result);

			//Update condition codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;

		//SUB
		case 0x2:
			result = (input - operand);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, false); }
			break;

		//RSB
		case 0x3:
			result = (operand - input);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, false); }
			break;

		//ADD
		case 0x4:
			result = (input + operand);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, true); }
			break;

		//ADC
		case 0x5:
			result = (input + operand + shift_out);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, true); }
			break;

		//SBC
		case 0x6:
			result = (input - operand + shift_out - 1);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, false); }
			break;

		//RSC
		case 0x7:
			result = (operand - input + shift_out - 1);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, false); }
			break;

		//TST
		case 0x8:
			result = (input & operand);

			//Update condtion codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;

		//TEQ
		case 0x9:
			result = (input ^ operand);

			//Update condtion codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;

		//CMP
		case 0xA:
			result = (input - operand);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, false); }
			break;

		//CMN
		case 0xB:
			result = (input + operand);
		
			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, true); }
			break;

		//ORR
		case 0xC:
			result = (input | operand);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;

		//MOV
		case 0xD:
			result = operand;
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;

		//BIC
		case 0xE:
			result = (input & (~operand));

			//Update condtion codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;

		//MVN
		case 0xF:
			result = ~operand;
			set_reg(0, result);

			//Update condtion codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;
	}

	//Timings for PC as destination register
	if(dest_reg == 15) 
	{
		//Clock CPU and controllers - 2S
		needs_flush = true; 
		clock(reg.r15, false);
		clock((reg.r15 + 4), false);
	}

	//Timings for regular registers
	else 
	{
		//Clock CPU and controllers - 1S
		clock((reg.r15 + 4), false);
	}
}

/****** ARM.9 Single Data Transfer ******/
void ARM7::single_data_transfer(u32 current_arm_instruction)
{
	//Grab Immediate-Offset flag - Bit 25
	u8 offset_is_register = (current_arm_instruction & 0x2000000) ? 1 : 0;

	//Grab Pre-Post bit - Bit 24
	u8 pre_post = (current_arm_instruction & 0x1000000) ? 1 : 0;

	//Grab Up-Down bit - Bit 23
	u8 up_down = (current_arm_instruction & 0x800000) ? 1 : 0;

	//Grab Byte-Word bit - Bit 22
	u8 byte_word = (current_arm_instruction & 0x400000) ? 1 : 0;

	//Grab Write-Back bit - Bit 21
	u8 write_back = (current_arm_instruction & 0x200000) ? 1 : 0;

	//Grab Load-Store bit - Bit 20
	u8 load_store = (current_arm_instruction & 0x100000) ? 1 : 0;

	//Grab the Base Register - Bits 16-19
	u8 base_reg = ((current_arm_instruction >> 16) & 0xF);

	//Grab the Destination Register - Bits 12-15
	u8 dest_reg = ((current_arm_instruction >> 12) & 0xF);

	u32 base_offset = 0;
	u32 base_addr = get_reg(base_reg);
	u32 value = 0;

	//Determine Offset - 12-bit immediate
	if(offset_is_register == 0) { base_offset = (current_arm_instruction & 0xFFF); }

	//Determine Offset - Shifted register
	else
	{
		//Grab register to use as offset - Bits 0-3
		u8 offset_register = (current_arm_instruction & 0xF);
		base_offset = get_reg(offset_register);

		//Grab the shift type - Bits 5-6
		u8 shift_type = ((current_arm_instruction >> 5) & 0x3);

		//Grab the shift offset - Bits 7-11
		u8 shift_offset = ((current_arm_instruction >> 7) & 0x1F);

		//Shift the register
		switch(shift_type)
		{
			//LSL
			case 0x0:
				logical_shift_left(base_offset, shift_offset);
				break;

			//LSR
			case 0x1:
				logical_shift_right(base_offset, shift_offset);
				break;

			//ASR
			case 0x2:
				arithmetic_shift_right(base_offset, shift_offset);
				break;

			//ROR
			case 0x3:
				rotate_right(base_offset, shift_offset);
				break;
		}
	}


	//Increment or decrement after transfer if post-indexing
	if(pre_post == 1) 
	{ 
		if(up_down == 1) { base_addr += base_offset; }
		else { base_addr -= base_offset; } 
	}

	//Clock CPU and controllers - 1N
	clock(reg.r15, true);

	//Store Byte or Word
	if(load_store == 0) 
	{
		if(byte_word == 1)
		{
			value = get_reg(dest_reg);
			mem->write_u8(base_addr, (value & 0xFF));
		}

		else
		{
			value = get_reg(dest_reg);
			mem->write_u32(base_addr, value);
		}

		//Clock CPU and controllers - 1N
		clock(base_addr, true);
	}

	//Load Byte or Word
	else
	{
		if(byte_word == 1)
		{
			//Clock CPU and controllers - 1I
			value = mem->read_u8(base_addr);
			clock();

			//Clock CPU and controllers - 1N
			if(dest_reg == 15) { clock((reg.r15 + 4), true); } 

			set_reg(dest_reg, value);
		}

		else
		{
			//Clock CPU and controllers - 1I
			value = mem->read_u32(base_addr);
			clock();

			//Clock CPU and controllers - 1N
			if(dest_reg == 15) { clock((reg.r15 + 4), true); } 

			set_reg(dest_reg, value);
		}
	}

	//Increment or decrement after transfer if post-indexing
	if(pre_post == 0) 
	{ 
		if(up_down == 1) { base_addr += base_offset; }
		else { base_addr -= base_offset; } 
	}

	//Write back into base register
	//Post-indexing ALWAYS does this. Pre-Indexing does this optionally
	if(pre_post == 0) { set_reg(base_reg, base_addr); }
	else if((pre_post == 1) && (write_back == 1)) { set_reg(base_reg, base_addr); }

	//Timings for LDR - PC
	if((dest_reg == 15) && (load_store == 1)) 
	{
		//Clock CPU and controllser - 2S
		clock(reg.r15, false);
		clock((reg.r15 + 4), false);
		needs_flush = true;
	}

	//Timings for LDR - No PC
	else if((dest_reg != 15) && (load_store == 1))
	{
		//Clock CPU and controllers - 1S
		clock(reg.r15, false);
	}
}

/****** ARM.11 Block Data Transfer ******/
void ARM7::block_data_transfer(u32 current_arm_instruction)
{
	//TODO - Clock cycles
	//TODO - Handle PSR bit
	//TODO - Handle empty RList

	//Grab Pre-Post bit - Bit 24
	u8 pre_post = (current_arm_instruction & 0x1000000) ? 1 : 0;

	//Grab Up-Down bit - Bit 23
	u8 up_down = (current_arm_instruction & 0x800000) ? 1 : 0;

	//Grab PSR bit - Bit 22
	u8 psr = (current_arm_instruction & 0x400000) ? 1 : 0;

	//Grab Write-Back bit - Bit 21
	u8 write_back = (current_arm_instruction & 0x200000) ? 1 : 0;

	//Grab Load-Store bit - Bit 20
	u8 load_store = (current_arm_instruction & 0x100000) ? 1 : 0;

	//Grab the Base Register - Bits 16-19
	u8 base_reg = ((current_arm_instruction >> 16) & 0xF);

	//Grab the Register List - Bits 0-15
	u16 r_list = (current_arm_instruction & 0xFFFF);

	//Warnings
	if(base_reg == 15) { std::cout<<"CPU::Warning - ARM.11 R15 used as Base Register \n"; }

	u32 base_addr = get_reg(base_reg);

	//Load-Store with an ascending stack order, Up-Down = 1
	if((up_down == 1) && (r_list != 0))
	{
		for(int x = 0; x < 16; x++)
		{
			//Increment before transfer if pre-indexing
			if(pre_post == 1) { base_addr += 4; }

			if(r_list & (1 << x)) 
			{ 
				//Store registers
				if(load_store == 0) { mem->write_u32(base_addr, get_reg(x)); }
			
				//Load registers
				else { set_reg(x, mem->read_u32(base_addr)); }
			}
			
			//Increment after transfer if post-indexing
			if(pre_post == 0) { base_addr += 4; }

			//Write back the into base register
			if(write_back == 1) { set_reg(base_reg, base_addr); }
		}
	}

	//Load-Store with a descending stack order, Up-Down = 0
	else if((up_down == 0) && (r_list != 0))
	{
		for(int x = 15; x >= 0; x--)
		{
			//Decrement before transfer if pre-indexing
			if(pre_post == 1) { base_addr -= 4; }

			if(r_list & (1 << x)) 
			{ 
				//Store registers
				if(load_store == 0) { mem->write_u32(base_addr, get_reg(x)); }
			
				//Load registers
				else { set_reg(x, mem->read_u32(base_addr)); }
			}
			
			//Decrement after transfer if post-indexing
			if(pre_post == 0) { base_addr -= 4; }

			//Write back the into base register
			if(write_back == 1) { set_reg(base_reg, base_addr); }
		}
	}

	//Special case, empty RList
	//Store PC, add or sub 0x40 to base address
	else { std::cout<<"Empty RList not implemented, too lazy atm :p\n"; }
}
		
		

		

	








