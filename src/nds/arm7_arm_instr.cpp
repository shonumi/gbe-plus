// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : arm_instr.cpp
// Date : July 30, 2016
// Description : ARM7 THUMB instructions
//
// Emulates an ARM7 ARM instructions with equivalent C++

#include "arm7.h"

/****** ARM.3 - Branch and Exchange ******/
void NTR_ARM7::branch_exchange(u32 current_arm_instruction)
{
	//Grab source register - Bits 0-2
	u8 src_reg = (current_arm_instruction & 0xF);

	if(src_reg == 15) { std::cout<<"CPU::ARM9::Warning - ARM.3 Branch and Exchange - R15 used as operand\n"; } 

	u32 result = get_reg(src_reg);
	u8 op = (current_arm_instruction >> 4) & 0xF;

	//Switch to THUMB mode if necessary
	if(result & 0x1) 
	{ 
		arm_mode = THUMB;
		reg.cpsr |= 0x20;
		result &= ~0x1;
	}

	switch(op)
	{
		//Branch
		case 0x1:
			reg.r15 = result;
			needs_flush = true;

			//Clock CPU and controllers - 1N + 2S
			execute_cycles += 3;

			break;

		default:
			std::cout<<"CPU::ARM7::Error - ARM.3 invalid Branch and Exchange opcode : 0x" << std::hex << op << "\n";
			running = false;
			break;
	}
}  

/****** ARM.4 - Branch and Branch with Link ******/
void NTR_ARM7::branch_link(u32 current_arm_instruction)
{
	//Grab offset
	u32 offset = (current_arm_instruction & 0xFFFFFF);
	offset <<= 2;

	//Grab opcode
	u8 op = (current_arm_instruction >> 24) & 0x1;

	u32 final_addr = reg.r15;

	//Add offset as 2s complement if necessary
	if(offset & 0x2000000) { offset |= 0xFC000000; }

	final_addr += offset;

	switch(op)
	{
		//Branch
		case 0x0:
			reg.r15 = final_addr;
			needs_flush = true;

			//Clock CPU and controllers - 1N + 2S
			execute_cycles += 3;

			break;

		//Branch and Link
		case 0x1:
			set_reg(14, (reg.r15 - 4));
			reg.r15 = final_addr;
			needs_flush = true;

			//Clock CPU and controllers - 1N + 2S
			execute_cycles += 3;

			break;
	}
}

