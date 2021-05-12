// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : s1c88.h
// Date : December 03, 2020
// Description : S1C88 CPU emulator
//
// Emulates an S1C88 CPU in software

#ifndef PM_CPU
#define PM_CPU

#include <string>
#include <iostream>

#include "common.h"
#include "mmu.h"
#include "lcd.h"
#include "timer.h"
#include "apu.h"

class S1C88
{
	public:
	
	//Internal Registers
	struct registers
	{
		//BA
		union
		{
			struct 
			{
				u8 a;
				u8 b;
			};
			u16 ba;
		};

		//HL
		union
		{
			struct 
			{
				u8 l;
				u8 h;
			};
			u16 hl;
		};

		u16 sp;

		u16 pc;
		u32 pc_ex;

		u16 iy;
		u32 iy_ex;

		u16 ix;
		u32 ix_ex;

		u16 br;
		u32 br_ex;

		u32 hl_ex;

		u8 sc;
		u8 cc;

		u8 nb;
		u8 cb;

		u8 xp;
		u8 yp;
		u8 ep;

	} reg;

	u32 opcode;
	u16 log_addr;
	u8 system_cycles;
	u32 debug_cycles;

	bool halt;

	u32 debug_opcode;

	//Memory management unit
	MIN_MMU* mem;

	//CPU flags
	bool running;
	bool interrupt_in_progress;
	bool skip_irq;

	//Audio-Video and other controllers
	struct io_controllers
	{
		MIN_LCD video;
		MIN_APU audio;
		std::vector<min_timer> timer;
	} controllers;

	//Core Functions
	S1C88();
	~S1C88();
	void reset();

	void execute();
	void handle_interrupt();
	void clock_system();

	u8 add_u8(u8 reg_one, u8 reg_two);
	u16 add_u16(u16 reg_one, u16 reg_two);

	u8 adc_u8(u8 reg_one, u8 reg_two);
	u16 adc_u16(u16 reg_one, u16 reg_two);

	u8 sub_u8(u8 reg_one, u8 reg_two);
	u16 sub_u16(u16 reg_one, u16 reg_two);

	u8 sbc_u8(u8 reg_one, u8 reg_two);
	u16 sbc_u16(u16 reg_one, u16 reg_two);

	void cp_u8(u8 reg_one, u8 reg_two);
	void cp_u16(u16 reg_one, u16 reg_two);

	u8 inc_u8(u8 reg_one);
	u16 inc_u16(u16 reg_one);

	u8 dec_u8(u8 reg_one);
	u16 dec_u16(u16 reg_one);

	u8 neg_u8(u8 reg_one);
	u16 mlt_u8(u8 reg_one, u8 reg_two);
	u16 div_u16(u16 reg_one, u8 reg_two);
	void bit_u8(u8 reg_one, u8 reg_two);

	u8 and_u8(u8 reg_one, u8 reg_two);
	u8 or_u8(u8 reg_one, u8 reg_two);
	u8 xor_u8(u8 reg_one, u8 reg_two);
	u8 cpl_u8(u8 reg_one);

	u8 sll_u8(u8 reg_one);
	u8 sla_u8(u8 reg_one);
	u8 srl_u8(u8 reg_one);
	u8 sra_u8(u8 reg_one);

	u8 rlc_u8(u8 reg_one);
	u8 rl_u8(u8 reg_one);
	u8 rrc_u8(u8 reg_one);
	u8 rr_u8(u8 reg_one);

	s8 comp8(u8 reg_one);
	s8 comp8u(u8 reg_one);
	s16 comp16(u16 reg_one);

	void update_regs();
};
		
#endif // PM_CPU 
		
