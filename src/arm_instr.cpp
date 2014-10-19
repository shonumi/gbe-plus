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
	u8 src_reg = (current_arm_instruction & 0xF);

	//Valid registers : 0-14
	if(src_reg <= 14)
	{
		u32 result = get_reg(src_reg);
		u8 op = (current_arm_instruction >> 4) & 0xF;

		//Switch to THUMB mode if necessary
		if(result & 0x1) 
		{ 
			arm_mode = THUMB;
			reg.cpsr |= 0x20;
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
	//Grab offset
	u32 offset = (current_arm_instruction & 0xFFFFFF);

	//Grab opcode
	u8 op = (current_arm_instruction >> 24) & 0x1;

	s32 jump_addr = 0;
	u32 final_addr = 0;

	//Convert 2's complement
	if(offset & 0x800000) 
	{
		offset--;
		offset = ~offset;

	jump_addr = (offset * -4);
	}

	else { jump_addr = offset * 4; }

	final_addr = reg.r15 + jump_addr;
	final_addr &= 0xFFFFFF;

	switch(op)
	{
		//Branch
		case 0x0:
			//Clock CPU and controllers - 1N
			clock(reg.r15, true);

			reg.r15 &= ~0xFFFFFF;
			reg.r15 |= final_addr;
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
			reg.r15 &= ~0xFFFFFF;
			reg.r15 |= final_addr;
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
			//If no shift was performed, use the current Carry Flag for this math op
			if(shift_out == 2) { shift_out = (reg.cpsr & CPSR_C_FLAG) ? 1 : 0; }

			result = (input + operand + shift_out);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, true); }
			break;

		//SBC
		case 0x6:
			//If no shift was performed, use the current Carry Flag for this math op
			if(shift_out == 2) { shift_out = (reg.cpsr & CPSR_C_FLAG) ? 1 : 0; }

			result = (input - operand + shift_out - 1);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, false); }
			break;

		//RSC
		case 0x7:
			//If no shift was performed, use the current Carry Flag for this math op
			if(shift_out == 2) { shift_out = (reg.cpsr & CPSR_C_FLAG) ? 1 : 0; }

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

/****** ARM.6 PSR Transfer ******/
void ARM7::psr_transfer(u32 current_arm_instruction)
{
	//Determine if an immediate or a register will be used as input (MSR only) - Bit 25
	bool use_immediate = (current_arm_instruction & 0x2000000) ? true : false;

	//Determine access is for CPSR or SPSR - Bit 22
	u8 psr = (current_arm_instruction & 0x400000) ? 1 : 0;

	//Grab opcode
	u8 op = (current_arm_instruction & 0x200000) ? 1 : 0;

	switch(op)
	{
		//MRS
		case 0x0:
			{
				//Grab destination register - Bits 12-15
				u8 dest_reg = ((current_arm_instruction >> 12) & 0xFF);

				if(dest_reg == 15) { std::cout<<"CPU::Warning - ARM.6 R15 used as Destination Register \n"; }

				//Store CPSR into destination register
				if(psr == 0) { set_reg(dest_reg, reg.cpsr); }
		
				//Store SPSR into destination register
				else { set_reg(dest_reg, get_spsr()); }
			}
			break;

		//MSR
		case 0x1:
			{
				u32 input = 0;

				//Create Op Field mask
				u32 op_field_mask = 0;

				//Flag field - Bit 19
				if(current_arm_instruction & 0x80000) { op_field_mask |= 0xFF000000; }
	
				//Status field - Bit 18
				if(current_arm_instruction & 0x40000) 
				{ 
					op_field_mask |= 0x00FF0000;
					std::cout<<"CPU::Warning - ARM.6 MSR enabled access to Status Field \n";
				}

				//Extension field - Bit 17
				if(current_arm_instruction & 0x20000) 
				{ 
					op_field_mask |= 0x0000FF00;
					std::cout<<"CPU::Warning - ARM.6 MSR enabled access to Extension Field \n";
				}

				//Control field - Bit 15
				if(current_arm_instruction & 0x10000) { op_field_mask |= 0x000000FF; }

				//Use shifted 8-bit immediate as input
				if(use_immediate) 
				{
					//Grab shift offset - Bits 8-11
					u8 offset = ((current_arm_instruction >> 8) & 0xF);

					//Grab 8-bit immediate - Bits 0-7
					input = (current_arm_instruction) & 0xFF;

					rotate_right_special(input, offset);
				}

				//Use register as input
				else
				{
					//Grab source register - Bits 0-3
					u8 src_reg = (current_arm_instruction & 0xF);

					if(src_reg == 15) { std::cout<<"CPU::Warning - ARM.6 R15 used as Source Register \n"; }

					input = get_reg(src_reg);
					input &= op_field_mask;
				}

				//Write into CPSR
				if(psr == 0) 
				{ 
					reg.cpsr &= ~op_field_mask;
					reg.cpsr |= input;
				}
	
				//Write into SPSR
				else
				{
					u32 temp_spsr = get_spsr();
					temp_spsr &= ~op_field_mask;
					temp_spsr |= input;
					set_spsr(temp_spsr);
				} 
			}
			break;
	}

	//Clock CPU and controllers - 1S
	clock((reg.r15 + 4), false);
} 