/****** ARM.5 Data Processing ******/
void NTR_ARM7::data_processing(u32 current_arm_instruction)
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
	u8 shift_out = 2;

	//Use immediate as operand
	if(use_immediate)
	{
		operand = (current_arm_instruction & 0xFF);
		u8 offset = (current_arm_instruction >> 8) & 0xF;
		
		//Shift immediate - ROR special case - Carry flag not affected
		shift_out = rotate_right_special(operand, offset);
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

			if(src_reg == 15) { input += 4; }
			if((current_arm_instruction & 0xF) == 15) { operand += 4; }
			
			//Valid registers to shift by are R0-R14
			if(((current_arm_instruction >> 8) & 0xF) == 0xF) { std::cout<< "CPU::ARM7::Error - ARM.5 Data Processing - Shifting Register-Operand by PC \n"; running = false; }
		}

		//Shift the register
		switch(shift_type)
		{
			//LSL
			case 0x0:
				if((!shift_immediate) && (offset == 0)) { break; }
				else { shift_out = logical_shift_left(operand, offset); }
				break;

			//LSR
			case 0x1:
				if((!shift_immediate) && (offset == 0)) { break; }
				else { shift_out = logical_shift_right(operand, offset); }
				break;

			//ASR
			case 0x2:
				if((!shift_immediate) && (offset == 0)) { break; }
				else { shift_out = arithmetic_shift_right(operand, offset); }
				break;

			//ROR
			case 0x3:
				if((!shift_immediate) && (offset == 0)) { break; }
				else { shift_out = rotate_right(operand, offset); }
				break;
		}
		
		//Clock CPU and controllers - 1I
		execute_cycles++;
	}		

	//TODO - When op is 0x8 through 0xB, make sure Bit 20 is 1 (rather force it? Unsure)
	//TODO - 2nd Operand for TST/TEQ/CMP/CMN must be R0 (rather force it to be R0)
	//TODO - See GBATEK - S=1, with unused Rd bits=1111b

	//Destination register is R15
	if(dest_reg == 15)
	{
		//Clock CPU and controllers - 1S + 1N
		execute_cycles += 2;
		
		//When the set condition parameter is 1 and destination register is R15, change CPSR to SPSR
		if(set_condition)
		{
			reg.cpsr = get_spsr();
			set_condition = false;

			//Set the CPU mode accordingly
			switch((reg.cpsr & 0x1F))
			{
				case 0x10: current_cpu_mode = USR; break;
				case 0x11: current_cpu_mode = FIQ; break;
				case 0x12: current_cpu_mode = IRQ; break;
				case 0x13: current_cpu_mode = SVC; break;
				case 0x17: current_cpu_mode = ABT; break;
				case 0x1B: current_cpu_mode = UND; break;
				case 0x1F: current_cpu_mode = SYS; break;
				default: std::cout<<"CPU::ARM9::Warning - ARM.5 CPSR setting unknown CPU mode -> 0x" << std::hex << (reg.cpsr & 0x1F) << "\n";
			}

			//Switch to ARM or THUMB mode if necessary
			arm_mode = (reg.cpsr & 0x20) ? THUMB : ARM;
		}

		needs_flush = true; 
	}

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
			if(set_condition) { update_condition_arithmetic(operand, input, result, false); }
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
			//Use the current Carry Flag for this math op
			shift_out = (reg.cpsr & CPSR_C_FLAG) ? 1 : 0;

			result = (input + operand + shift_out);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, operand, result, true); }
			break;

		//SBC
		case 0x6:
			//Use the current Carry Flag for this math op
			shift_out = (reg.cpsr & CPSR_C_FLAG) ? 1 : 0;

			result = (input - operand + shift_out - 1);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic(input, (operand + shift_out - 1), result, false); }
			break;

		//RSC
		case 0x7:
			//Use the current Carry Flag for this math op
			shift_out = (reg.cpsr & CPSR_C_FLAG) ? 1 : 0;

			result = (operand - input + shift_out - 1);
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_arithmetic((operand + shift_out - 1), input, result, false); }
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
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;

		//MVN
		case 0xF:
			result = ~operand;
			set_reg(dest_reg, result);

			//Update condtion codes
			if(set_condition) { update_condition_logical(result, shift_out); }
			break;
	}

	//Clock CPU and controllers - 1S
	execute_cycles++;
}

/****** ARM.6 PSR Transfer ******/
void NTR_ARM7::psr_transfer(u32 current_arm_instruction)
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
				u8 dest_reg = ((current_arm_instruction >> 12) & 0xF);

				if(dest_reg == 15) { std::cout<<"CPU::ARM7::Warning - ARM.6 R15 used as Destination Register \n"; }

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
					std::cout<<"CPU::ARM7::Warning - ARM.6 MSR enabled access to Status Field \n";
				}

				//Extension field - Bit 17
				if(current_arm_instruction & 0x20000) 
				{ 
					op_field_mask |= 0x0000FF00;
					std::cout<<"CPU::ARM7::Warning - ARM.6 MSR enabled access to Extension Field \n";
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

					if(src_reg == 15) { std::cout<<"CPU::ARM7::Warning - ARM.6 R15 used as Source Register \n"; }

					input = get_reg(src_reg);
					input &= op_field_mask;
				}

				//Write into CPSR
				if(psr == 0) 
				{ 
					reg.cpsr &= ~op_field_mask;
					reg.cpsr |= input;

					//Set the CPU mode accordingly
					switch((reg.cpsr & 0x1F))
					{
						case 0x10: current_cpu_mode = USR; break;
						case 0x11: current_cpu_mode = FIQ; break;
						case 0x12: current_cpu_mode = IRQ; break;
						case 0x13: current_cpu_mode = SVC; break;
						case 0x17: current_cpu_mode = ABT; break;
						case 0x1B: current_cpu_mode = UND; break;
						case 0x1F: current_cpu_mode = SYS; break;
						default: std::cout<<"CPU::ARM7::Warning - ARM.6 CPSR setting unknown CPU mode\n";
					}
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
	execute_cycles++;
} 

