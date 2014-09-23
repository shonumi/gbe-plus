// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : arm7.cpp
// Date : April 09, 2014
// Description : ARM7TDMI emulator
//
// Emulates an ARM7TDMI CPU in software
// This is basically the core of the GBA

#include "arm7.h"

/****** CPU Constructor ******/
ARM7::ARM7()
{
	reset();
	std::cout<<"CPU::Initialized\n";
}

/****** CPU Destructor ******/
ARM7::~ARM7()
{
	std::cout<<"CPU::Shutdown\n";
}

/****** CPU Reset ******/
void ARM7::reset()
{
	reg.r0 = reg.r1 = reg.r2 = reg.r3 = reg.r4 = reg.r5 = reg.r6 = reg.r7 = reg.r8 = reg.r9 = reg.r10 = reg.r11 = reg.r12 = reg.r14 = 0;
	reg.r13 = reg.r13_fiq = reg.r13_svc = reg.r13_abt = reg.r13_irq = reg.r13_und = 0x03007F00;
	reg.r15 = 0x8000000;
	reg.cpsr = 0x5F;

	reg.r8_fiq = reg.r9_fiq = reg.r10_fiq = reg.r11_fiq = reg.r12_fiq = reg.r14_fiq = reg.spsr_fiq = 0;
	reg.r14_svc = reg.spsr_svc = 0;
	reg.r14_abt = reg.spsr_abt = 0;
	reg.r14_irq = reg.spsr_irq = 0;
	reg.r14_und = reg.spsr_und = 0;

	running = false;
	bool in_interrupt = false;

	arm_mode = ARM;
	current_cpu_mode = SYS;

	debug_message = 0xFF;
	debug_code = 0;

	flush_pipeline();
	mem = NULL;
}

/****** CPU register getter ******/
u32 ARM7::get_reg(u8 g_reg) const
{
	switch(g_reg)
	{
		case 0: return reg.r0; break;
		case 1: return reg.r1; break;
		case 2: return reg.r2; break;
		case 3: return reg.r3; break;
		case 4: return reg.r4; break;
		case 5: return reg.r5; break;
		case 6: return reg.r6; break;
		case 7: return reg.r7; break;
		
		case 8: 
			switch(current_cpu_mode)
			{
				case FIQ: return reg.r8_fiq; break;
				default: return reg.r8; break;
			}
			break;

		case 9: 
			switch(current_cpu_mode)
			{
				case FIQ: return reg.r9_fiq; break;
				default: return reg.r9; break;
			}
			break;

		case 10: 
			switch(current_cpu_mode)
			{
				case FIQ: return reg.r10_fiq; break;
				default: return reg.r10; break;
			}
			break;

		case 11: 
			switch(current_cpu_mode)
			{
				case FIQ: return reg.r11_fiq; break;
				default: return reg.r11; break;
			}
			break;

		case 12: 
			switch(current_cpu_mode)
			{
				case FIQ: return reg.r12_fiq; break;
				default: return reg.r12; break;
			}
			break;

		case 13: 
			switch(current_cpu_mode)
			{
				case SYS: return reg.r13; break;
				case FIQ: return reg.r13_fiq; break;
				case SVC: return reg.r13_svc; break;
				case ABT: return reg.r13_abt; break;
				case IRQ: return reg.r13_irq; break;
				case UND: return reg.r13_und; break;
			}
			break;

		case 14: 
			switch(current_cpu_mode)
			{
				case SYS: return reg.r14; break;
				case FIQ: return reg.r14_fiq; break;
				case SVC: return reg.r14_svc; break;
				case ABT: return reg.r14_abt; break;
				case IRQ: return reg.r14_irq; break;
				case UND: return reg.r14_und; break;
			}
			break;

		case 15: return reg.r15; break;
	}
}