/****** ARM.7 Multiply and Multiply-Accumulate ******/
void ARM7::multiply(u32 current_arm_instruction)
{
	//TODO - Timings
	//TODO - The rest of the opcodes
	//TODO - Find out what GBATEK means when it says the carry flag is 'destroyed'.
	//TODO - Set conditions

	//Grab operand register Rm - Bits 0-3
	u8 op_rm_reg = (current_arm_instruction) & 0xF;

	//Grab operand register Rs - Bits 8-11
	u8 op_rs_reg = ((current_arm_instruction >> 8) & 0xF);

	//Grab accumulate register Rn - Bits 12-15
	u8 accu_reg = ((current_arm_instruction >> 12) & 0xF);

	//Grab destination register Rd - Bits 16-19
	u8 dest_reg = ((current_arm_instruction >> 16) & 0xF);

	//Determine if condition codes should be updated - Bit 20
	bool set_condition = (current_arm_instruction & 0x100000) ? true : false;

	//Grab opcode - Bits 21-24
	u8 op_code = ((current_arm_instruction >> 21) & 0xF);

	//Make sure no operand or destination register is R15
	if(op_rm_reg == 15) { std::cout<<"CPU::Warning - ARM.7 R15 used as Rm\n"; }
	if(op_rs_reg == 15) { std::cout<<"CPU::Warning - ARM.7 R15 used as Rs\n"; }
	if(accu_reg == 15) { std::cout<<"CPU::Warning - ARM.7 R15 used as Rn\n"; }
	if(dest_reg == 15) { std::cout<<"CPU::Warning - ARM.7 R15 used as Rd\n"; }

	u32 Rm = get_reg(op_rm_reg);
	u32 Rs = get_reg(op_rs_reg);
	u32 Rn = get_reg(accu_reg);
	u32 Rd = get_reg(dest_reg);

	u64 value_64 = 0;
	u32 value_32 = 0;

	//Perform multiplication ops
	switch(op_code)
	{
		//MUL
		case 0x0:
			value_32 = (Rm * Rs);
			set_reg(dest_reg, Rd);
			
			break;

		//UMULL
		case 0x4:
			value_64 = (Rm * Rs);
			
			//Set Rn to low 32-bits, Rd to high 32-bits
			Rn = (value_64 & 0xFFFFFFFF);
			Rd = (value_64 >> 32);

			set_reg(accu_reg, Rn);
			set_reg(dest_reg, Rd);

			break;

		default: std::cout<<"CPU::Warning:: - ARM.7 Invalid or unimplemented opcode : " << std::hex << (int)op_code << "\n"; std::cout<<"OP -> 0x" << current_arm_instruction << "\n";
	}
}
			