/****** ARM.7 Multiply and Multiply-Accumulate ******/
void NTR_ARM7::multiply(u32 current_arm_instruction)
{
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
	if(op_rm_reg == 15) { std::cout<<"CPU::ARM7::Warning - ARM.7 R15 used as Rm\n"; }
	if(op_rs_reg == 15) { std::cout<<"CPU::ARM7::Warning - ARM.7 R15 used as Rs\n"; }
	if(accu_reg == 15) { std::cout<<"CPU::ARM7::Warning - ARM.7 R15 used as Rn\n"; }
	if(dest_reg == 15) { std::cout<<"CPU::ARM7::Warning - ARM.7 R15 used as Rd\n"; }

	u32 Rm = get_reg(op_rm_reg);
	u32 Rs = get_reg(op_rs_reg);
	u32 Rn = get_reg(accu_reg);
	u32 Rd = get_reg(dest_reg);

	u64 value_64 = 1;
	u64 hi_lo = 0;
	s64 value_s64 = 1;
	u32 value_32 = 0;

	u8 m_count = 0;

	//Calculate m_count based on Rs
	switch(op_code)
	{
		case 0x0:
		case 0x1:
		case 0x6:
		case 0x7:
			//Get count of most significant bits that are all 0 or all 1 for timing
			if((Rs >> 8) == 0xFFFFFF || 0) { m_count = 1; }
			else if((Rs >> 16) == 0xFFFF || 0) { m_count = 2; }
			else if((Rs >> 24) == 0xFF || 0) { m_count = 3; }
			else { m_count = 4; }
			
			break;

		case 0x4:
		case 0x5:
			//Get count of most significant bits that are all 0 or all 1 for timing
			if((Rs >> 8) == 0) { m_count = 1; }
			else if((Rs >> 16) == 0) { m_count = 2; }
			else if((Rs >> 24) == 0) { m_count = 3; }
			else { m_count = 4; }

			break;
	}

	//Perform multiplication ops
	switch(op_code)
	{
		//MUL
		case 0x0:
			value_32 = (Rm * Rs);
			set_reg(dest_reg, value_32);

			if(set_condition)
			{
				//Negative flag
				if(value_32 & 0x80000000) { reg.cpsr |= CPSR_N_FLAG; }
				else { reg.cpsr &= ~CPSR_N_FLAG; }

				//Zero flag
				if(value_32 == 0) { reg.cpsr |= CPSR_Z_FLAG; }
				else { reg.cpsr &= ~CPSR_Z_FLAG; }
			}

			//Clock CPU and controllers - 1S + (m)I
			execute_cycles += (1 + m_count);
			
			break;

		//MLA
		case 0x1:
			value_32 = (Rm * Rs) + Rn;
			set_reg(dest_reg, value_32);

			if(set_condition)
			{
				//Negative flag
				if(value_32 & 0x80000000) { reg.cpsr |= CPSR_N_FLAG; }
				else { reg.cpsr &= ~CPSR_N_FLAG; }

				//Zero flag
				if(value_32 == 0) { reg.cpsr |= CPSR_Z_FLAG; }
				else { reg.cpsr &= ~CPSR_Z_FLAG; }
			}

			//Clock CPU and controllers - 1S + (m + 1)I
			execute_cycles += (2 + m_count);
			
			break;

		//UMULL
		case 0x4:
			value_64 = (value_64 * Rm * Rs);
			
			//Set Rn to low 32-bits, Rd to high 32-bits
			Rn = (value_64 & 0xFFFFFFFF);
			Rd = (value_64 >> 32);

			set_reg(accu_reg, Rn);
			set_reg(dest_reg, Rd);

			if(set_condition)
			{
				//Negative flag
				if(value_64 & 0x8000000000000000) { reg.cpsr |= CPSR_N_FLAG; }
				else { reg.cpsr &= ~CPSR_N_FLAG; }

				//Zero flag
				if(value_64 == 0) { reg.cpsr |= CPSR_Z_FLAG; }
				else { reg.cpsr &= ~CPSR_Z_FLAG; }
			}

			//Clock CPU and controllers - 1S + (m + 1)I
			execute_cycles += (2 + m_count);

			break;

		//UMLAL
		case 0x5:
			//This looks weird, but it is a workaround for compilers that support 64-bit unsigned ints, but complain about shifts greater than 32
			hi_lo = Rd;
			hi_lo <<= 16;
			hi_lo <<= 16;
			hi_lo |= Rn;

			value_64 = (value_64 * Rm * Rs) + hi_lo;
			
			//Set Rn to low 32-bits, Rd to high 32-bits
			Rn = (value_64 & 0xFFFFFFFF);
			Rd = (value_64 >> 32);

			set_reg(accu_reg, Rn);
			set_reg(dest_reg, Rd);

			if(set_condition)
			{
				//Negative flag
				if(value_64 & 0x8000000000000000) { reg.cpsr |= CPSR_N_FLAG; }
				else { reg.cpsr &= ~CPSR_N_FLAG; }

				//Zero flag
				if(value_64 == 0) { reg.cpsr |= CPSR_Z_FLAG; }
				else { reg.cpsr &= ~CPSR_Z_FLAG; }
			}

			//Clock CPU and controllers - 1S + (m + 2)I
			execute_cycles += (3 + m_count);

			break;


		//SMULL
		case 0x6:
			//Messy C++ casting... It works though, and this is what we need
			value_s64 = (value_s64 * (s32)Rm * (s32)Rs);
			value_64 = value_s64;

			//Set Rn to low 32-bits, Rd to high 32-bits
			Rn = (value_s64 & 0xFFFFFFFF);
			Rd = (value_s64 >> 32);

			set_reg(accu_reg, Rn);
			set_reg(dest_reg, Rd);

			if(set_condition)
			{
				//Negative flag
				if(value_s64 & 0x8000000000000000) { reg.cpsr |= CPSR_N_FLAG; }
				else { reg.cpsr &= ~CPSR_N_FLAG; }

				//Zero flag
				if(value_s64 == 0) { reg.cpsr |= CPSR_Z_FLAG; }
				else { reg.cpsr &= ~CPSR_Z_FLAG; }
			}

			//Clock CPU and controllers - 1S + (m + 1)I
			execute_cycles += (2 + m_count);

			break;

		//SMLAL
		case 0x7:
			//This looks weird, but it is a workaround for compilers that support 64-bit unsigned ints, but complain about shifts greater than 32
			hi_lo = Rd;
			hi_lo <<= 16;
			hi_lo <<= 16;
			hi_lo |= Rn;

			//Messy C++ casting... It works though, and this is what we need
			value_s64 = (value_s64 * (s32)Rm * (s32)Rs) + hi_lo;
			value_64 = value_s64;

			//Set Rn to low 32-bits, Rd to high 32-bits
			Rn = (value_s64 & 0xFFFFFFFF);
			Rd = (value_s64 >> 32);

			set_reg(accu_reg, Rn);
			set_reg(dest_reg, Rd);

			if(set_condition)
			{
				//Negative flag
				if(value_s64 & 0x8000000000000000) { reg.cpsr |= CPSR_N_FLAG; }
				else { reg.cpsr &= ~CPSR_N_FLAG; }

				//Zero flag
				if(value_s64 == 0) { reg.cpsr |= CPSR_Z_FLAG; }
				else { reg.cpsr &= ~CPSR_Z_FLAG; }
			}

			//Clock CPU and controllers - 1S + (m + 2)I
			execute_cycles += (3 + m_count);

			break;
			

		default: std::cout<<"CPU::ARM7::Warning:: - ARM.7 Invalid or unimplemented opcode : " << std::hex << (int)op_code << "\n"; std::cout<<"OP -> 0x" << current_arm_instruction << "\n";
			 std::cout<<"PC -> 0x" << std::hex << reg.r15 << "\n";
	}

	//ARMv4 destorys Carry Flag after all supported multiply operations
	reg.cpsr &= ~CPSR_C_FLAG;
}
			