/****** CPU register setter ******/
void ARM7::set_reg(u8 s_reg, u32 value)
{
	switch(s_reg)
	{
		case 0: reg.r0 = value; break;
		case 1: reg.r1 = value; break;
		case 2: reg.r2 = value; break;
		case 3: reg.r3 = value; break;
		case 4: reg.r4 = value; break;
		case 5: reg.r5 = value; break;
		case 6: reg.r6 = value; break;
		case 7: reg.r7 = value; break;
		
		case 8: 
			switch(current_cpu_mode)
			{
				case FIQ: reg.r8_fiq = value; break;
				default: reg.r8 = value; break;
			}
			break;

		case 9: 
			switch(current_cpu_mode)
			{
				case FIQ: reg.r9_fiq = value; break;
				default: reg.r9 = value; break;
			}
			break;

		case 10: 
			switch(current_cpu_mode)
			{
				case FIQ: reg.r10_fiq = value; break;
				default: reg.r10 = value; break;
			}
			break;

		case 11: 
			switch(current_cpu_mode)
			{
				case FIQ: reg.r11_fiq = value; break;
				default: reg.r11 = value; break;
			}
			break;

		case 12: 
			switch(current_cpu_mode)
			{
				case FIQ: reg.r12_fiq = value; break;
				default: reg.r12 = value; break;
			}
			break;

		case 13: 
			switch(current_cpu_mode)
			{
				case SYS: reg.r13 = value; break;
				case FIQ: reg.r13_fiq = value; break;
				case SVC: reg.r13_svc = value; break;
				case ABT: reg.r13_abt = value; break;
				case IRQ: reg.r13_irq = value; break;
				case UND: reg.r13_und = value; break;
			}
			break;

		case 14: 
			switch(current_cpu_mode)
			{
				case SYS: reg.r14 = value; break;
				case FIQ: reg.r14_fiq = value; break;
				case SVC: reg.r14_svc = value; break;
				case ABT: reg.r14_abt = value; break;
				case IRQ: reg.r14_irq = value; break;
				case UND: reg.r14_und = value; break;
			}
			break;

		case 15: reg.r15 = value; break;
	}
}

/****** Saved Program Status Register getter ******/
u32 ARM7::get_spsr() const
{
	switch(current_cpu_mode)
	{
		case SYS: std::cout<<"CPU::Warning - Tried to read SPSR in USER-SYSTEM mode\n"; return reg.cpsr; break;
		case FIQ: return reg.spsr_fiq; break;
		case SVC: return reg.spsr_svc; break;
		case ABT: return reg.spsr_abt; break;
		case IRQ: return reg.spsr_irq; break;
		case UND: return reg.spsr_und; break;
	}
}

/****** Saved Program Status Register setter ******/
void ARM7::set_spsr(u32 value)
{
	switch(current_cpu_mode)
	{
		case SYS: std::cout<<"CPU::Warning - Tried to write SPSR in USER-SYSTEM mode\n"; break;
		case FIQ: reg.spsr_fiq = value; break;
		case SVC: reg.spsr_svc = value; break;
		case ABT: reg.spsr_abt = value; break;
		case IRQ: reg.spsr_irq = value; break;
		case UND: reg.spsr_und = value; break;
	}
}

/****** Fetch ARM instruction ******/
void ARM7::fetch()
{
	//Fetch THUMB instructions
	if(arm_mode == THUMB)
	{
		//Read 16-bit THUMB instruction
		instruction_pipeline[pipeline_pointer] = mem->read_u16(reg.r15);

		//Set the operation to perform as UNDEFINED until decoded
		instruction_operation[pipeline_pointer] = UNDEFINED;
	}

	//Fetch ARM instructions
	else if(arm_mode == ARM)
	{
		//Read 32-bit ARM instruction
		instruction_pipeline[pipeline_pointer] = mem->read_u32(reg.r15);

		//Set the operation to perform as UNDEFINED until decoded
		instruction_operation[pipeline_pointer] = UNDEFINED;
	}
}