/****** ARM.9 Single Data Transfer ******/
void ARM7::single_data_transfer(u32 current_arm_instruction)
{
	//TODO - If, PC is src/dest reg, must be PC+12 (or rather PC+4 since GBE+ always returns PC+8 anyway)

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


	//Increment or decrement before transfer if pre-indexing
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

/****** ARM.10 Halfword-Signed Transfer ******/
void ARM7::halfword_signed_transfer(u32 current_arm_instruction)
{
	//TODO - Timings

	//Grab Pre-Post bit - Bit 24
	u8 pre_post = (current_arm_instruction & 0x1000000) ? 1 : 0;

	//Grab Up-Down bit - Bit 23
	u8 up_down = (current_arm_instruction & 0x800000) ? 1 : 0;

	//Grab Immediate-Offset flag - Bit 22
	u8 offset_is_register = (current_arm_instruction & 0x400000) ? 1 : 0;

	//Grab Write-Back bit - Bit 21
	u8 write_back = (current_arm_instruction & 0x200000) ? 1 : 0;

	//Grab Load-Store bit - Bit 20
	u8 load_store = (current_arm_instruction & 0x100000) ? 1 : 0;

	//Grab the Base Register - Bits 16-19
	u8 base_reg = ((current_arm_instruction >> 16) & 0xF);

	//Grab the Destination Register - Bits 12-15
	u8 dest_reg = ((current_arm_instruction >> 12) & 0xF);

	//Grab opcode Bits 5-6
	u8 op = ((current_arm_instruction >> 5) & 0x3);

	//Write-Back is always enabled for Post-Indexing
	if(pre_post == 0) { write_back = 1; }

	u32 base_offset = 0;
	u32 base_addr = get_reg(base_reg);
	u32 value = 0;

	//Determine offset if offset is a register
	if(offset_is_register == 0)
	{
		//Register is Bits 0-3
		base_offset = get_reg((current_arm_instruction & 0xF));

		if((current_arm_instruction & 0xF) == 15) { std::cout<<"CPU::Warning - ARM.10 Offset Register is PC\n"; }
	}

	//Determine offset if offset is immediate
	else
	{
		//Upper 4 bits are Bits 8-11
		base_offset = (current_arm_instruction >> 8) & 0xF;
		base_offset <<= 4;

		//Lower 4 bits are Bits 0-3
		base_offset |= (current_arm_instruction & 0xF);
	}

	//Increment or decrement before transfer if pre-indexing
	if(pre_post == 1) 
	{ 
		if(up_down == 1) { base_addr += base_offset; }
		else { base_addr -= base_offset; } 
	}

	//Perform Load or Store ops
	switch(op)
	{
		//Load-Store unsigned halfword
		case 0x1:
			//Store halfword
			if(load_store == 0)
			{
				value = get_reg(dest_reg);
	
				//If PC is the Destination Register, add 4
				if(dest_reg == 15) { value += 4; }

				value &= 0xFFFF;
				mem->write_u16(base_addr, value);
			}

			//Load halfword
			else
			{
				value = mem->read_u16(base_addr);
				set_reg(dest_reg, value);
			}

			break;

		//Load signed byte (sign extended)
		case 0x2:
			value = mem->read_u8(base_addr);

			if(value & 0x80) { value |= 0xFFFFFF00; }
			set_reg(dest_reg, value);

			break;

		//Load signed halfword (sign extended)
		case 0x3:
			value = mem->read_u16(base_addr);

			if(value & 0x8000) { value |= 0xFFFF0000; }
			set_reg(dest_reg, value);

			break;

		//SWP
		default:
			std::cout<<"This is actually ARM.12 - Single Data Swap\n";
			return;
	}

	//Increment or decrement after transfer if post-indexing
	if(pre_post == 0) 
	{ 
		if(up_down == 1) { base_addr += base_offset; }
		else { base_addr -= base_offset; } 
	}

	//Write-back into base register
	if(write_back == 1){ set_reg(base_reg, base_addr); }
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
			if(r_list & (1 << x)) 
			{
				//Increment before transfer if pre-indexing
				if(pre_post == 1) { base_addr += 4; }

				//Store registers
				if(load_store == 0) { mem->write_u32(base_addr, get_reg(x)); }
			
				//Load registers
				else { set_reg(x, mem->read_u32(base_addr)); }

				//Increment after transfer if post-indexing
				if(pre_post == 0) { base_addr += 4; }
			}

			//Write back the into base register
			if(write_back == 1) { set_reg(base_reg, base_addr); }
		}
	}

	//Load-Store with a descending stack order, Up-Down = 0
	else if((up_down == 0) && (r_list != 0))
	{
		for(int x = 15; x >= 0; x--)
		{
			if(r_list & (1 << x)) 
			{
				//Decrement before transfer if pre-indexing
				if(pre_post == 1) { base_addr -= 4; }

				//Store registers
				if(load_store == 0) { mem->write_u32(base_addr, get_reg(x)); }
			
				//Load registers
				else { set_reg(x, mem->read_u32(base_addr)); }

				//Decrement after transfer if post-indexing
				if(pre_post == 0) { base_addr -= 4; }
			}

			//Write back the into base register
			if(write_back == 1) { set_reg(base_reg, base_addr); }
		}
	}

	//Special case, empty RList
	//Store PC, add or sub 0x40 to base address
	else { std::cout<<"Empty RList not implemented, too lazy atm :p\n"; }
}

		
/****** ARM.12 - Single Data Swap ******/
void ARM7::single_data_swap(u32 current_arm_instruction)
{
	//TODO - Timings

	//Grab source register - Bits 0-3
	u8 src_reg = (current_arm_instruction & 0xF);

	//Grab destination register - Bits 12-15
	u8 dest_reg = ((current_arm_instruction >> 12) & 0xF);
		
	//Grab base register - Bits 16-19
	u8 base_reg = ((current_arm_instruction >> 16) & 0xF);

	//Determine if a byte or word is being swapped - Bit 22
	u8 byte_word = (current_arm_instruction & 0x400000) ? 1 : 0;

	u32 base_addr = get_reg(base_reg);
	u32 dest_value = 0;
	u32 swap_value = 0;

	//Swap a single byte
	if(byte_word == 1)
	{
		//Grab values before swapping
		dest_value = mem->read_u8(base_addr);
		swap_value = (get_reg(src_reg) & 0xFF);

		//Swap the values
		mem->write_u8(base_addr, swap_value);
		set_reg(dest_reg, dest_value);
	}

	//Swap a single word
	else
	{
		//Grab values before swapping
		dest_value = mem->read_u32(base_addr);
		swap_value = get_reg(src_reg);

		//Swap the values
		mem->write_u32(base_addr, swap_value);
		set_reg(dest_reg, dest_value);
	}
}
		




	