/****** ARM.9 Single Data Transfer ******/
void NTR_ARM7::single_data_transfer(u32 current_arm_instruction)
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


	//Increment or decrement before transfer if pre-indexing
	if(pre_post == 1) 
	{ 
		if(up_down == 1) { base_addr += base_offset; }
		else { base_addr -= base_offset; } 
	}

	//Store Byte or Word
	if(load_store == 0) 
	{
		if(byte_word == 1)
		{
			value = get_reg(dest_reg);
			if(dest_reg == 15) { value += 4; }
			value &= 0xFF;
			mem_check_8(base_addr, value, false);

			//Clock CPU and controllers - 2N
			execute_cycles = 1 + get_access_time(base_addr, DATA_16);
		}

		else
		{
			value = get_reg(dest_reg);
			if(dest_reg == 15) { value += 4; }
			mem->write_u32(base_addr, value);

			//Clock CPU and controllers - 2N
			execute_cycles += 1 + get_access_time(base_addr, DATA_32);
		}
	}

	//Load Byte or Word
	else
	{
		if(byte_word == 1)
		{
			value = mem->read_u8(base_addr);

			//Clock CPU and controllers - 1I + 1N
			execute_cycles = 1 + get_access_time(base_addr, DATA_16); 

			set_reg(dest_reg, value);
		}

		else
		{
			mem_check_32(base_addr, value, true);

			//Clock CPU and controllers - 1I + 1N
			execute_cycles = 1 + get_access_time(base_addr, DATA_32); 

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
	if((pre_post == 0) && (base_reg != dest_reg)) { set_reg(base_reg, base_addr); }
	else if((pre_post == 1) && (write_back == 1) && (base_reg != dest_reg)) { set_reg(base_reg, base_addr); }

	//Timings for LDR - PC
	if((dest_reg == 15) && (load_store == 1)) 
	{
		//Switch to THUMB mode if necessary
		if(reg.r15 & 0x1) 
		{ 
			arm_mode = THUMB;
			reg.cpsr |= 0x20;
			reg.r15 &= ~0x1;
		}

		//Clock CPU and controllers - 2S + 1N
		execute_cycles += 3;
		needs_flush = true;
	}

	//Timings for LDR - No PC
	else if((dest_reg != 15) && (load_store == 1))
	{
		//Clock CPU and controllers - 1S
		execute_cycles++;
	}
}

/****** ARM.10 Halfword-Signed Transfer ******/
void NTR_ARM7::halfword_signed_transfer(u32 current_arm_instruction)
{
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

		if((current_arm_instruction & 0xF) == 15) { std::cout<<"CPU::ARM7::Warning - ARM.10 Offset Register is PC\n"; }
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

				//Clock CPU and controllers - 2N
				execute_cycles = 1 + get_access_time(base_addr, DATA_16);
			}

			//Load halfword
			else
			{
				value = mem->read_u16(base_addr);
				set_reg(dest_reg, value);

				//Clock CPU and controllers - 1I + 1N
				execute_cycles = 1 + get_access_time(base_addr, DATA_16);
			}

			break;

		//Load signed byte (sign extended)
		case 0x2:
			value = mem->read_u8(base_addr);

			if(value & 0x80) { value |= 0xFFFFFF00; }
			set_reg(dest_reg, value);

			//Clock CPU and controllers - 1I + 1N
			execute_cycles = 1 + get_access_time(base_addr, DATA_16);

			break;

		//Load signed halfword (sign extended)
		case 0x3:
			value = mem->read_u16(base_addr);

			if(value & 0x8000) { value |= 0xFFFF0000; }
			set_reg(dest_reg, value);

			//Clock CPU and controllers - 1I + 1N
			execute_cycles = 1 + get_access_time(base_addr, DATA_16);

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
	if((write_back == 1) && (base_reg != dest_reg)) { set_reg(base_reg, base_addr); }

	//Timings for LDR - PC
	if((dest_reg == 15) && (load_store == 1)) 
	{
		//Switch to THUMB mode if necessary
		if(reg.r15 & 0x1) 
		{ 
			arm_mode = THUMB;
			reg.cpsr |= 0x20;
			reg.r15 &= ~0x1;
		}

		//Clock CPU and controllers - 2S + 1N
		execute_cycles += 3;
		needs_flush = true;
	}

	//Timings for LDR - No PC
	else if((dest_reg != 15) && (load_store == 1))
	{
		//Clock CPU and controllers - 1S
		execute_cycles++;
	}
}

