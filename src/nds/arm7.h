// GB Enhanced+ Copyright Daniel Baxter 2016
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : arm7.h
// Date : July 30, 2016
// Description : ARM7TDMI emulator
//
// Emulates an ARM7TDMI CPU in software

#ifndef NDS7_CPU
#define NDS7_CPU

#include <string>
#include <iostream>
#include <vector>

#include "common.h"
#include "timer.h"
#include "mmu.h"
#include "lcd.h"

class NTR_ARM7
{
	public:

	//ARM instruction enumerations
	enum arm_instructions
	{
		UNDEFINED,
		PIPELINE_FILL,
		ARM_3,
		ARM_4,
		ARM_5,
		ARM_6,
		ARM_7,
		ARM_9,
		ARM_10,
		ARM_11,
		ARM_12,
		ARM_13,
		ARM_14,
		ARM_15,
		ARM_16,
		ARM_17,
		THUMB_1,
		THUMB_2,
		THUMB_3,
		THUMB_4,
		THUMB_5,
		THUMB_6,
		THUMB_7,
		THUMB_8,
		THUMB_9,
		THUMB_10,
		THUMB_11,
		THUMB_12,
		THUMB_13,
		THUMB_14,
		THUMB_15,
		THUMB_16,
		THUMB_17,
		THUMB_18,
		THUMB_19
	};

	//ARM CPU mode enumerations
	enum cpu_modes
	{
		USR,
		SYS,
		FIQ,
		SVC,
		ABT,
		IRQ,
		UND
	};

	//ARM CPU instruction mode enumerations
	enum instr_modes
	{
		ARM,
		THUMB
	};

	//BIOS read-state enumerations
	enum bios_state
	{
		BIOS_STARTUP,
		BIOS_IRQ_EXECUTE,
		BIOS_IRQ_FINISH,
		BIOS_SWI_FINISH
	};

	cpu_modes current_cpu_mode;
	instr_modes arm_mode;

	//Internal registers - 32bits each
	struct registers
	{
		//General purpose registers
		u32 r0;
		u32 r1;
		u32 r2;
		u32 r3;
		u32 r4;
		u32 r5;
		u32 r6;
		u32 r7;
		u32 r8;
		u32 r9;
		u32 r10;
		u32 r11;
		u32 r12;

		//Stack Pointer - SP
		u32 r13; 

		//Link Register - LR
		u32 r14;

		//Program Counter - PC
		u32 r15;

		//Current Program Status Register - CPSR
		u32 cpsr;

		//Banked FIQ registers
		u32 r8_fiq;
		u32 r9_fiq;
		u32 r10_fiq;
		u32 r11_fiq;
		u32 r12_fiq;
		u32 r13_fiq;
		u32 r14_fiq;
		u32 spsr_fiq;

		//Banked Supervisor registers
		u32 r13_svc;
		u32 r14_svc;
		u32 spsr_svc;

		//Banked Abort registers
		u32 r13_abt;
		u32 r14_abt;
		u32 spsr_abt;

		//Banked IRQ registers
		u32 r13_irq;
		u32 r14_irq;
		u32 spsr_irq;

		//Banked Undefined registers
		u32 r13_und;
		u32 r14_und;
		u32 spsr_und;

	} reg;

	bool running;
	bool needs_flush;
	bool in_interrupt;

	u8 idle_state;

	bool swi_vblank_wait;
	u32 swi_waitbyloop_count;

	u32 instruction_pipeline[3];
	arm_instructions instruction_operation[3];
	u8 pipeline_pointer;

	u8 debug_message;
	u32 debug_code;
	u32 debug_cycles;

	s16 sync_cycles;
	bool re_sync;

	NTR_MMU* mem;

	//Audio-Video and other controllers
	//TODO - NDS7 will handle audio
	struct io_controllers
	{
		std::vector<nds_timer> timer;
	} controllers;