/****** Decode ARM instruction ******/
void ARM7::decode()
{
	u8 pipeline_id = (pipeline_pointer + 2) % 3;

	if(instruction_operation[pipeline_id] == PIPELINE_FILL) { return; }

	//Decode THUMB instructions
	if(arm_mode == THUMB)
	{
		u16 current_instruction = instruction_pipeline[pipeline_id];
		
		if(((current_instruction >> 13) == 0) && (((current_instruction >> 11) & 0x7) != 3))
		{
			//THUMB_1
			instruction_operation[pipeline_id] = THUMB_1;
		}

		else if(((current_instruction >> 11) & 0x1F) == 3)
		{
			//THUMB_2
			instruction_operation[pipeline_id] = THUMB_2;
		}

		else if((current_instruction >> 13) == 1)
		{
			//THUMB_3
			instruction_operation[pipeline_id] = THUMB_3;
		}

		else if(((current_instruction >> 10) & 0x3F) == 0x10)
		{
			//THUMB_4
			instruction_operation[pipeline_id] = THUMB_4;
		}

		else if(((current_instruction >> 10) & 0x3F) == 0x11)
		{
			//THUMB_5
			instruction_operation[pipeline_id] = THUMB_5;
		}

		else if(((current_instruction >> 13) & 0x7) == 3)
		{
			//THUMB_9
			instruction_operation[pipeline_id] = THUMB_9;
		}

		else if((current_instruction >> 12) == 8)
		{
			//THUMB_10
			instruction_operation[pipeline_id] = THUMB_10;
		}

		else if((current_instruction >> 12) == 13)
		{
			//THUMB_16
			instruction_operation[pipeline_id] = THUMB_16;
		}
	}

	//Decode ARM instructions
	if(arm_mode == ARM)
	{
		u32 current_instruction = instruction_pipeline[pipeline_id];

		if(((current_instruction >> 8) & 0xFFFFF) == 0x12FFF)
		{
			//ARM_3
			instruction_operation[pipeline_id] = ARM_3;
		}

		else if(((current_instruction >> 25) & 0x7) == 0x5)
		{
			//ARM_4
			instruction_operation[pipeline_id] = ARM_4;
		}

		else if((current_instruction & 0xD900000) == 0x1000000) 
		{
			//ARM_6
			instruction_operation[pipeline_id] = ARM_6;
		}

		else if(((current_instruction >> 26) & 0x3) == 0x0)
		{
			//ARM_5
			instruction_operation[pipeline_id] = ARM_5;
		}

		else if(((current_instruction >> 26) & 0x3) == 0x1)
		{
			//ARM_9
			instruction_operation[pipeline_id] = ARM_9;
		}

		else if(((current_instruction >> 25) & 0x7) == 0x4)
		{
			//ARM_11
			instruction_operation[pipeline_id] = ARM_11;
		}
	}
}

/****** Execute ARM instruction ******/
void ARM7::execute()
{
	u8 pipeline_id = (pipeline_pointer + 1) % 3;

	if(instruction_operation[pipeline_id] == PIPELINE_FILL) 
	{
		debug_message = 0xFF; 
		return; 
	}

	//Execute THUMB instruction
	if(arm_mode == THUMB)
	{
		switch(instruction_operation[pipeline_id])
		{
			case THUMB_1:
				move_shifted_register(instruction_pipeline[pipeline_id]);
				debug_message = 0x0; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_2:
				add_sub_immediate(instruction_pipeline[pipeline_id]);
				debug_message = 0x1; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_3:
				mcas_immediate(instruction_pipeline[pipeline_id]);
				debug_message = 0x2; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_4:
				alu_ops(instruction_pipeline[pipeline_id]);
				debug_message = 0x3; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_5:
				hireg_bx(instruction_pipeline[pipeline_id]);
				debug_message = 0x4; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_9:
	 			load_store_imm_offset(instruction_pipeline[pipeline_id]);
				debug_message = 0x8; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_10:
				load_store_halfword(instruction_pipeline[pipeline_id]);
				debug_message = 0x9; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_16:
				conditional_branch(instruction_pipeline[pipeline_id]);
				debug_message = 0xF; debug_code = instruction_pipeline[pipeline_id];
				break;

			default:
				debug_message = 0x13; debug_code = instruction_pipeline[pipeline_id];
				running = false;
				break;
		}
	}

	//Execute ARM instruction
	else if(arm_mode == ARM)
	{
		//Conditionally execute ARM instruction
		if(check_condition(instruction_pipeline[pipeline_id]))
		{

			switch(instruction_operation[pipeline_id])
			{
				case ARM_3:
					branch_exchange(instruction_pipeline[pipeline_id]);
					debug_message = 0x14; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_4:
					branch_link(instruction_pipeline[pipeline_id]);
					debug_message = 0x15; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_5:
					data_processing(instruction_pipeline[pipeline_id]);
					debug_message = 0x16; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_6:
					psr_transfer(instruction_pipeline[pipeline_id]);
					debug_message = 0x17; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_9:
					single_data_transfer(instruction_pipeline[pipeline_id]);
					debug_message = 0x19; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_11:
					block_data_transfer(instruction_pipeline[pipeline_id]);
					debug_message = 0x1B; debug_code = instruction_pipeline[pipeline_id];
					break;

				default:
					debug_message = 0x1E; debug_code = instruction_pipeline[pipeline_id];
					running = false;
					break;
			}
		}

		else { debug_message = 0x1E; debug_code = instruction_pipeline[pipeline_id]; }
	}

}