/****** ARM.11 Block Data Transfer ******/
void NTR_ARM7::block_data_transfer(u32 current_arm_instruction)
{
	//TODO - Clock cycles
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
	if(base_reg == 15) { std::cout<<"CPU::ARM7::Warning - ARM.11 R15 used as Base Register \n"; }

	//Force USR mode if PSR bit is set
	cpu_modes temp_mode = current_cpu_mode;
	if(psr) { current_cpu_mode = USR; }

	u32 base_addr = get_reg(base_reg);
	u32 old_base = base_addr;
	u8 transfer_reg = 0xFF;

	//Find out the first register in the Register List
	for(int x = 0; x < 16; x++)
	{
		if(r_list & (1 << x))
		{
			transfer_reg = x;
			x = 0xFF;
			break;
		}
	}

	//Clock CPU and controllers
	execute_cycles += (load_store) ? 1 : 2;

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
				if(load_store == 0) 
				{
					if((x == transfer_reg) && (base_reg == transfer_reg)) { mem->write_u32(base_addr, old_base); }
					else { mem->write_u32(base_addr, get_reg(x)); }
				}
			
				//Load registers
				else 
				{
					if((x == transfer_reg) && (base_reg == transfer_reg)) { write_back = 0; }
					set_reg(x, mem->read_u32(base_addr));
					if(x == 15) { needs_flush = true; } 
				}

				//Clock CPU and controllers
				execute_cycles += get_access_time(base_addr, DATA_32);

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
				if(load_store == 0) 
				{ 
					if((x == transfer_reg) && (base_reg == transfer_reg)) { mem->write_u32(base_addr, old_base); }
					else { mem->write_u32(base_addr, get_reg(x)); }
				}
			
				//Load registers
				else 
				{
					if((x == transfer_reg) && (base_reg == transfer_reg)) { write_back = 0; }
					set_reg(x, mem->read_u32(base_addr));
					if(x == 15) { needs_flush = true; } 
				}

				//Clock CPU and controllers
				execute_cycles += get_access_time(base_addr, DATA_32);

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

	//Restore CPU mode if PSR bit is set
	if(psr)
	{
		current_cpu_mode = temp_mode;

		//Also set CPSR to current SPSR if loading R15
		if(needs_flush)
		{
			reg.cpsr = get_spsr();

			//Set the CPU mode accordingly
			switch((reg.cpsr & 0x1F))
			{
				case 0x10: current_cpu_mode = USR; break;
				case 0x11: current_cpu_mode = FIQ; break;
				case 0x12: current_cpu_mode = IRQ; break;
				case 0x13: current_cpu_mode = SVC; break;
				case 0x17: current_cpu_mode = ABT; break;
				case 0x1B: current_cpu_mode = UND; break;
				case 0x1F: current_cpu_mode = SYS; break;
				default: std::cout<<"CPU::ARM9::Warning - ARM.5 CPSR setting unknown CPU mode -> 0x" << std::hex << (reg.cpsr & 0x1F) << "\n";
			}
		}
	}

	//Switch to THUMB mode if necessary
	if((needs_flush) && (reg.r15 & 0x1)) 
	{
		arm_mode = THUMB;
		reg.cpsr |= 0x20;
		reg.r15 &= ~0x1;
	}

	//Timings for LDM PC
	if(need_flush && load_store) 
	{
		//Clock CPU and controllers - 2S + 1N
		execute_cycles += 3;
	}

	//Timings for LDR - No PC
	else if(load_store)
	{
		//Clock CPU and controllers - 1S
		execute_cycles++;
	}
}
		