	NTR_ARM7();
	~NTR_ARM7();

	//ARM pipelining functions
	void fetch();
	void decode();
	void execute();
	void update_pc();
	void flush_pipeline();

	void reset();

	//Get and set ARM registers
	u32 get_reg(u8 g_reg) const;
	void set_reg(u8 s_reg, u32 value);
	u32 get_spsr() const;
	void set_spsr(u32 value);

	//ARM instructions
	void branch_exchange(u32 current_arm_instruction);
	void branch_link(u32 current_arm_instruction);
	void data_processing(u32 current_arm_instruction);
	void psr_transfer(u32 current_arm_instruction);
	void multiply(u32 current_arm_instruction);
	void single_data_transfer(u32 current_arm_instruction);
	void halfword_signed_transfer(u32 current_arm_instruction);
	void block_data_transfer(u32 current_arm_instruction);
	void single_data_swap(u32 current_arm_instruction);
	void software_interrupt_breakpoint(u32 current_arm_instruction);

	//THUMB instructions
	void move_shifted_register(u16 current_thumb_instruction);
	void add_sub_immediate(u16 current_thumb_instruction);
	void mcas_immediate(u16 current_thumb_instruction);
	void alu_ops(u16 current_thumb_instruction);
	void hireg_bx(u16 current_thumb_instruction);
	void load_pc_relative(u16 current_thumb_instruction);
	void load_store_reg_offset(u16 current_thumb_instruction);
	void load_store_sign_ex(u16 current_thumb_instruction);
	void load_store_imm_offset(u16 current_thumb_instruction);
	void load_store_halfword(u16 current_thumb_instruction);
	void load_store_sp_relative(u16 current_thumb_instruction);
	void get_relative_address(u16 current_thumb_instruction);
	void add_offset_sp(u16 current_thumb_instruction);
	void push_pop(u16 current_thumb_instruction);
	void multiple_load_store(u16 current_thumb_instruction);
	void conditional_branch(u16 current_thumb_instruction);
	void unconditional_branch(u16 current_thumb_instruction);
	void long_branch_link(u16 current_thumb_instruction);

	//System functions
	void clock(u32 access_address, bool first_access);
	void clock();
	void clock_timers();
	void clock_dma();
	void handle_interrupt();

	//DMA functions
	void dma0();
	void dma1();
	void dma2();
	void dma3();

	//Misc CPU helpers
	void update_condition_logical(u32 result, u8 shift_out);
	void update_condition_arithmetic(u32 input, u32 operand, u32 result, bool addition);
	bool check_condition(u32 current_arm_instruction) const;
	u8 logical_shift_left(u32& input, u8 offset);
	u8 logical_shift_right(u32& input, u8 offset);
	u8 arithmetic_shift_right(u32& input, u8 offset);
	u8 rotate_right(u32& input, u8 offset);
	void rotate_right_special(u32& input, u8 offset);
	void mem_check_32(u32 addr, u32& value, bool load_store);
	void mem_check_16(u32 addr, u32& value, bool load_store);
	void mem_check_8(u32 addr, u32& value, bool load_store);

	//HLE Software Interrupts (BIOS Calls)
	void process_swi(u32 comment);
	void swi_softreset();
	void swi_registerramreset();
	void swi_div();
	void swi_divarm();
	void swi_sqrt();
	void swi_arctan();
	void swi_arctan2();
	void swi_cpufastset();
	void swi_cpuset();
	void swi_halt();
	void swi_sleep();
	void swi_intrwait();
	void swi_vblankintrwait();
	void swi_lz77uncompvram();
	void swi_rluncompvram();
	void swi_huffuncomp();
	void swi_getbioschecksum();
	void swi_bgaffineset();
	void swi_objaffineset();
	void swi_bitunpack();
	void swi_midikey2freq();

	void swi_getcrc16();
	void swi_waitbyloop();
};
		
#endif // NDS7_CPU