/****** Flush the pipeline - Called when branching or resetting ******/
void ARM7::flush_pipeline()
{
	needs_flush = false;
	pipeline_pointer = 0;
	instruction_pipeline[0] = instruction_pipeline[1] = instruction_pipeline[2] = 0;
	instruction_operation[0] = instruction_operation[1] = instruction_operation[2] = PIPELINE_FILL;
}

/****** Updates the PC after each fetch-decode-execute ******/
void ARM7::update_pc()
{
	if(arm_mode == ARM)
	{
		reg.r15 += 4;
	}

	else if(arm_mode == THUMB)
	{
		reg.r15 += 2;
	}
}

/****** Check conditional code ******/
bool ARM7::check_condition(u32 current_arm_instruction) const
{
	switch(current_arm_instruction >> 28)
	{
		//EQ
		case 0x0:
			if(reg.cpsr & CPSR_Z_FLAG) { return true; }
			else { return false; }
			break;

		//NE
		case 0x1:
			if(reg.cpsr & CPSR_Z_FLAG) { return false; }
			else { return true; }
			break;

		//CS
		case 0x2:
			if(reg.cpsr & CPSR_C_FLAG) { return true; }
			else { return false; }
			break;

		//CC
		case 0x3:
			if(reg.cpsr & CPSR_C_FLAG) { return false; }
			else { return true; }
			break;

		//MI
		case 0x4:
			if(reg.cpsr & CPSR_N_FLAG) { return true; }
			else { return false; }
			break;

		//PL
		case 0x5:
			if(reg.cpsr & CPSR_N_FLAG) { return false; }
			else { return true; }
			break;

		//VS
		case 0x6:
			if(reg.cpsr & CPSR_V_FLAG) { return true; }
			else { return false; }
			break;

		//VC
		case 0x7:
			if(reg.cpsr & CPSR_V_FLAG) { return false; }
			else { return true; }
			break;

		//HI
		case 0x8:
			if((reg.cpsr & CPSR_C_FLAG) && ((reg.cpsr & CPSR_Z_FLAG) == 0)) { return true; }
			else { return false; }
			break;

		//LS
		case 0x9:
			if((reg.cpsr & CPSR_Z_FLAG) && ((reg.cpsr & CPSR_C_FLAG) == 0)) { return true; }
			else { return false; }
			break;

		//GE
		case 0xA:
			{
				u8 n = (reg.cpsr & CPSR_N_FLAG) ? 1 : 0;
				u8 v = (reg.cpsr & CPSR_V_FLAG) ? 1 : 0;

				if(n == v) { return true; }
				else { return false; }
			}
			break;

		//LT
		case 0xB:
			{
				u8 n = (reg.cpsr & CPSR_N_FLAG) ? 1 : 0;
				u8 v = (reg.cpsr & CPSR_V_FLAG) ? 1 : 0;

				if(n != v) { return true; }
				else { return false; }
			}
			break;

		//GT
		case 0xC:
			{
				u8 n = (reg.cpsr & CPSR_N_FLAG) ? 1 : 0;
				u8 v = (reg.cpsr & CPSR_V_FLAG) ? 1 : 0;
				u8 z = (reg.cpsr & CPSR_Z_FLAG) ? 1 : 0;

				if((z == 0) && (n == v)) { return true; }
				else { return false; }
			}
			break;

		//LE
		case 0xD:
			{
				u8 n = (reg.cpsr & CPSR_N_FLAG) ? 1 : 0;
				u8 v = (reg.cpsr & CPSR_V_FLAG) ? 1 : 0;
				u8 z = (reg.cpsr & CPSR_Z_FLAG) ? 1 : 0;

				if((z == 1) && (n != v)) { return true; }
				else { return false; }
			}
			break;

		//AL
		case 0xE: return true; break;

		//NV
		default: std::cout<<"CPU::Warning: ARM instruction uses reserved conditional code NV \n"; return true;
	}
}

/****** Updates the condition codes in the CPSR register after logical operations ******/
void ARM7::update_condition_logical(u32 result, u8 shift_out)
{
	//Zero flag
	if(result == 0) { reg.cpsr |= CPSR_Z_FLAG; }
	else { reg.cpsr &= ~CPSR_Z_FLAG; }

	//Carry flag
	if(shift_out == 1) { reg.cpsr |= CPSR_C_FLAG; }
	else if(shift_out == 0) { reg.cpsr &= ~CPSR_C_FLAG; }
}