/****** ARM.12 - Single Data Swap ******/
void NTR_ARM7::single_data_swap(u32 current_arm_instruction)
{
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

	//Clock CPU and controllers - 1S + 1I
	execute_cycles += 2;

	//Swap a single byte
	if(byte_word == 1)
	{
		//Grab values before swapping
		dest_value = mem->read_u8(base_addr);
		swap_value = (get_reg(src_reg) & 0xFF);

		//Clock CPU and controllers - 1N
		execute_cycles += get_access_time(base_addr, DATA_16);

		//Swap the values
		mem->write_u8(base_addr, swap_value);
		set_reg(dest_reg, dest_value);

		//Clock CPU and controllers - 1N
		execute_cycles += get_access_time(base_addr, DATA_16);
	}

	//Swap a single word
	else
	{
		//Grab values before swapping
		dest_value = mem->read_u32(base_addr);
		swap_value = get_reg(src_reg);

		//Clock CPU and controllers - 1N
		execute_cycles += get_access_time(base_addr, DATA_32);

		//Swap the values
		mem->write_u32(base_addr, swap_value);
		set_reg(dest_reg, dest_value);

		//Clock CPU and controllers - 1N
		execute_cycles += get_access_time(base_addr, DATA_32);
	}
}

/****** ARM.13 - Software Interrupt ******/
void NTR_ARM7::software_interrupt_breakpoint(u32 current_arm_instruction)
{
	//TODO - Realistic timings
	//TODO - LLE version of SWIs

	//Grab SWI comment - Bits 0-23
	u32 comment = (current_arm_instruction & 0xFFFFFF);
	comment >>= 16;

	process_swi(comment);

	//Clock CPU and controllers
	execute_cycles += 1;
}
