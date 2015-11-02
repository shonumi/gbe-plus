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
	reg.r13 = reg.r13_fiq = reg.r13_abt = reg.r13_und = 0x03007F00;
	reg.r13_svc = 0x03007FE0;
	reg.r13_irq = 0x03007FA0;
	reg.r15 = 0x8000000;
	reg.cpsr = 0x5F;

	reg.r8_fiq = reg.r9_fiq = reg.r10_fiq = reg.r11_fiq = reg.r12_fiq = reg.r14_fiq = reg.spsr_fiq = 0;
	reg.r14_svc = reg.spsr_svc = 0;
	reg.r14_abt = reg.spsr_abt = 0;
	reg.r14_irq = reg.spsr_irq = 0;
	reg.r14_und = reg.spsr_und = 0;

	running = false;
	in_interrupt = false;
	sleep = false;

	swi_vblank_wait = false;

	arm_mode = ARM;
	current_cpu_mode = SYS;
	bios_read_state = BIOS_STARTUP;

	controllers.timer.clear();
	controllers.timer.resize(4);

	for(int x = 0; x < 4; x++)
	{
		controllers.timer[x].counter = 0;
		controllers.timer[x].reload_value = 0;
		controllers.timer[x].prescalar = 0;
		controllers.timer[x].cycles = 0;
		controllers.timer[x].count_up = false;
		controllers.timer[x].enable = false;
	}

	debug_message = 0xFF;
	debug_code = 0;
	debug_cycles = 0;

	flush_pipeline();
	mem = NULL;

	std::cout<<"CPU::Initialized\n";
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
				case USR:
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
				case USR:
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
				case USR:
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
				case USR:
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
		case USR:
		case SYS: return reg.cpsr; break;
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
		case USR:
		case SYS: break;
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
		instruction_pipeline[pipeline_pointer] = mem->read_u16_fast(reg.r15);

		//Set the operation to perform as UNDEFINED until decoded
		instruction_operation[pipeline_pointer] = UNDEFINED;
	}

	//Fetch ARM instructions
	else if(arm_mode == ARM)
	{
		//Read 32-bit ARM instruction
		instruction_pipeline[pipeline_pointer] = mem->read_u32_fast(reg.r15);

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
		
		if(((current_instruction >> 13) == 0) && (((current_instruction >> 11) & 0x7) != 0x3))
		{
			//THUMB_1
			instruction_operation[pipeline_id] = THUMB_1;
		}

		else if(((current_instruction >> 11) & 0x1F) == 0x3)
		{
			//THUMB_2
			instruction_operation[pipeline_id] = THUMB_2;
		}

		else if((current_instruction >> 13) == 0x1)
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

		else if((current_instruction >> 11) == 0x9)
		{
			//THUMB_6
			instruction_operation[pipeline_id] = THUMB_6;
		}

		else if((current_instruction >> 12) == 0x5)
		{
			if(current_instruction & 0x200)
			{
				//THUMB_8
				instruction_operation[pipeline_id] = THUMB_8;
			}

			else
			{
				//THUMB_7
				instruction_operation[pipeline_id] = THUMB_7;
			}
		}

		else if(((current_instruction >> 13) & 0x7) == 0x3)
		{
			//THUMB_9
			instruction_operation[pipeline_id] = THUMB_9;
		}

		else if((current_instruction >> 12) == 0x8)
		{
			//THUMB_10
			instruction_operation[pipeline_id] = THUMB_10;
		}

		else if((current_instruction >> 12) == 0x9)
		{
			//THUMB_11
			instruction_operation[pipeline_id] = THUMB_11;
		}

		else if((current_instruction >> 12) == 0xA)
		{
			//THUMB_12
			instruction_operation[pipeline_id] = THUMB_12;
		}

		else if((current_instruction >> 8) == 0xB0)
		{
			//THUMB_13
			instruction_operation[pipeline_id] = THUMB_13;
		}

		else if((current_instruction >> 12) == 0xB)
		{
			//THUMB_14
			instruction_operation[pipeline_id] = THUMB_14;
		}

		else if((current_instruction >> 12) == 0xC)
		{
			//THUMB_15
			instruction_operation[pipeline_id] = THUMB_15;
		}

		else if((current_instruction >> 12) == 13)
		{
			//THUMB_16
			instruction_operation[pipeline_id] = THUMB_16;
		}

		else if((current_instruction >> 11) == 0x1C)
		{
			//THUMB_18
			instruction_operation[pipeline_id] = THUMB_18;
		}

		else if((current_instruction >> 11) >= 0x1E)
		{
			//THUMB_19
			instruction_operation[pipeline_id] = THUMB_19;
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

		//TODO - Move ARM_6 decoding to final stage of ARM_5 decoding
		//TODO - Move ARM_12 deconding to final stage of ARM_10 decoding		

		else if((current_instruction & 0xD900000) == 0x1000000) 
		{

			if((current_instruction & 0x80) && (current_instruction & 0x10) && ((current_instruction & 0x2000000) == 0))
			{
				if(((current_instruction >> 5) & 0x3) == 0) 
				{ 
					instruction_operation[pipeline_id] = ARM_12;
				}

				else 
				{
					instruction_operation[pipeline_id] = ARM_10;
				}
			}

			else 
			{
				//ARM_6
				instruction_operation[pipeline_id] = ARM_6;
			}
		}

		else if(((current_instruction >> 26) & 0x3) == 0x0)
		{
			if((current_instruction & 0x80) && ((current_instruction & 0x10) == 0))
			{
				//ARM.5
				if(current_instruction & 0x2000000)
				{
					instruction_operation[pipeline_id] = ARM_5;
				}

				//ARM.5
				else if((current_instruction & 0x100000) && (((current_instruction >> 23) & 0x3) == 0x2))
				{
					instruction_operation[pipeline_id] = ARM_5;
				}

				//ARM.5
				else if(((current_instruction >> 23) & 0x3) != 0x2)
				{
					instruction_operation[pipeline_id] = ARM_5;
				}

				//ARM.7
				else
				{
					instruction_operation[pipeline_id] = ARM_7;
				}
			}

			else if((current_instruction & 0x80) && (current_instruction & 0x10))
			{
				if(((current_instruction >> 4) & 0xF) == 0x9)
				{
					//ARM.5
					if(current_instruction & 0x2000000)
					{
						instruction_operation[pipeline_id] = ARM_5;
					}

					//ARM.12
					else if(((current_instruction >> 23) & 0x3) == 0x2)
					{
						instruction_operation[pipeline_id] = ARM_12;
					}

					//ARM.7
					else
					{
						instruction_operation[pipeline_id] = ARM_7;
					}
				}

				//ARM.5
				else if(current_instruction & 0x2000000)
				{
					instruction_operation[pipeline_id] = ARM_5;
				}

				//ARM.10
				else
				{
					instruction_operation[pipeline_id] = ARM_10;
				}
			}

			//ARM.5
			else
			{
				instruction_operation[pipeline_id] = ARM_5;
			}
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

		else if(((current_instruction >> 24) & 0xF) == 0xF)
		{
			//ARM_13
			instruction_operation[pipeline_id] = ARM_13;
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

			case THUMB_6:
				load_pc_relative(instruction_pipeline[pipeline_id]);
				debug_message = 0x5; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_7:
				load_store_reg_offset(instruction_pipeline[pipeline_id]);
				debug_message = 0x6; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_8:
				load_store_sign_ex(instruction_pipeline[pipeline_id]);
				debug_message = 0x7; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_9:
	 			load_store_imm_offset(instruction_pipeline[pipeline_id]);
				debug_message = 0x8; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_10:
				load_store_halfword(instruction_pipeline[pipeline_id]);
				debug_message = 0x9; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_11:
				load_store_sp_relative(instruction_pipeline[pipeline_id]);
				debug_message = 0xA; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_12:
				get_relative_address(instruction_pipeline[pipeline_id]);
				debug_message = 0xB; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_13:
				add_offset_sp(instruction_pipeline[pipeline_id]);
				debug_message = 0xC; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_14:
				push_pop(instruction_pipeline[pipeline_id]);
				debug_message = 0xD; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_15:
				multiple_load_store(instruction_pipeline[pipeline_id]);
				debug_message = 0xE; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_16:
				conditional_branch(instruction_pipeline[pipeline_id]);
				debug_message = 0xF; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_18:
				unconditional_branch(instruction_pipeline[pipeline_id]);
				debug_message = 0x11; debug_code = instruction_pipeline[pipeline_id];
				break;

			case THUMB_19:
				long_branch_link(instruction_pipeline[pipeline_id]);
				debug_message = 0x12; debug_code = instruction_pipeline[pipeline_id];
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

				case ARM_7:
					multiply(instruction_pipeline[pipeline_id]);
					debug_message = 0x18; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_9:
					single_data_transfer(instruction_pipeline[pipeline_id]);
					debug_message = 0x19; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_10:
					halfword_signed_transfer(instruction_pipeline[pipeline_id]);
					debug_message = 0x1A; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_11:
					block_data_transfer(instruction_pipeline[pipeline_id]);
					debug_message = 0x1B; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_12:
					single_data_swap(instruction_pipeline[pipeline_id]);
					debug_message = 0x1C; debug_code = instruction_pipeline[pipeline_id];
					break;

				case ARM_13:
					software_interrupt_breakpoint(instruction_pipeline[pipeline_id]);
					debug_message = 0x1D; debug_code = instruction_pipeline[pipeline_id];
					break;

				default:
					debug_message = 0x1E; debug_code = instruction_pipeline[pipeline_id];
					running = false;
					break;
			}
		}

		//Skip ARM instruction
		else 
		{ 
			debug_message = 0x1F; 
			debug_code = instruction_pipeline[pipeline_id];

			//Clock CPU and controllers - 1S
			clock(reg.r15, false); 
		}
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
	reg.r15 += (arm_mode == ARM) ? 4 : 2;
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
			if((reg.cpsr & CPSR_Z_FLAG) || ((reg.cpsr & CPSR_C_FLAG) == 0)) { return true; }
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

				if((z == 1) || (n != v)) { return true; }
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
	//Negative flag
	if(result & 0x80000000) { reg.cpsr |= CPSR_N_FLAG; }
	else { reg.cpsr &= ~CPSR_N_FLAG; }

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

	//Carry flag - Addition
	if((operand > (0xFFFFFFFF - input)) && (addition)) { reg.cpsr |= CPSR_C_FLAG; }

	//Carry flag - Subtraction
	else if((operand <= input) && (!addition)) { reg.cpsr |= CPSR_C_FLAG; }

	else { reg.cpsr &= ~CPSR_C_FLAG; }

	//Overflow flag
	u8 input_msb = (input & 0x80000000) ? 1 : 0;
	u8 operand_msb = (operand & 0x80000000) ? 1 : 0;
	u8 result_msb = (result & 0x80000000) ? 1 : 0;

	if(addition)
	{
		if(input_msb != operand_msb) { reg.cpsr &= ~CPSR_V_FLAG; }
		
		else
		{
			if((result_msb == input_msb) && (result_msb == operand_msb)) { reg.cpsr &= ~CPSR_V_FLAG; }
			else { reg.cpsr |= CPSR_V_FLAG; }
		}
	}

	else
	{
		if(input_msb == operand_msb) { reg.cpsr &= ~CPSR_V_FLAG; }
		
		else
		{
			if(result_msb == operand_msb) { reg.cpsr |= CPSR_V_FLAG; }
			else { reg.cpsr &= ~CPSR_V_FLAG; }
		}
	}
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

		if(offset >= 32) { input = 0; }
		else { input <<= offset; }
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

		if(offset >= 32) { input = 0; }
		else { input >>= offset; }
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
		u8 high_bit = (input & 0x80000000) ? 1 : 0;		

		//Basically LSR, but bits become Bit 31
		for(int x = 0; x < offset; x++)
		{
			carry_out = (input & 0x1) ? 1 : 0;
			input >>= 1;
			if(high_bit == 1) { input |= 0x80000000; }
		}	
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

/****** Checks address before 32-bit reading/writing for special case scenarios ******/
void ARM7::mem_check_32(u32 addr, u32& value, bool load_store)
{
	//Assume normal operation until a special case occurs
	bool normal_operation = true;

	//Check for special case scenarios for read ops
	if(load_store)
	{
		//Misaligned LDR or SWP
		if((addr & 0x1) || (addr & 0x2)) 
		{
			normal_operation = false;

			//Force alignment by word, then rotate right the read
			u8 offset = (addr & 0x3) * 8;
			value = mem->read_u32(addr & ~0x3);
			rotate_right(value, offset);
		}

		//Out of bounds unused memory
		if((addr & ~0x3) >= 0x10000000)
		{
			normal_operation = false;
	
			//Read the opcode instruction at PC
			if(arm_mode == ARM) { value = mem->read_u32(reg.r15); }
			else { value = (mem->read_u16(reg.r15) << 16) | mem->read_u16(reg.r15); }
		}

		//Return specific values when trying to read BIOS when PC is not within the BIOS
		if(((addr & ~0x3) <= 0x3FFF) && (reg.r15 > 0x3FFF))
		{
			normal_operation = false;

			switch(bios_read_state)
			{
				case BIOS_STARTUP: value = 0xE129F000; break;
				case BIOS_IRQ_EXECUTE : value = 0xE25EF004; break;
				case BIOS_IRQ_FINISH : value = 0xE55EC002; break;
				case BIOS_SWI_FINISH : value = 0xE3A02004; break;
			}
		}

		//Special reads to I/O with some bits being unreadable
		switch(addr)
		{
			//Return zero for the lower halfword of the following addresses (only top halfword is readable)

			//DMAxCNT_H
			case 0x40000BA: value = (mem->read_u16(0x40000BA) << 16); normal_operation = false; break;
			case 0x40000C6: value = (mem->read_u16(0x40000C6) << 16); normal_operation = false; break;
			case 0x40000D2: value = (mem->read_u16(0x40000D2) << 16); normal_operation = false; break;
			case 0x40000DE: value = (mem->read_u16(0x40000DE) << 16); normal_operation = false; break;

			//Return 0 for the following addresses (only bottom halfword is readable)

			//DMAxCNT_L
			case 0x40000B8:
 			case 0x40000C4:
			case 0x40000D0:
			case 0x40000DC:
				value = 0;
				normal_operation = false;
				break;
		}

		//Normal operation
		if(normal_operation) { value = mem->read_u32(addr); }
	}

	//Check for special case scenarios for write ops
	else
	{
		//Misaligned STR
		if((addr & 0x1) || (addr & 0x2)) 
		{
			normal_operation = false;

			//Force alignment by word, but that's all, no rotation
			mem->write_u32((addr & ~0x3), value);
		}

		//Normal operation
		else { mem->write_u32(addr, value); }
	}
}

/****** Checks address before 16-bit reading/writing for special case scenarios ******/
void ARM7::mem_check_16(u32 addr, u32& value, bool load_store)
{
	//Assume normal operation until a special case occurs
	bool normal_operation = true;

	//Check for special case scenarios for read ops
	if(load_store)
	{
		//Misaligned LDR
		if(addr & 0x1) 
		{
			normal_operation = false;

			//Force alignment by halfword
			value = mem->read_u16(addr & ~0x1);
		}

		//Out of bounds unused memory
		if((addr & ~0x1) >= 0x10000000)
		{
			normal_operation = false;
	
			//Read the opcode instruction at PC
			value = mem->read_u16(reg.r15);
		}

		//Return 0 for certain readable I/O
		switch(addr)
		{
			case 0x40000BA:
			case 0x40000C6:
			case 0x40000D2:
			case 0x40000DE: value = 0; normal_operation = false; break;
		}

		//Return specific values when trying to read BIOS when PC is not within the BIOS
		if(((addr & ~0x1) <= 0x3FFF) && (reg.r15 > 0x3FFF))
		{
			normal_operation = false;

			switch(bios_read_state)
			{
				case BIOS_STARTUP: value = 0xF000; break;
				case BIOS_IRQ_EXECUTE : value = 0xF004; break;
				case BIOS_IRQ_FINISH : value = 0xC002; break;
				case BIOS_SWI_FINISH : value = 0x2004; break;
			}
		}

		//Normal operation
		if(normal_operation) { value = mem->read_u16(addr); }
	}

	//Check for special case scenarios for write ops
	else
	{
		//Misaligned STR
		if(addr & 0x1) 
		{
			normal_operation = false;

			//Force alignment by word, but that's all, no rotation
			mem->write_u16((addr & ~0x1), value);
		}

		//Normal operation
		else { mem->write_u16(addr, value); }
	}
}

/****** Checks address before 8-bit reading/writing for special case scenarios ******/
void ARM7::mem_check_8(u32 addr, u32& value, bool load_store)
{
	//Assume normal operation until a special case occurs
	bool normal_operation = true;

	//Check for special case scenarios for read ops
	if(load_store)
	{
		//Undocumented by GBATEK!
		//Unused 8-bit reads only return the 1st byte of the opcode at the PC (makes sense, but no documentation on this whatsoever)
		if(addr >= 0x10000000)
		{
			normal_operation = false;
			value = mem->read_u8(reg.r15);
		}

		//Normal operation
		if(normal_operation) { value = mem->read_u8(addr); }
	}

	//Check for special case scenarios for write ops
	else
	{
		//Check if the address is anywhere near VRAM first
		if((addr >= 0x5000000) && (addr < 0x8000000))
		{
			//Ignore 8-bit writes to OBJ VRAM (BG Modes 0-2)
			if((addr >= 0x6010000) && (addr <= 0x6017FFF) && ((mem->memory_map[DISPCNT] & 0x3) <= 2)) { return; }

			//Ignore 8-bit writes to OBJ VRAM (BG Modes 3-5)
			else if((addr >= 0x6014000) && (addr <= 0x6017FFF) && ((mem->memory_map[DISPCNT] & 0x3) > 2)) { return; }

			//Ignore 8-bit writes to OAM
			else if((addr >= 0x7000000) && (addr <= 0x70003FF)) { return; }

			//Special write to BG data (BG Modes 0-2)
			else if((addr >= 0x6000000) && (addr <= 0x600FFFF) && ((mem->memory_map[DISPCNT] & 0x3) <= 2)) 
			{
				mem->write_u16(addr, ((value << 8) | value));
			}

			//Special write to BG data (BG Modes 3-5)
			else if((addr >= 0x6000000) && (addr <= 0x6013FFF) && ((mem->memory_map[DISPCNT] & 0x3) > 2)) 
			{
				mem->write_u16(addr, ((value << 8) | value));
			}

			//Special write to Palette data
			else if((addr >= 0x5000000) && (addr <= 0x50003FF)) 
			{
				mem->write_u16(addr, ((value << 8) | value));
			}
		}
		
		//Normal operation
		else { mem->write_u8(addr, value); }
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
	else if((access_addr >= 0x8000000) && (access_addr <= 0x9FFFFFF))
	{
		//Determine first access cycles (Non-Sequential)
		if(first_access) { access_cycles += mem->n_clock;  }

		//Determine second access cycles (Sequential)
		else { access_cycles += mem->s_clock; }
	}

	//Run controllers for each cycle		 
	for(int x = 0; x < access_cycles; x++)
	{
		controllers.video.step();
		clock_timers();
		clock_dma();
		debug_cycles++;
	}
}

/****** Runs audio and video controllers every clock cycle ******/
void ARM7::clock()
{
	controllers.video.step();
	clock_timers();
	clock_dma();
}

/****** Runs DMA controllers every clock cycle ******/
void ARM7::clock_dma()
{
	//DMA0
	if(mem->dma[0].enable) { dma0(); }

	//DMA1
	if(mem->dma[1].enable) { dma1(); }

	//DMA2
	if(mem->dma[2].enable) { dma2(); }

	//DMA3
	if(mem->dma[3].enable) { dma3(); }
}

/****** Runs Timer controllers every clock cycle ******/
void ARM7::clock_timers()
{
	for(int x = 0; x < 4; x++)
	{
		//See if this timer is enabled first
		if(controllers.timer[x].enable)
		{
			controllers.timer[x].cycles++;

			//If the amount of cycles matches the prescalar, increment counter
			if(controllers.timer[x].cycles == controllers.timer[x].prescalar)
			{
				controllers.timer[x].cycles = 0;
				if(!controllers.timer[x].count_up)
				{
					controllers.timer[x].counter++;

					//If counter overflows, reload value, trigger interrupt if necessary
					if(controllers.timer[x].counter == 0) 
					{
						controllers.timer[x].counter = controllers.timer[x].reload_value;

						//Increment next timer if in count-up mode
						if((x < 4) && (controllers.timer[x+1].count_up)) { controllers.timer[x+1].counter++; }

						//Interrupt
						if(controllers.timer[x].interrupt)
						{
							mem->memory_map[REG_IF] |= (8 << x);
						}

						//Timer 0 Audio FIFO A, DMA 1-2
						if((x == 0) && (controllers.audio.apu_stat.dma[0].timer == 0) && (mem->dma[1].destination_address == FIFO_A) && (mem->dma[1].started)) 
						{
							controllers.audio.apu_stat.dma[0].buffer[controllers.audio.apu_stat.dma[0].counter++] = mem->memory_map[mem->dma[1].start_address++];
							controllers.audio.apu_stat.dma[0].length++;

							//Trigger DMA IRQ after 16th bit is transferred
							if((mem->memory_map[REG_IE+1] & 0x2) && ((controllers.audio.apu_stat.dma[0].counter % 16) == 0)) { mem->memory_map[REG_IF+1] |= 0x2; }
						}

						if((x == 0) && (controllers.audio.apu_stat.dma[1].timer == 0) && (mem->dma[2].destination_address == FIFO_B) && (mem->dma[2].started)) 
						{ 
							controllers.audio.apu_stat.dma[1].buffer[controllers.audio.apu_stat.dma[1].counter++] = mem->memory_map[mem->dma[2].start_address++];
							controllers.audio.apu_stat.dma[1].length++;

							//Trigger DMA IRQ after 16th bit is transferred
							if((mem->memory_map[REG_IE+1] & 0x4) && ((controllers.audio.apu_stat.dma[1].counter % 16) == 0)) { mem->memory_map[REG_IF+1] |= 0x4; }
						}
					
						/*
						else if((x == 0) && (controllers.audio.apu_stat.dma[0].timer == 0) && (mem->dma[2].destination_address == FIFO_A)) { }

						//Timer 0 Audio FIFO B, DMA 1-2
						else if((x == 0) && (controllers.audio.apu_stat.dma[1].timer == 0) && (mem->dma[1].destination_address == FIFO_B)) { }
						else if((x == 0) && (controllers.audio.apu_stat.dma[1].timer == 0) && (mem->dma[2].destination_address == FIFO_B)) { }

						//Timer 1 Audio FIFO A, DMA 1-2
						else if((x == 1) && (controllers.audio.apu_stat.dma[0].timer == 1) && (mem->dma[1].destination_address == FIFO_A)) { }
						else if((x == 1) && (controllers.audio.apu_stat.dma[0].timer == 1) && (mem->dma[2].destination_address == FIFO_A)) { }

						//Timer 1 Audio FIFO B, DMA 1-2
						else if((x == 1) && (controllers.audio.apu_stat.dma[1].timer == 1) && (mem->dma[1].destination_address == FIFO_B)) { }
						else if((x == 1) && (controllers.audio.apu_stat.dma[1].timer == 1) && (mem->dma[2].destination_address == FIFO_B)) { }
						*/
					}
				}
			}
		}
	}
}

/****** Jumps to or exits an interrupt ******/
void ARM7::handle_interrupt()
{
	//TODO - Implement a better way of exiting interrupts other than recognizing the SUB PC, #4 instruction

	//Exit interrupt
	if((in_interrupt) && (debug_code == 0xE25EF004))
	{
		reg.cpsr = get_spsr();
		reg.cpsr &= ~CPSR_IRQ;
		needs_flush = true;
		current_cpu_mode = SYS;
		in_interrupt = false;
		arm_mode = (reg.cpsr & 0x20) ? THUMB : ARM;
		bios_read_state = BIOS_IRQ_FINISH;
		debug_code = 0xFEEDBACC;
	}

	//Jump into an interrupt, check if the master flag is enabled
	if((mem->memory_map[REG_IME] & 0x1) && ((reg.cpsr & CPSR_IRQ) == 0) && (!in_interrupt))
	{
		u16 if_check = mem->read_u16_fast(REG_IF);
		u16 ie_check = mem->read_u16_fast(REG_IE);

		//Match up bits in IE and IF
		for(int x = 0; x < 14; x++)
		{
			//When there is a match, jump to interrupt vector
			if((ie_check & (1 << x)) && (if_check & (1 << x)))
			{
				//If an HLE SWI is waiting for the next VBlank interrupt, advance the PC to complete the SWI call
				if((swi_vblank_wait) && (x == 0)) 
				{  
					reg.r15 += (arm_mode == ARM) ? 4 : 2;
					swi_vblank_wait = false; 
				}

				current_cpu_mode = IRQ;

				//If a Branch instruction has just executed, the PC is changed before jumping into the interrupt
				//When returning from an interrupt, the GBA calls SUBS R15, R14, 0x4 to return where it left off
				//As a result, LR needs to hold the value of the PC + 4 (really, PC+nn+4, where nn is 2 instruction sizes)
				if(needs_flush) { set_reg(14, (reg.r15 + 4)); }

				//When there is no Branch, THUMB's LR has to be set to PC+nn+2 (nn is 4, two instruction sizes)
				//In GBE+, the branch instruction executes, but interrupts happens before PC updates at the new location (so we move the PC along here instead).
				//SUBS R15, R14, 0x4 would jump back to the current instruction, thus executing the THUMB opcode twice!
				else if((!needs_flush) && (arm_mode == THUMB)) { set_reg(14, (reg.r15 + 2)); }
				else if((!needs_flush) && (arm_mode == ARM)) { set_reg(14, reg.r15); }

				reg.r15 = 0x18;
				set_spsr(reg.cpsr);
				needs_flush = true;
				in_interrupt = true;
				arm_mode = ARM;
				reg.cpsr &= ~0x20;
				reg.cpsr |= CPSR_IRQ;
				bios_read_state = BIOS_IRQ_EXECUTE;
				return;
			}
		}
	}
}
	
				
				