/****** Updates the condition codes in the CPSR register after arithmetic operations ******/
void ARM7::update_condition_arithmetic(u32 input, u32 operand, u32 result, bool addition)
{
	//Negative flag
	if(result & 0x80000000) { reg.cpsr |= CPSR_N_FLAG; }
	else { reg.cpsr &= ~CPSR_N_FLAG; }

	//Zero flag
	if(result == 0) { reg.cpsr |= CPSR_Z_FLAG; }
	else { reg.cpsr &= ~CPSR_Z_FLAG; }

	//Carry flag
	if((input & 0x80000000) && !(result & 0x80000000) && (addition)) { reg.cpsr |= CPSR_C_FLAG; }

	//Overflow flag
	if(addition)
	{
		u64 real_result = input + operand;
		if((input <= 0x7FFFFFFF) && (operand <= 0x7FFFFFFF) && (real_result >= 0x80000000)) { reg.cpsr |= CPSR_V_FLAG; }
		else { reg.cpsr &= ~CPSR_V_FLAG; }
	}

	else { reg.cpsr &= ~CPSR_V_FLAG; }
}

/****** Performs 32-bit logical shift left - Returns Carry Out ******/
u8 ARM7::logical_shift_left(u32& input, u8 offset)
{
	u8 carry_out = 0;

	if(offset > 0)
	{
		//Test for carry
		//Perform LSL #(n-1), if Bit 31 is 1, we know it will carry out
		u32 carry_test = input << (offset - 1);
		carry_out = (carry_test & 0x80000000) ? 1 : 0;

		input <<= offset;
	}

	//LSL #0
	//No shift performed, carry flag not affected, set it to something not 0 or 1 to check!
	else { carry_out = 2; }

	return carry_out;
}

/****** Performs 32-bit logical shift right - Returns Carry Out ******/
u8 ARM7::logical_shift_right(u32& input, u8 offset)
{
	u8 carry_out = 0;

	if(offset > 0)
	{
		//Test for carry
		//Perform LSR #(n-1), if Bit 0 is 1, we know it will carry out
		u32 carry_test = input >> (offset - 1);
		carry_out = (carry_test & 0x1) ? 1 : 0;

		input >>= offset;
	}

	//LSR #0
	//Same as LSR #32, input becomes zero, carry flag is Bit 31 of input
	else
	{
		carry_out = (input & 0x80000000) ? 1 : 0;
		input = 0;
	}

	return carry_out;
}

/****** Performs 32-bit arithmetic shift right - Returns Carry Out ******/
u8 ARM7::arithmetic_shift_right(u32& input, u8 offset)
{
	u8 carry_out = 0;

	if(offset > 0)
	{
		//Convert input to a signed 32-bit integer, shifting it in C++ is equivalent to arithmetic shifting
		s32 s_input = input;

		//Test for carry
		//Perform ASR #(n-1), if Bit 0 is 1, we know it will carry out
		//Note, doesn't matter that we do logical shifting here, carry out is the same
		u32 carry_test = input >> (offset - 1);
		carry_out = (carry_test & 0x1) ? 1 : 0;

		s_input >>= offset;
		input = s_input;
	}

	//ASR #0
	//Same as ASR #32, input becomes 0xFFFFFFFF or 0x0 depending on Bit 31 of input
	//Carry flag set to 0 or 1 depending on Bit 31 of input
	else
	{
		if(input & 0x80000000) { input = 0xFFFFFFFF; carry_out = 1; }
		else { input = 0; carry_out = 0; }
	}

	return carry_out;
}

/****** Performs 32-bit rotate right ******/
u8 ARM7::rotate_right(u32& input, u8 offset)
{
	u8 carry_out = 0;

	if(offset > 0)
	{
		//Perform ROR shift on immediate
		for(int x = 0; x < offset; x++)
		{
			carry_out = input & 0x1;
			input >>= 1;

			if(carry_out) { input |= 0x80000000; }
		}
	}

	//ROR #0
	//Same as RRX #1, which is similar to ROR #1, except Bit 31 now becomes the old carry flag
	else
	{
		u8 old_carry = (reg.cpsr & CPSR_C_FLAG) ? 1 : 0;
		carry_out = input & 0x1;
		input >>= 1;
		
		if(old_carry) { input |= 0x80000000; }
	}

	return carry_out;
}

/****** Performs 32-bit rotate right - For ARM.5 Data Processing when Bit 25 is 1 ******/
void ARM7::rotate_right_special(u32& input, u8 offset)
{
	if(offset > 0)
	{
		//Perform ROR shift on immediate
		for(int x = 0; x < (offset * 2); x++)
		{
			u8 carry_out = input & 0x1;
			input >>= 1;

			if(carry_out) { input |= 0x80000000; }
		}
	}
}

/****** Runs audio and video controllers every clock cycle ******/
void ARM7::clock(u32 access_addr, bool first_access)
{
	//Determine cycles with Wait States + access timing
	u8 access_cycles = 1;

	//VRAM Access
	if((access_addr >= 0x5000000) && (access_addr <= 0x70003FF))
	{
		//Access time is +1 if outside of H-Blank of V-Blank, that is to say during scanline rendering
		//Otherwise, each 16-bit access is 1 cycle
		if(controllers.video.lcd_mode == 0) { access_cycles++; }

	}

	//Wait State 0
	if((access_addr >= 0x8000000) && (access_addr <= 0x9FFFFFF))
	{
		u16 wait_control = mem->read_u16(0x4000204);

		//Determine first access cycles (Non-Sequential)
		if(first_access)
		{
			wait_control >>= 2; 
			wait_control &= 0x3;

			switch(wait_control)
			{
				case 0x0: access_cycles += 4; break;
				case 0x1: access_cycles += 3; break;
				case 0x2: access_cycles += 2; break;
				case 0x3: access_cycles += 8; break;
			}
		}

		//Determine second access cycles (Sequential)
		else
		{
			wait_control >>= 4;
			wait_control &= 0x1;

			switch(wait_control)
			{
				case 0x0: access_cycles += 2; break;
				case 0x1: access_cycles += 1; break;
			}
		}
	}

	//Run controllers for each cycle		 
	for(int x = 0; x < access_cycles; x++)
	{
		controllers.video.step();
	}
}

/****** Runs audio and video controllers every clock cycle ******/
void ARM7::clock()
{
	controllers.video.step();
}

/****** Jumps to or exits an interrupt ******/
void ARM7::handle_interrupt()
{
	u16 ime_check = mem->read_u16(REG_IME);
	u16 if_check = mem->read_u16(REG_IF);
	u16 ie_check = mem->read_u16(REG_IE);

	//TODO - Implement a better way of exiting interrupts other than recognizing the SUB PC, #4 instruction
	//TODO - Correctly set CPSR flags (don't force THUMB mode)
	//TODO - Check the CPU's interrupt flag in addition to the GBA's IME register

	//Exit interrupt
	if((in_interrupt) && (debug_code == 0xE25EF004))
	{
		reg.cpsr = get_spsr();
		needs_flush = true;
		current_cpu_mode = SYS;
		mem->write_u16(REG_IME, 0x1);
		in_interrupt = false;
		arm_mode = (reg.cpsr & 0x20) ? THUMB : ARM;
	}

	//Jump into an interrupt, check if the master flag is enabled
	if(ime_check & 0x1)
	{
		//Match up bits in IE and IF
		for(int x = 0; x < 14; x++)
		{
			//When there is a match, jump to interrupt vector
			if((ie_check & (1 << x)) && (if_check & (1 << x)))
			{
				current_cpu_mode = IRQ;

				//If a Branch instruction has just executed, the PC technically should be 2 fetches ahead before the interrupt triggers
				//GBE+ does not do the fetches right away, so the PC is not updated until later
				//This screws with LR, so here we manually adjust LR by 2 instruction sizes.
				if((needs_flush == true) && (arm_mode == THUMB)) { set_reg(14, (reg.r15 + 4)); }
				else if((needs_flush == true) && (arm_mode == ARM)) { set_reg(14, (reg.r15 + 8)); }
				else { set_reg(14, reg.r15); }

				reg.r15 = 0x18;
				set_spsr(reg.cpsr);
				needs_flush = true;
				in_interrupt = true;
				arm_mode = ARM;
				reg.cpsr &= ~0x20;
				mem->write_u16(REG_IME, 0x0);
				return;
			}
		}
	}
}
	
				
				