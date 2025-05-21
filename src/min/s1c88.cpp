// GB Enhanced Copyright Daniel Baxter 2013
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : s1c88.cpp
// Date : December 03, 2020
// Description : S1C88 CPU emulator
//
// Emulates a S1C88 in software

#include "s1c88.h"

/****** S1C88 Constructor ******/
S1C88::S1C88() 
{
	reset();
}

/****** S1C88 Deconstructor ******/
S1C88::~S1C88() 
{ 
	std::cout<<"CPU::Shutdown\n";
}

/****** S1C88 Reset ******/
void S1C88::reset() 
{
	reg.a = 0;
	reg.b = 0;
	reg.h = 0;
	reg.l = 0;
	reg.pc = 0;
	reg.sp = 0;
	reg.ix = 0;
	reg.iy = 0;
	reg.br = 0;
	reg.sc = 0;
	reg.cc = 0;
	reg.ep = 0;
	reg.xp = 0;
	reg.yp = 0;

	reg.hl_ex = 0;
	reg.br_ex = 0;
	reg.ix_ex = 0;
	reg.iy_ex = 0;
	reg.pc_ex = 0;

	reg.cb = 0;
	reg.nb = 0;

	opcode = 0;
	log_addr = 0;
	system_cycles = 0;
	debug_cycles = 0;
	running = false;

	skip_irq = false;
	halt = false;

	debug_opcode = 0;

	controllers.timer.clear();
	controllers.timer.resize(4);

	for(int x = 0; x < 4; x++)
	{
		controllers.timer[x].cnt = 0;
		controllers.timer[x].counter = 0;
		controllers.timer[x].reload_value = 0;
		controllers.timer[x].prescalar_lo = 0;
		controllers.timer[x].prescalar_hi = 0;
		controllers.timer[x].clock_lo = 0;
		controllers.timer[x].clock_hi = 0;

		controllers.timer[x].pivot = 0;
		controllers.timer[x].pivot_status = 0;

		controllers.timer[x].osc_lo = 0;
		controllers.timer[x].osc_hi = 0;

		controllers.timer[x].interrupt = false;
		controllers.timer[x].full_mode = false;

		controllers.timer[x].enable_lo = false;
		controllers.timer[x].enable_hi = false;

		controllers.timer[x].enable_osc = false;

		controllers.timer[x].enable_scalar_hi = false;
		controllers.timer[x].enable_scalar_lo = false;
	}

	//256Hz timer setup
	controllers.timer[3].prescalar_lo = 15625;

	mem = NULL;

	std::cout<<"CPU::Initialized\n";
}

/****** Adds 2 8-bit registers ******/
u8 S1C88::add_u8(u8 reg_one, u8 reg_two)
{
	u8 result;

	u16 c_flag;
	s16 o_flag;

	//Binary mode
	if((reg.sc & DECIMAL_FLAG) == 0)
	{
		//Unpacked operation
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one &= 0xF;
			reg_two &= 0xF;

			result = reg_one + reg_two;

			c_flag = reg_one + reg_two;
			o_flag = comp8u(reg_one) + comp8u(reg_two);

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if(c_flag >= 0x10) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x8) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}

		//Packed operation
		else
		{
			result = reg_one + reg_two;

			c_flag = reg_one + reg_two;
			o_flag = comp8(reg_one) + comp8(reg_two);

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if(c_flag >= 0x100) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}
	}

	//Decimal Mode
	else
	{
		//Convert registers into BCD format - Unpacked
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one = util::get_bcd_int(reg_one & 0xF);
			reg_two = util::get_bcd_int(reg_two & 0xF);

			result = (reg_one + reg_two) % 10;
			c_flag = reg_one + reg_two;
		}

		//Convert registers into BCD format - Packed
		else
		{
			reg_one = util::get_bcd_int(reg_one);
			reg_two = util::get_bcd_int(reg_two);

			result = u16(reg_one + reg_two) % 100;
			c_flag = reg_one + reg_two;
		}

		result = util::get_bcd(result);

		//Zero flag
		if(!result) { reg.sc |= ZERO_FLAG; }
		else { reg.sc &= ~ZERO_FLAG; }

		//Carry flag
		if(((reg.sc & 0x20) == 1) && (c_flag > 9)) { reg.sc |= CARRY_FLAG; }
		else if(((reg.sc & 0x20) == 0) && (c_flag > 99)) { reg.sc |= CARRY_FLAG; }
		else { reg.sc &= ~CARRY_FLAG; }

		//Overflow flag
		//Negative flag
		reg.sc &= ~0xC;
	}

	return result;
}

/****** Adds 2 16-bit registers ******/
u16 S1C88::add_u16(u16 reg_one, u16 reg_two)
{
	u16 result;

	u32 c_flag;
	s32 o_flag;

	result = reg_one + reg_two;

	c_flag = reg_one + reg_two;
	o_flag = comp16(reg_one) + comp16(reg_two);

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Carry flag
	if(c_flag >= 0x10000) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	//Overflow flag
	if(o_flag > MAX_16) { reg.sc |= OVERFLOW_FLAG; }
	else if(o_flag < MIN_16) { reg.sc |= OVERFLOW_FLAG; }
	else { reg.sc &= ~OVERFLOW_FLAG; }

	//Negative flag
	if(result & 0x8000) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	return result;
}

/****** Adds 2 8-bit registers + Carry ******/
u8 S1C88::adc_u8(u8 reg_one, u8 reg_two)
{
	u8 carry = (reg.sc & 0x2) ? 1 : 0;
	u8 result;

	u16 c_flag;
	s16 o_flag;

	//Binary mode
	if((reg.sc & 0x10) == 0)
	{
		//Unpacked operation
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one &= 0xF;
			reg_two &= 0xF;

			result = reg_one + reg_two + carry;

			c_flag = reg_one + reg_two + carry;
			o_flag = comp8u(reg_one) + comp8u(reg_two) + carry;

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if(c_flag >= 0x10) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x8) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}
	
		//Packed operation
		else
		{
			result = reg_one + reg_two + carry;

			c_flag = reg_one + reg_two + carry;
			o_flag = comp8(reg_one) + comp8(reg_two) + carry;

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if(c_flag >= 0x100) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}
	}

	//Decimal Mode
	else
	{
		//Convert registers into BCD format - Unpacked
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one = util::get_bcd_int(reg_one & 0xF);
			reg_two = util::get_bcd_int(reg_two & 0xF);

			result = (reg_one + reg_two + carry) % 10;
			c_flag = reg_one + reg_two + carry;
		}

		//Convert registers into BCD format - Packed
		else
		{
			reg_one = util::get_bcd_int(reg_one);
			reg_two = util::get_bcd_int(reg_two);

			result = u16(reg_one + reg_two + carry) % 100;
			c_flag = reg_one + reg_two + carry;
		}

		result = util::get_bcd(result);

		//Zero flag
		if(!result) { reg.sc |= ZERO_FLAG; }
		else { reg.sc &= ~ZERO_FLAG; }

		//Carry flag
		if(((reg.sc & 0x20) == 1) && (c_flag > 9)) { reg.sc |= CARRY_FLAG; }
		else if(((reg.sc & 0x20) == 0) && (c_flag > 99)) { reg.sc |= CARRY_FLAG; }
		else { reg.sc &= ~CARRY_FLAG; }

		//Overflow flag
		//Negative flag
		reg.sc &= ~0xC;
	}

	return result;
}



/****** Adds 2 16-bit registers + Carry ******/
u16 S1C88::adc_u16(u16 reg_one, u16 reg_two)
{
	u8 carry = (reg.sc & 0x2) ? 1 : 0;
	u16 result;

	u32 c_flag;
	s32 o_flag;

	result = reg_one + reg_two + carry;

	c_flag = reg_one + reg_two + carry;
	o_flag = comp16(reg_one) + comp16(reg_two) + carry;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Carry flag
	if(c_flag >= 0x10000) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	//Overflow flag
	if(o_flag > MAX_16) { reg.sc |= OVERFLOW_FLAG; }
	else if(o_flag < MIN_16) { reg.sc |= OVERFLOW_FLAG; }
	else { reg.sc &= ~OVERFLOW_FLAG; }

	//Negative flag
	if(result & 0x8000) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	return result;
}

/****** Subtracts 2 8-bit registers ******/
u8 S1C88::sub_u8(u8 reg_one, u8 reg_two)
{
	u8 result;

	s16 o_flag;

	//Binary mode
	if((reg.sc & 0x10) == 0)
	{
		//Unpacked operation
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one &= 0xF;
			reg_two &= 0xF;

			result = reg_one - reg_two;
			o_flag = comp8u(reg_one) - comp8u(reg_two);

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if(reg_two > reg_one) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x8) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}

		//Packed operation
		else
		{
			result = reg_one - reg_two;
			o_flag = comp8(reg_one) - comp8(reg_two);

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if(reg_two > reg_one) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}
	}

	//Decimal Mode
	else
	{
		//Convert registers into BCD format - Unpacked
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one = util::get_bcd_int(reg_one & 0xF);
			reg_two = util::get_bcd_int(reg_two & 0xF);

			if(reg_one < reg_two) { result = 10 + s16(reg_one - reg_two); }
			else { result = (reg_one - reg_two) % 10; }
		}

		//Convert registers into BCD format - Packed
		else
		{
			reg_one = util::get_bcd_int(reg_one);
			reg_two = util::get_bcd_int(reg_two);

			if(reg_one < reg_two) { result = 100 + s16(reg_one - reg_two); }
			else { result = int(reg_one - reg_two) % 100; }
		}

		result = util::get_bcd(result);

		//Zero flag
		if(!result) { reg.sc |= ZERO_FLAG; }
		else { reg.sc &= ~ZERO_FLAG; }

		//Carry flag
		if(reg_two > reg_one) { reg.sc |= CARRY_FLAG; }
		else { reg.sc &= ~CARRY_FLAG; }

		//Overflow flag
		//Negative flag
		reg.sc &= ~0xC;
	}

	return result;
}

/****** Subtracts 2 16-bit registers ******/
u16 S1C88::sub_u16(u16 reg_one, u16 reg_two)
{
	u16 result;
	s32 o_flag;

	result = reg_one - reg_two;
	o_flag = comp16(reg_one) - comp16(reg_two);

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Carry flag
	if(reg_two > reg_one) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	//Overflow flag
	if(o_flag > MAX_16) { reg.sc |= OVERFLOW_FLAG; }
	else if(o_flag < MIN_16) { reg.sc |= OVERFLOW_FLAG; }
	else { reg.sc &= ~OVERFLOW_FLAG; }

	//Negative flag
	if(result & 0x8000) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	return result;
}

/****** Subtracts 2 8-bit registers - Carry ******/
u8 S1C88::sbc_u8(u8 reg_one, u8 reg_two)
{
	u8 carry = (reg.sc & 0x2) ? 1 : 0;
	u8 result;

	s16 o_flag;

	//Binary mode
	if((reg.sc & 0x10) == 0)
	{
		//Unpacked operation
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one &= 0xF;
			reg_two &= 0xF;

			result = reg_one - reg_two - carry;
			o_flag = comp8u(reg_one) - comp8u(reg_two) - carry;

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if((reg_two + carry) > reg_one) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x8) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}
			
		//Packed operation
		else
		{
			result = reg_one - reg_two - carry;
			o_flag = comp8(reg_one) - comp8(reg_two) - carry;

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if((reg_two + carry) > reg_one) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}
	}

	//Decimal Mode
	else
	{
		//Convert registers into BCD format - Unpacked
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one = util::get_bcd_int(reg_one & 0xF);
			reg_two = util::get_bcd_int(reg_two & 0xF);

			if(reg_one < (reg_two + carry)) { result = 10 + s16(reg_one - reg_two - carry); }
			else { result = (reg_one - reg_two - carry) % 10; }
		}

		//Convert registers into BCD format - Packed
		else
		{
			reg_one = util::get_bcd_int(reg_one);
			reg_two = util::get_bcd_int(reg_two);

			if(reg_one < (reg_two + carry)) { result = 100 + s16(reg_one - reg_two - carry); }
			else { result = (reg_one - reg_two - carry) % 100; }
		}

		result = util::get_bcd(result);

		//Zero flag
		if(!result) { reg.sc |= ZERO_FLAG; }
		else { reg.sc &= ~ZERO_FLAG; }

		//Carry flag
		if((reg_two + carry) > reg_one) { reg.sc |= CARRY_FLAG; }
		else { reg.sc &= ~CARRY_FLAG; }

		//Overflow flag
		//Negative flag
		reg.sc &= ~(OVERFLOW_FLAG | NEGATIVE_FLAG);
	}

	return result;
}

/****** Subtracts 2 16-bit registers - Carry ******/
u16 S1C88::sbc_u16(u16 reg_one, u16 reg_two)
{
	u8 carry = (reg.sc & 0x2) ? 1 : 0;
	u16 result;

	s32 o_flag;

	result = reg_one - reg_two - carry;
	o_flag = comp16(reg_one) - comp16(reg_two) - carry;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Carry flag
	if((reg_two + carry) > reg_one) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	//Overflow flag
	if(o_flag > MAX_16) { reg.sc |= OVERFLOW_FLAG; }
	else if(o_flag < MIN_16) { reg.sc |= OVERFLOW_FLAG; }
	else { reg.sc &= ~OVERFLOW_FLAG; }

	//Negative flag
	if(result & 0x8000) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	return result;
}

/****** Compares 2 8-bit registers ******/
void S1C88::cp_u8(u8 reg_one, u8 reg_two)
{
	u8 result;

	s16 o_flag;

	result = reg_one - reg_two;
	o_flag = comp8(reg_one) - comp8(reg_two);

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Carry flag
	if(reg_two > reg_one) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	//Overflow flag
	if(o_flag > MAX_8) { reg.sc |= OVERFLOW_FLAG; }
	else if(o_flag < MIN_8) { reg.sc |= OVERFLOW_FLAG; }
	else { reg.sc &= ~OVERFLOW_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }
}

/****** Compares 2 16-bit registers ******/
void S1C88::cp_u16(u16 reg_one, u16 reg_two)
{
	u16 result;
	s32 o_flag;

	result = reg_one - reg_two;
	o_flag = comp16(reg_one) - comp16(reg_two);

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Carry flag
	if(reg_two > reg_one) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	//Overflow flag
	if(o_flag > MAX_16) { reg.sc |= OVERFLOW_FLAG; }
	else if(o_flag < MIN_16) { reg.sc |= OVERFLOW_FLAG; }
	else { reg.sc &= ~OVERFLOW_FLAG; }

	//Negative flag
	if(result & 0x8000) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }
}

/****** Increments an 8-bit register ******/
u8 S1C88::inc_u8(u8 reg_one)
{
	reg_one++;

	//Zero flag
	if(!reg_one) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	return reg_one;
}

/****** Increments a 16-bit register ******/
u16 S1C88::inc_u16(u16 reg_one)
{
	reg_one++;

	//Zero flag
	if(!reg_one) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	return reg_one;
}

/****** Decrements an 8-bit register ******/
u8 S1C88::dec_u8(u8 reg_one)
{
	reg_one--;

	//Zero flag
	if(!reg_one) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	return reg_one;
}

/****** Decrements a 16-bit register ******/
u16 S1C88::dec_u16(u16 reg_one)
{
	reg_one--;

	//Zero flag
	if(!reg_one) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	return reg_one;
}

/****** Negates an 8-bit register ******/
u8 S1C88::neg_u8(u8 reg_one)
{
	u8 result;

	s16 o_flag;

	//Binary mode
	if((reg.sc & 0x10) == 0)
	{
		//Unpacked operation
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one &= 0xF;

			result = 0 - reg_one;
			o_flag = 0 - comp8u(reg_one);

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if(reg_one > 0) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8_UNPACK) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x8) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}

		//Packed operation
		else
		{
			result = 0 - reg_one;
			o_flag = 0 - comp8(reg_one);

			//Zero flag
			if(!result) { reg.sc |= ZERO_FLAG; }
			else { reg.sc &= ~ZERO_FLAG; }

			//Carry flag
			if(reg_one > 0) { reg.sc |= CARRY_FLAG; }
			else { reg.sc &= ~CARRY_FLAG; }

			//Overflow flag
			if(o_flag > MAX_8) { reg.sc |= OVERFLOW_FLAG; }
			else if(o_flag < MIN_8) { reg.sc |= OVERFLOW_FLAG; }
			else { reg.sc &= ~OVERFLOW_FLAG; }

			//Negative flag
			if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
			else { reg.sc &= ~NEGATIVE_FLAG; }
		}
	}

	//Decimal Mode
	else
	{
		//Convert registers into BCD format - Unpacked
		if(reg.sc & UNPACK_FLAG)
		{
			reg_one = util::get_bcd_int(reg_one & 0xF);

			result = (10 - reg_one) % 10;
			o_flag = 0 - reg_one;
		}

		//Convert registers into BCD format - Packed
		else
		{
			reg_one = util::get_bcd_int(reg_one);

			result = int(100 - reg_one) % 100;
			o_flag = 0 - reg_one;
		}

		result = util::get_bcd(result);

		//Zero flag
		if(!result) { reg.sc |= ZERO_FLAG; }
		else { reg.sc &= ~ZERO_FLAG; }

		//Carry flag
		if(o_flag < 0) { reg.sc |= CARRY_FLAG; }
		else { reg.sc &= ~CARRY_FLAG; }

		//Overflow flag
		//Negative flag
		reg.sc &= ~0xC;
	}

	return result;
}

/****** Multiplies 2 8-bit registers ******/
u16 S1C88::mlt_u8(u8 reg_one, u8 reg_two)
{
	u16 result = reg_one * reg_two;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x8000) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Overflow flag
	//Carry flag
	reg.sc &= ~(OVERFLOW_FLAG | CARRY_FLAG);

	return result;
}

/****** Divides a 16-bit register by an 8-bit register ******/
u16 S1C88::div_u16(u16 reg_one, u8 reg_two)
{
	u16 result = 0;

	//When dividing by zero, throw exception - TODO
	if(!reg_two)
	{
		std::cout<<"CPU::Warning - Division by Zero Exception\n";
		return reg_one;
	}

	u16 q = reg_one / reg_two;
	u8 r = (reg_one % reg_two);
	
	result = (r << 8) | (q & 0xFF);

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(q & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Overflow flag
	if(q >= 0x100) { reg.sc |= OVERFLOW_FLAG; }
	else { reg.sc &= ~OVERFLOW_FLAG; }

	//Carry flag
	reg.sc &= ~CARRY_FLAG;

	if(q >= 0x100) { return reg_one; }
	else { return result; }
}

/****** Performs a bit test on 2 8-bit registers ******/
void S1C88::bit_u8(u8 reg_one, u8 reg_two)
{
	u8 result = reg_one & reg_two;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }
}

/****** Logical AND 2 8-bit registers ******/
u8 S1C88::and_u8(u8 reg_one, u8 reg_two)
{
	u8 result = reg_one & reg_two;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	return result;
}

/****** Logical OR 2 8-bit registers ******/
u8 S1C88::or_u8(u8 reg_one, u8 reg_two)
{
	u8 result = reg_one | reg_two;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	return result;
}

/****** Logical XOR 2 8-bit registers ******/
u8 S1C88::xor_u8(u8 reg_one, u8 reg_two)
{
	u8 result = reg_one ^ reg_two;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	return result;
}

/****** Logical NOT an 8-bit registers ******/
u8 S1C88::cpl_u8(u8 reg_one)
{
	u8 result = ~reg_one;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	return result;
}

/****** Shift Left Logical ******/
u8 S1C88::sll_u8(u8 reg_one)
{
	u8 result = reg_one << 1;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Carry flag
	if(reg_one & 0x80) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	return result;
}

/****** Shift Left Arithmetic ******/
u8 S1C88::sla_u8(u8 reg_one)
{
	u8 result = reg_one << 1;
	u16 o_flag = reg_one << 1;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Overflow flag
	if(o_flag >= 0x100) { reg.sc |= OVERFLOW_FLAG; }
	else { reg.sc &= ~OVERFLOW_FLAG; }

	//Carry flag
	if(reg_one & 0x80) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	return result;
}

/****** Shift Right Logical ******/
u8 S1C88::srl_u8(u8 reg_one)
{
	u8 result = reg_one >> 1;

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Carry flag
	if(reg_one & 0x1) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	//Negative flag
	reg.sc &= ~NEGATIVE_FLAG;

	return result;
}

/****** Shift Right Arithmetic ******/
u8 S1C88::sra_u8(u8 reg_one)
{
	u8 result = reg_one >> 1;
	if(reg_one & 0x80) { result |= 0x80; }

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Carry flag
	if(reg_one & 0x1) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	//Overflow flag
	reg.sc &= ~OVERFLOW_FLAG;

	return result;
}

/****** Rotate Left Circular ******/
u8 S1C88::rlc_u8(u8 reg_one)
{
	u8 result = reg_one << 1;
	if(reg_one & 0x80) { result |= 0x1; }

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Carry flag
	if(reg_one & 0x80) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	return result;
}

/****** Rotate Left with Carry ******/
u8 S1C88::rl_u8(u8 reg_one)
{
	u8 result = reg_one << 1;
	if(reg.sc & 0x2) { result |= 0x1; }

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Carry flag
	if(reg_one & 0x80) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	return result;
}

/****** Rotate Right Circular ******/
u8 S1C88::rrc_u8(u8 reg_one)
{
	u8 result = reg_one >> 1;
	if(reg_one & 0x1) { result |= 0x80; }

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Carry flag
	if(reg_one & 0x1) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	return result;
}

/****** Rotate Right with Carry ******/
u8 S1C88::rr_u8(u8 reg_one)
{
	u8 result = reg_one >> 1;
	if(reg.sc & 0x2) { result |= 0x80; }

	//Zero flag
	if(!result) { reg.sc |= ZERO_FLAG; }
	else { reg.sc &= ~ZERO_FLAG; }

	//Negative flag
	if(result & 0x80) { reg.sc |= NEGATIVE_FLAG; }
	else { reg.sc &= ~NEGATIVE_FLAG; }

	//Carry flag
	if(reg_one & 0x1) { reg.sc |= CARRY_FLAG; }
	else { reg.sc &= ~CARRY_FLAG; }

	return result;
}

/****** Convert an 8-bit unsigned value to 2's compliment ******/
s8 S1C88::comp8(u8 reg_one)
{
	s8 result = 0;

	if(reg_one & 0x80)
	{
		reg_one--;
		reg_one = ~reg_one;
		result = -1 * reg_one;
	}

	else { result = reg_one; }

	return result;
}

/****** Convert an 8-bit unsigned value to 2's compliment - Unpacked version ******/
s8 S1C88::comp8u(u8 reg_one)
{
	s8 result = 0;

	if(reg_one & 0x8)
	{
		reg_one--;
		reg_one = (~reg_one & 0xF);
		result = -1 * reg_one;
	}

	else { result = (reg_one & 0xF); }

	return result;
}

/****** Convert an 16-bit unsigned value to 2's compliment ******/
s16 S1C88::comp16(u16 reg_one)
{
	s16 result = 0;

	if(reg_one & 0x8000)
	{
		reg_one--;
		reg_one = ~reg_one;
		result = -1 * reg_one;
	}

	else { result = reg_one; }

	return result;
} 

/****** Fetch, decode, and execute instruction ******/
void S1C88::execute()
{
	//Handle HALT status
	if(halt) { system_cycles = 4; return; }

	update_regs();

	skip_irq = false;

	u32 temp_val = 0;
	s16 s_temp = 0;

	//Read 1st byte of opcode
	opcode = mem->read_u8(reg.pc_ex++);
	reg.pc++;

	//Decode (and fetch additional bytes) and execute instructions
	switch(opcode)
	{
		//ADD A, A
		case 0x00:
			reg.a = add_u8(reg.a, reg.a);
			system_cycles = 8;
			break;

		//ADD A, B
		case 0x01:
			reg.a = add_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//ADD A, #nn
		case 0x02:
			reg.a = add_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//ADD A, [HL]
		case 0x03:
			reg.a = add_u8(reg.a, mem->read_u8(reg.hl_ex));
			system_cycles = 8;
			break;

		//ADD A, [BR + #nn]
		case 0x04:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			reg.a = add_u8(reg.a, temp_val);
			system_cycles = 12;

			reg.pc++;
			break;

		//ADD A, [#nnnn]
		case 0x05:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.a = add_u8(reg.a, mem->read_u8(temp_val));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//ADD A, [IX]
		case 0x06:
			reg.a = add_u8(reg.a, mem->read_u8(reg.ix_ex));
			system_cycles = 8;
			break;

		//ADD A, [IY]
		case 0x07:
			reg.a = add_u8(reg.a, mem->read_u8(reg.iy_ex));
			system_cycles = 8;
			break;

		//ADC A, A
		case 0x08:
			reg.a = adc_u8(reg.a, reg.a);
			system_cycles = 8;
			break;

		//ADC A, B
		case 0x09:
			reg.a = adc_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//ADC A, #nn
		case 0x0A:
			reg.a = adc_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//ADC A, [HL]
		case 0x0B:
			reg.a = adc_u8(reg.a, mem->read_u8(reg.hl_ex));
			system_cycles = 8;
			break;

		//ADC A, [BR + #nn]
		case 0x0C:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			reg.a = adc_u8(reg.a, temp_val);
			system_cycles = 12;

			reg.pc++;
			break;

		//ADC A, [#nnnn]
		case 0x0D:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.a = adc_u8(reg.a, mem->read_u8(temp_val));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//ADC A, [IX]
		case 0x0E:
			reg.a = adc_u8(reg.a, mem->read_u8(reg.ix_ex));
			system_cycles = 8;
			break;

		//ADC A, [IY]
		case 0x0F:
			reg.a = adc_u8(reg.a, mem->read_u8(reg.iy_ex));
			system_cycles = 8;
			break;

		//SUB A, A
		case 0x10:
			reg.a = sub_u8(reg.a, reg.a);
			system_cycles = 8;
			break;

		//SUB A, A
		case 0x11:
			reg.a = sub_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//SUB A, #nn
		case 0x12:
			reg.a = sub_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//SUB A, [HL]
		case 0x13:
			reg.a = sub_u8(reg.a, mem->read_u8(reg.hl_ex));
			system_cycles = 8;
			break;

		//SUB A, [BR + #nn]
		case 0x14:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			reg.a = sub_u8(reg.a, temp_val);
			system_cycles = 12;

			reg.pc++;
			break;

		//SUB A, [#nnnn]
		case 0x15:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.a = sub_u8(reg.a, mem->read_u8(temp_val));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//SUB A, [IX]
		case 0x16:
			reg.a = sub_u8(reg.a, mem->read_u8(reg.ix_ex));
			system_cycles = 8;
			break;

		//SUB A, [IY]
		case 0x17:
			reg.a = sub_u8(reg.a, mem->read_u8(reg.iy_ex));
			system_cycles = 8;
			break;

		//SBC A, A
		case 0x18:
			reg.a = sbc_u8(reg.a, reg.a);
			system_cycles = 8;
			break;

		//SBC A, B
		case 0x19:
			reg.a = sbc_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//SBC A, #nn
		case 0x1A:
			reg.a = sbc_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//SBC A, [HL]
		case 0x1B:
			reg.a = sbc_u8(reg.a, mem->read_u8(reg.hl_ex));
			system_cycles = 8;
			break;

		//SBC A, [BR + #nn]
		case 0x1C:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			reg.a = sbc_u8(reg.a, temp_val);
			system_cycles = 12;

			reg.pc++;
			break;

		//SBC A, [#nnnn]
		case 0x1D:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.a = sbc_u8(reg.a, mem->read_u8(temp_val));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//SBC A, [IX]
		case 0x1E:
			reg.a = sbc_u8(reg.a, mem->read_u8(reg.ix_ex));
			system_cycles = 8;
			break;

		//SBC A, [IY]
		case 0x1F:
			reg.a = sbc_u8(reg.a, mem->read_u8(reg.iy_ex));
			system_cycles = 8;
			break;

		//AND A, A
		case 0x20:
			reg.a = and_u8(reg.a, reg.a);
			system_cycles = 8;
			break;

		//AND A, B
		case 0x21:
			reg.a = and_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//AND A, #nn
		case 0x22:
			reg.a = and_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//AND A, [HL]
		case 0x23:
			reg.a = and_u8(reg.a, mem->read_u8(reg.hl_ex));
			system_cycles = 8;
			break;

		//AND A, [BR + #nn]
		case 0x24:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			reg.a = and_u8(reg.a, temp_val);
			system_cycles = 8;

			reg.pc++;
			break;

		//AND A, [#nnnn]
		case 0x25:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.a = and_u8(reg.a, mem->read_u8(temp_val));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//AND A, [IX]
		case 0x26:
			reg.a = and_u8(reg.a, mem->read_u8(reg.ix_ex));
			system_cycles = 8;
			break;

		//AND A, [IY]
		case 0x27:
			reg.a = and_u8(reg.a, mem->read_u8(reg.iy_ex));
			system_cycles = 8;
			break;

		//OR A, A
		case 0x28:
			reg.a = or_u8(reg.a, reg.a);
			system_cycles = 8;
			break;

		//OR A, B
		case 0x29:
			reg.a = or_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//OR A, #nn
		case 0x2A:
			reg.a = or_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//OR A, [HL]
		case 0x2B:
			reg.a = or_u8(reg.a, mem->read_u8(reg.hl_ex));
			system_cycles = 8;
			break;

		//OR A, [BR + #nn]
		case 0x2C:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			reg.a = or_u8(reg.a, temp_val);
			system_cycles = 8;

			reg.pc++;
			break;

		//OR A, [#nnnn]
		case 0x2D:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.a = or_u8(reg.a, mem->read_u8(temp_val));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//OR A, [IX]
		case 0x2E:
			reg.a = or_u8(reg.a, mem->read_u8(reg.ix_ex));
			system_cycles = 8;
			break;

		//OR A, [IY]
		case 0x2F:
			reg.a = or_u8(reg.a, mem->read_u8(reg.iy_ex));
			system_cycles = 8;
			break;

		//CP A, A
		case 0x30:
			cp_u8(reg.a, reg.a);
			system_cycles = 8;
			break;

		//CP A, B
		case 0x31:
			cp_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//CP A, #nn
		case 0x32:
			cp_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//CP A, [HL]
		case 0x33:
			cp_u8(reg.a, mem->read_u8(reg.hl_ex));
			system_cycles = 8;
			break;

		//CP A, [BR + #nn]
		case 0x34:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			cp_u8(reg.a, temp_val);
			system_cycles = 12;

			reg.pc++;
			break;

		//CP A, [#nnnn]
		case 0x35:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			cp_u8(reg.a, mem->read_u8(temp_val));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//CP A, [IX]
		case 0x36:
			cp_u8(reg.a, mem->read_u8(reg.ix_ex));
			system_cycles = 8;
			break;

		//CP A, [IY]
		case 0x37:
			cp_u8(reg.a, mem->read_u8(reg.iy_ex));
			system_cycles = 8;
			break;

		//XOR A, A
		case 0x38:
			reg.a = xor_u8(reg.a, reg.a);
			system_cycles = 8;
			break;

		//XOR A, B
		case 0x39:
			reg.a = xor_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//XOR A, #nn
		case 0x3A:
			reg.a = xor_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//XOR A, [HL]
		case 0x3B:
			reg.a = xor_u8(reg.a, mem->read_u8(reg.hl_ex));
			system_cycles = 8;
			break;

		//XOR A, [BR + #nn]
		case 0x3C:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			reg.a = xor_u8(reg.a, temp_val);
			system_cycles = 8;

			reg.pc++;
			break;

		//XOR A, [#nnnn]
		case 0x3D:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.a = xor_u8(reg.a, mem->read_u8(temp_val));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//XOR A, [IX]
		case 0x3E:
			reg.a = xor_u8(reg.a, mem->read_u8(reg.ix_ex));
			system_cycles = 8;
			break;

		//XOR A, [IY]
		case 0x3F:
			reg.a = xor_u8(reg.a, mem->read_u8(reg.iy_ex));
			system_cycles = 8;
			break;

		//LD A, A
		case 0x40:
			system_cycles = 4;
			break;

		//LD A, B
		case 0x41:
			reg.a = reg.b;
			system_cycles = 4;
			break;

		//LD A, L
		case 0x42:
			reg.a = reg.l;
			system_cycles = 4;
			break;

		//LD A, H
		case 0x43:
			reg.a = reg.h;
			system_cycles = 4;
			break;

		//LD A, [BR + #nn]
		case 0x44:
			reg.a = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			system_cycles = 12;

			reg.pc++;
			break;

		//LD A, [HL]
		case 0x45:
			reg.a = mem->read_u8(reg.hl_ex);
			system_cycles = 8;
			break;

		//LD A, [IX]
		case 0x46:
			reg.a = mem->read_u8(reg.ix_ex);
			system_cycles = 8;
			break;

		//LD A, [IY]
		case 0x47:
			reg.a = mem->read_u8(reg.iy_ex);
			system_cycles = 8;
			break;

		//LD B, A
		case 0x48:
			reg.b = reg.a;
			system_cycles = 4;
			break;

		//LD B, B
		case 0x49:
			system_cycles = 4;
			break;

		//LD B, L
		case 0x4A:
			reg.b = reg.l;
			system_cycles = 4;
			break;

		//LD B, H
		case 0x4B:
			reg.b = reg.h;
			system_cycles = 4;
			break;

		//LD B, [BR + #nn]
		case 0x4C:
			reg.b = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			system_cycles = 12;

			reg.pc++;
			break;

		//LD B, [HL]
		case 0x4D:
			reg.b = mem->read_u8(reg.hl_ex);
			system_cycles = 8;
			break;

		//LD B, [IX]
		case 0x4E:
			reg.b = mem->read_u8(reg.ix_ex);
			system_cycles = 8;
			break;

		//LD B, [IY]
		case 0x4F:
			reg.b = mem->read_u8(reg.iy_ex);
			system_cycles = 8;
			break;

		//LD L, A
		case 0x50:
			reg.l = reg.a;
			system_cycles = 4;
			break;

		//LD L, B
		case 0x51:
			reg.l = reg.b;
			system_cycles = 4;
			break;

		//LD L, L
		case 0x52:
			system_cycles = 4;
			break;

		//LD L, H
		case 0x53:
			reg.l = reg.h;
			system_cycles = 4;
			break;

		//LD L, [BR + #nn]
		case 0x54:
			reg.l = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			system_cycles = 12;

			reg.pc++;
			break;

		//LD L, [HL]
		case 0x55:
			reg.l = mem->read_u8(reg.hl_ex);
			system_cycles = 8;
			break;

		//LD L, [IX]
		case 0x56:
			reg.l = mem->read_u8(reg.ix_ex);
			system_cycles = 8;
			break;

		//LD L, [IY]
		case 0x57:
			reg.l = mem->read_u8(reg.iy_ex);
			system_cycles = 8;
			break;

		//LD H, A
		case 0x58:
			reg.h = reg.a;
			system_cycles = 4;
			break;

		//LD H, B
		case 0x59:
			reg.h = reg.b;
			system_cycles = 4;
			break;

		//LD H, L
		case 0x5A:
			reg.h = reg.l;
			system_cycles = 4;
			break;

		//LD H, H
		case 0x5B:
			system_cycles = 4;
			break;

		//LD H, [BR + #nn]
		case 0x5C:
			reg.h = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			system_cycles = 12;

			reg.pc++;
			break;

		//LD H, [HL]
		case 0x5D:
			reg.h = mem->read_u8(reg.hl_ex);
			system_cycles = 8;
			break;

		//LD H, [IX]
		case 0x5E:
			reg.h = mem->read_u8(reg.ix_ex);
			system_cycles = 8;
			break;

		//LD H, [IY]
		case 0x5F:
			reg.h = mem->read_u8(reg.iy_ex);
			system_cycles = 8;
			break;

		//LD [IX], A
		case 0x60:
			mem->write_u8(reg.ix_ex, reg.a);
			system_cycles = 8;
			break;

		//LD [IX], B
		case 0x61:
			mem->write_u8(reg.ix_ex, reg.b);
			system_cycles = 8;
			break;

		//LD [IX], L
		case 0x62:
			mem->write_u8(reg.ix_ex, reg.l);
			system_cycles = 8;
			break;

		//LD [IX], H
		case 0x63:
			mem->write_u8(reg.ix_ex, reg.h);
			system_cycles = 8;
			break;

		//LD [IX], [BR + #nn]
		case 0x64:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			mem->write_u8(reg.ix_ex, temp_val);
			system_cycles = 16;

			reg.pc++;
			break;

		//LD [IX], [HL]
		case 0x65:
			mem->write_u8(reg.ix_ex, mem->read_u8(reg.hl_ex));
			system_cycles = 12;
			break;

		//LD [IX], [IX]
		case 0x66:
			mem->write_u8(reg.ix_ex, mem->read_u8(reg.ix_ex));
			system_cycles = 12;
			break;

		//LD [IX], [IY]
		case 0x67:
			mem->write_u8(reg.ix_ex, mem->read_u8(reg.iy_ex));
			system_cycles = 12;
			break;

		//LD [HL], A
		case 0x68:
			mem->write_u8(reg.hl_ex, reg.a);
			system_cycles = 8;
			break;

		//LD [HL], B
		case 0x69:
			mem->write_u8(reg.hl_ex, reg.b);
			system_cycles = 8;
			break;

		//LD [HL], L
		case 0x6A:
			mem->write_u8(reg.hl_ex, reg.l);
			system_cycles = 8;
			break;

		//LD [HL], H
		case 0x6B:
			mem->write_u8(reg.hl_ex, reg.h);
			system_cycles = 8;
			break;

		//LD [HL], [BR + #nn]
		case 0x6C:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			mem->write_u8(reg.hl_ex, temp_val);
			system_cycles = 16;

			reg.pc++;
			break;
		
		//LD [HL], [HL]
		case 0x6D:
			mem->write_u8(reg.hl_ex, mem->read_u8(reg.hl_ex));
			system_cycles = 12;
			break;

		//LD [HL], [IX]
		case 0x6E:
			mem->write_u8(reg.hl_ex, mem->read_u8(reg.ix_ex));
			system_cycles = 12;
			break;

		//LD [HL], [IY]
		case 0x6F:
			mem->write_u8(reg.hl_ex, mem->read_u8(reg.iy_ex));
			system_cycles = 12;
			break;

		//LD [IY], A
		case 0x70:
			mem->write_u8(reg.iy_ex, reg.a);
			system_cycles = 8;
			break;

		//LD [IY], B
		case 0x71:
			mem->write_u8(reg.iy_ex, reg.b);
			system_cycles = 8;
			break;

		//LD [IY], L
		case 0x72:
			mem->write_u8(reg.iy_ex, reg.l);
			system_cycles = 8;
			break;

		//LD [IY], H
		case 0x73:
			mem->write_u8(reg.iy_ex, reg.h);
			system_cycles = 8;
			break;

		//LD [IY], [BR + #nn]
		case 0x74:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			mem->write_u8(reg.iy_ex, temp_val);
			system_cycles = 16;

			reg.pc++;
			break;

		//LD [IY], [HL]
		case 0x75:
			mem->write_u8(reg.iy_ex, mem->read_u8(reg.hl_ex));
			system_cycles = 12;
			break;

		//LD [IY], [IX]
		case 0x76:
			mem->write_u8(reg.iy_ex, mem->read_u8(reg.ix_ex));
			system_cycles = 12;
			break;

		//LD [IY], [IY]
		case 0x77:
			mem->write_u8(reg.iy_ex, mem->read_u8(reg.iy_ex));
			system_cycles = 12;
			break;

		//LD [BR + #nn], A
		case 0x78:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, reg.a);
			system_cycles = 12;

			reg.pc++;
			break;

		//LD [BR + #nn], B
		case 0x79:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, reg.b);
			system_cycles = 12;

			reg.pc++;
			break;

		//LD [BR + #nn], L
		case 0x7A:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, reg.l);
			system_cycles = 12;

			reg.pc++;
			break;

		//LD [BR + #nn], H
		case 0x7B:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, reg.h);
			system_cycles = 12;

			reg.pc++;
			break;

		//LD [BR + #nn], [HL]
		case 0x7D:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, mem->read_u8(reg.hl_ex));
			system_cycles = 16;

			reg.pc++;
			break;

		//LD [BR + #nn], [IX]
		case 0x7E:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, mem->read_u8(reg.ix_ex));
			system_cycles = 16;

			reg.pc++;
			break;

		//LD [BR + #nn], [IY]
		case 0x7F:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, mem->read_u8(reg.iy_ex));
			system_cycles = 16;

			reg.pc++;
			break;

		//INC A
		case 0x80:
			reg.a = inc_u8(reg.a);
			system_cycles = 8;
			break;

		//INC B
		case 0x81:
			reg.b = inc_u8(reg.b);
			system_cycles = 8;
			break;

		//INC L
		case 0x82:
			reg.l = inc_u8(reg.l);
			system_cycles = 8;
			break;

		//INC H
		case 0x83:
			reg.h = inc_u8(reg.h);
			system_cycles = 8;
			break;

		//INC BR
		case 0x84:
			reg.br = inc_u8(reg.br);
			system_cycles = 8;
			break;

		//INC [BR + #nn]
		case 0x85:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, inc_u8(mem->read_u8(temp_val)));
			system_cycles = 16;

			reg.pc++;
			break;

		//INC [HL]
		case 0x86:
			temp_val = inc_u8(mem->read_u8(reg.hl_ex));
			mem->write_u8(reg.hl_ex, temp_val);
			system_cycles = 12;
			break;
			
		//INC SP
		case 0x87:
			reg.sp = inc_u16(reg.sp);
			system_cycles = 8;
			break;

		//DEC A
		case 0x88:
			reg.a = dec_u8(reg.a);
			system_cycles = 8;
			break;

		//DEC B
		case 0x89:
			reg.b = dec_u8(reg.b);
			system_cycles = 8;
			break;

		//DEC L
		case 0x8A:
			reg.l = dec_u8(reg.l);
			system_cycles = 8;
			break;

		//DEC H
		case 0x8B:
			reg.h = dec_u8(reg.h);
			system_cycles = 8;
			break;

		//DEC BR
		case 0x8C:
			reg.br = dec_u8(reg.br);
			system_cycles = 8;
			break;

		//DEC [BR + #nn]
		case 0x8D:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, dec_u8(mem->read_u8(temp_val)));
			system_cycles = 16;

			reg.pc++;
			break;

		//DEC [HL]
		case 0x8E:
			temp_val = dec_u8(mem->read_u8(reg.hl_ex));
			mem->write_u8(reg.hl_ex, temp_val);
			system_cycles = 12;
			break;
			
		//DEC SP
		case 0x8F:
			reg.sp = dec_u16(reg.sp);
			system_cycles = 8;
			break;
		
		//INC BA
		case 0x90:
			reg.ba = inc_u16(reg.ba);
			system_cycles = 8;
			break;

		//INC HL
		case 0x91:
			reg.hl = inc_u16(reg.hl);
			system_cycles = 8;
			break;

		//INC IX
		case 0x92:
			reg.ix = inc_u16(reg.ix);
			system_cycles = 8;
			break;

		//INC IY
		case 0x93:
			reg.iy = inc_u16(reg.iy);
			system_cycles = 8;
			break;

		//BIT A, B
		case 0x94:
			bit_u8(reg.a, reg.b);
			system_cycles = 8;
			break;

		//BIT [HL], #nn
		case 0x95:
			bit_u8(mem->read_u8(reg.hl_ex), mem->read_u8(reg.pc_ex++));
			system_cycles = 12;

			reg.pc++;
			break;

		//BIT A, #nn
		case 0x96:
			bit_u8(reg.a, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//BIT B, #nn
		case 0x97:
			bit_u8(reg.b, mem->read_u8(reg.pc_ex++));
			system_cycles = 8;

			reg.pc++;
			break;

		//DEC BA
		case 0x98:
			reg.ba = dec_u16(reg.ba);
			system_cycles = 8;
			break;

		//DEC HL
		case 0x99:
			reg.hl = dec_u16(reg.hl);
			system_cycles = 8;
			break;

		//DEC IX
		case 0x9A:
			reg.ix = dec_u16(reg.ix);
			system_cycles = 8;
			break;

		//DEC IY
		case 0x9B:
			reg.iy = dec_u16(reg.iy);
			system_cycles = 8;
			break;

		//AND SC, #nn
		case 0x9C:
			reg.sc = and_u8(reg.sc, mem->read_u8(reg.pc_ex++));
			system_cycles = 12;
			skip_irq = true;

			reg.pc++;
			break;

		//OR SC, #nn
		case 0x9D:
			reg.sc = or_u8(reg.sc, mem->read_u8(reg.pc_ex++));
			system_cycles = 12;
			skip_irq = true;

			reg.pc++;
			break;

		//XOR SC, #nn
		case 0x9E:
			reg.sc = xor_u8(reg.sc, mem->read_u8(reg.pc_ex++));
			system_cycles = 12;
			skip_irq = true;

			reg.pc++;
			break;

		//LD SC, #nn
		case 0x9F:
			reg.sc = mem->read_u8(reg.pc_ex++);
			system_cycles = 12;
			skip_irq = true;

			reg.pc++;
			break;

		//PUSH BA
		case 0xA0:
			mem->write_u8(--reg.sp, reg.b);
			mem->write_u8(--reg.sp, reg.a);
			system_cycles = 16;
			break;

		//PUSH HL
		case 0xA1:
			mem->write_u8(--reg.sp, reg.h);
			mem->write_u8(--reg.sp, reg.l);
			system_cycles = 16;
			break;

		//PUSH IX
		case 0xA2:
			mem->write_u8(--reg.sp, (reg.ix >> 8));
			mem->write_u8(--reg.sp, (reg.ix & 0xFF));
			system_cycles = 16;
			break;

		//PUSH IY
		case 0xA3:
			mem->write_u8(--reg.sp, (reg.iy >> 8));
			mem->write_u8(--reg.sp, (reg.iy & 0xFF));
			system_cycles = 16;
			break;

		//PUSH BR
		case 0xA4:
			mem->write_u8(--reg.sp, reg.br);
			system_cycles = 12;
			break;

		//PUSH EP
		case 0xA5:
			mem->write_u8(--reg.sp, reg.ep);
			system_cycles = 12;
			break;

		//PUSH IP
		case 0xA6:
			mem->write_u8(--reg.sp, reg.xp);
			mem->write_u8(--reg.sp, reg.yp);
			system_cycles = 16;
			break;

		//PUSH SC
		case 0xA7:
			mem->write_u8(--reg.sp, reg.sc);
			system_cycles = 12;
			break;

		//POP BA
		case 0xA8:
			reg.a = mem->read_u8(reg.sp++);
			reg.b = mem->read_u8(reg.sp++);
			system_cycles = 12;
			break;

		//POP HL
		case 0xA9:
			reg.l = mem->read_u8(reg.sp++);
			reg.h = mem->read_u8(reg.sp++);
			system_cycles = 12;
			break;

		//POP IX
		case 0xAA:
			reg.ix = mem->read_u8(reg.sp++);
			reg.ix |= (mem->read_u8(reg.sp++) << 8);
			system_cycles = 12;
			break;

		//POP IY
		case 0xAB:
			reg.iy = mem->read_u8(reg.sp++);
			reg.iy |= (mem->read_u8(reg.sp++) << 8);
			system_cycles = 12;
			break;

		//POP BR
		case 0xAC:
			reg.br = mem->read_u8(reg.sp++);
			system_cycles = 8;
			break;

		//POP EP
		case 0xAD:
			reg.ep = mem->read_u8(reg.sp++);
			system_cycles = 8;
			break;

		//POP IP
		case 0xAE:
			reg.yp = mem->read_u8(reg.sp++);
			reg.xp = mem->read_u8(reg.sp++);
			system_cycles = 12;
			break;

		//POP SC
		case 0xAF:
			reg.sc = mem->read_u8(reg.sp++);
			system_cycles = 8;
			skip_irq = true;
			break;

		//LD A, #nn
		case 0xB0:
			reg.a = mem->read_u8(reg.pc_ex++);
			system_cycles = 8;

			reg.pc++;
			break;

		//LD B, #nn
		case 0xB1:
			reg.b = mem->read_u8(reg.pc_ex++);
			system_cycles = 8;

			reg.pc++;
			break;

		//LD L, #nn
		case 0xB2:
			reg.l = mem->read_u8(reg.pc_ex++);
			system_cycles = 8;

			reg.pc++;
			break;

		//LD H, #nn
		case 0xB3:
			reg.h = mem->read_u8(reg.pc_ex++);
			system_cycles = 8;

			reg.pc++;
			break;

		//LD BR, #nn
		case 0xB4:
			reg.br = mem->read_u8(reg.pc_ex++);
			system_cycles = 8;

			reg.pc++;
			break;

		//LD [HL], #nn
		case 0xB5:
			mem->write_u8(reg.hl_ex, mem->read_u8(reg.pc_ex++));
			system_cycles = 12;

			reg.pc++;
			break;

		//LD [IX], #nn
		case 0xB6:
			mem->write_u8(reg.ix_ex, mem->read_u8(reg.pc_ex++));
			system_cycles = 12;

			reg.pc++;
			break;

		//LD [IY], #nn
		case 0xB7:
			mem->write_u8(reg.iy_ex, mem->read_u8(reg.pc_ex++));
			system_cycles = 12;

			reg.pc++;
			break;

		//LD BA, [#nnnn]
		case 0xB8:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.ba = mem->read_u16(temp_val);
			system_cycles = 20;

			reg.pc += 2;
			break;

		//LD HL, [#nnnn]
		case 0xB9:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.hl = mem->read_u16(temp_val);
			system_cycles = 20;

			reg.pc += 2;
			break;

		//LD IX, [#nnnn]
		case 0xBA:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.ix = mem->read_u16(temp_val);
			system_cycles = 20;

			reg.pc += 2;
			break;

		//LD IY, [#nnnn]
		case 0xBB:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.iy = mem->read_u16(temp_val);
			system_cycles = 20;

			reg.pc += 2;
			break;

		//LD [#nnnn], BA
		case 0xBC:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			mem->write_u16(temp_val, reg.ba);
			system_cycles = 20;

			reg.pc += 2;
			break;

		//LD [#nnnn], HL
		case 0xBD:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			mem->write_u16(temp_val, reg.hl);
			system_cycles = 20;

			reg.pc += 2;
			break;

		//LD [#nnnn], IX
		case 0xBE:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			mem->write_u16(temp_val, reg.ix);
			system_cycles = 20;

			reg.pc += 2;
			break;

		//LD [#nnnn], IY
		case 0xBF:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			mem->write_u16(temp_val, reg.iy);
			system_cycles = 20;

			reg.pc += 2;
			break;

		//ADD BA, #nnnn
		case 0xC0:
			temp_val = mem->read_u16(reg.pc_ex);
			reg.ba = add_u16(reg.ba, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//ADD HL, #nnnn
		case 0xC1:
			temp_val = mem->read_u16(reg.pc_ex);
			reg.hl = add_u16(reg.hl, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//ADD IX, #nnnn
		case 0xC2:
			temp_val = mem->read_u16(reg.pc_ex);
			reg.ix = add_u16(reg.ix, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//ADD IY, #nnnn
		case 0xC3:
			temp_val = mem->read_u16(reg.pc_ex);
			reg.iy = add_u16(reg.iy, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//LD BA, #nnnn
		case 0xC4:
			reg.ba = mem->read_u16(reg.pc_ex);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//LD HL, #nnnn
		case 0xC5:
			reg.hl = mem->read_u16(reg.pc_ex);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//LD IX, #nnnn
		case 0xC6:
			reg.ix = mem->read_u16(reg.pc_ex);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//LD IY, #nnnn
		case 0xC7:
			reg.iy = mem->read_u16(reg.pc_ex);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//EX BA, HL
		case 0xC8:
			temp_val = reg.ba;
			reg.ba = reg.hl;
			reg.hl = temp_val;
			system_cycles = 12;
			break;

		//EX BA, IX
		case 0xC9:
			temp_val = reg.ba;
			reg.ba = reg.ix;
			reg.ix = temp_val;
			system_cycles = 12;
			break;

		//EX BA, IY
		case 0xCA:
			temp_val = reg.ba;
			reg.ba = reg.iy;
			reg.iy = temp_val;
			system_cycles = 12;
			break;

		//EX BA, SP
		case 0xCB:
			temp_val = reg.ba;
			reg.ba = reg.sp;
			reg.sp = temp_val;
			system_cycles = 12;
			break;

		//EX A, B
		case 0xCC:
			temp_val = reg.a;
			reg.a = reg.b;
			reg.b = temp_val;
			system_cycles = 8;
			break;

		//EX A, [HL]
		case 0xCD:
			temp_val = reg.a;
			reg.a = mem->read_u8(reg.hl_ex);
			mem->write_u8(reg.hl_ex, temp_val);
			system_cycles = 12;
			break;

		//Extend 0xCE instructions
		case 0xCE:
			opcode = (opcode << 8) | mem->read_u8(reg.pc_ex++);
			reg.pc++;

			switch(opcode & 0xFF)
			{
				//ADD A, [IX + #ss]
				case 0x00:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = add_u8(reg.a, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//ADD A, [IY + #ss]
				case 0x01:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = add_u8(reg.a, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//ADD A, [IX + L]
				case 0x02:
					reg.a = add_u8(reg.a, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//ADD A, [IY + L]
				case 0x03:
					reg.a = add_u8(reg.a, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//ADD [HL], A
				case 0x04:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = add_u8(temp_val, reg.a);
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 16;
					break;

				//ADD [HL], #nn
				case 0x05:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = add_u8(temp_val, mem->read_u8(reg.pc_ex++));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;

					reg.pc++;
					break;

				//ADD [HL], [IX]
				case 0x06:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = add_u8(temp_val, mem->read_u8(reg.ix_ex));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;
					break;

				//ADD [HL], [IY]
				case 0x07:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = add_u8(temp_val, mem->read_u8(reg.iy_ex));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;
					break;

				//ADC A, [IX + #ss]
				case 0x08:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = adc_u8(reg.a, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//ADC A, [IY + #ss]
				case 0x09:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = adc_u8(reg.a, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//ADC A, [IX + L]
				case 0x0A:
					reg.a = adc_u8(reg.a, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//ADC A, [IY + L]
				case 0x0B:
					reg.a = adc_u8(reg.a, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//ADC [HL], A
				case 0x0C:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = adc_u8(temp_val, reg.a);
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 16;
					break;

				//ADC [HL], #nn
				case 0x0D:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = adc_u8(temp_val, mem->read_u8(reg.pc_ex++));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;

					reg.pc++;
					break;

				//ADC [HL], [IX]
				case 0x0E:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = adc_u8(temp_val, mem->read_u8(reg.ix_ex));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;
					break;

				//ADC [HL], [IY]
				case 0x0F:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = adc_u8(temp_val, mem->read_u8(reg.iy_ex));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;
					break;

				//SUB A, [IX + #ss]
				case 0x10:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = sub_u8(reg.a, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//SUB A, [IY + #ss]
				case 0x11:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = sub_u8(reg.a, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//SUB A, [IX + L]
				case 0x12:
					reg.a = sub_u8(reg.a, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//SUB A, [IY + L]
				case 0x13:
					reg.a = sub_u8(reg.a, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//SUB [HL], A
				case 0x14:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = sub_u8(temp_val, reg.a);
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 16;
					break;

				//SUB [HL], #nn
				case 0x15:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = sub_u8(temp_val, mem->read_u8(reg.pc_ex++));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;

					reg.pc++;
					break;

				//SUB [HL], [IX]
				case 0x16:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = sub_u8(temp_val, mem->read_u8(reg.ix_ex));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;
					break;

				//SUB [HL], [IY]
				case 0x17:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = sub_u8(temp_val, mem->read_u8(reg.iy_ex));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;
					break;

				//SBC A, [IX + #ss]
				case 0x18:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = sbc_u8(reg.a, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//SBC A, [IY + #ss]
				case 0x19:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = sbc_u8(reg.a, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//SBC A, [IX + L]
				case 0x1A:
					reg.a = sbc_u8(reg.a, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//SBC A, [IY + L]
				case 0x1B:
					reg.a = sbc_u8(reg.a, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//SBC [HL], A
				case 0x1C:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = sbc_u8(temp_val, reg.a);
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 16;
					break;

				//SBC [HL], #nn
				case 0x1D:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = sbc_u8(temp_val, mem->read_u8(reg.pc_ex++));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;

					reg.pc++;
					break;

				//SBC [HL], [IX]
				case 0x1E:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = sbc_u8(temp_val, mem->read_u8(reg.ix_ex));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;
					break;

				//SBC [HL], [IY]
				case 0x1F:
					temp_val = mem->read_u8(reg.hl_ex);
					temp_val = sbc_u8(temp_val, mem->read_u8(reg.iy_ex));
					mem->write_u8(reg.hl_ex, temp_val);
					system_cycles = 20;
					break;

				//AND A, [IX + #ss]
				case 0x20:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = and_u8(reg.a, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//AND A, [IY + #ss]
				case 0x21:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = and_u8(reg.a, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//AND A, [IX + L]
				case 0x22:
					reg.a = and_u8(reg.a, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//AND A, [IY + L]
				case 0x23:
					reg.a = and_u8(reg.a, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//AND [HL], A
				case 0x24:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, and_u8(temp_val, reg.a));
					system_cycles = 16;
					break;

				//AND [HL], #nn
				case 0x25:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, and_u8(temp_val, mem->read_u8(reg.pc_ex++)));
					system_cycles = 20;

					reg.pc++;
					break;

				//AND [HL], [IX]
				case 0x26:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, and_u8(temp_val, mem->read_u8(reg.ix_ex)));
					system_cycles = 20;
					break;

				//AND [HL], [IY]
				case 0x27:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, and_u8(temp_val, mem->read_u8(reg.iy_ex)));
					system_cycles = 20;
					break;

				//OR A, [IX + #ss]
				case 0x28:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = or_u8(reg.a, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//OR A, [IY + #ss]
				case 0x29:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = or_u8(reg.a, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//OR A, [IX + L]
				case 0x2A:
					reg.a = or_u8(reg.a, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//OR A, [IY + L]
				case 0x2B:
					reg.a = or_u8(reg.a, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//OR [HL], A
				case 0x2C:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, or_u8(temp_val, reg.a));
					system_cycles = 16;
					break;

				//OR [HL], #nn
				case 0x2D:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, or_u8(temp_val, mem->read_u8(reg.pc_ex++)));
					system_cycles = 20;

					reg.pc++;
					break;

				//OR [HL], [IX]
				case 0x2E:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, or_u8(temp_val, mem->read_u8(reg.ix_ex)));
					system_cycles = 20;
					break;

				//OR [HL], [IY]
				case 0x2F:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, or_u8(temp_val, mem->read_u8(reg.iy_ex)));
					system_cycles = 20;
					break;

				//CP A, [IX + #ss]
				case 0x30:
					s_temp = mem->read_s8(reg.pc_ex++);
					cp_u8(reg.a, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//CP A, [IY + #ss]
				case 0x31:
					s_temp = mem->read_s8(reg.pc_ex++);
					cp_u8(reg.a, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//CP A, [IX + L]
				case 0x32:
					cp_u8(reg.a, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//CP A, [IY + L]
				case 0x33:
					cp_u8(reg.a, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//CP [HL], A
				case 0x34:
					temp_val = mem->read_u8(reg.hl_ex);
					cp_u8(temp_val, reg.a);
					system_cycles = 16;
					break;

				//CP [HL], #nn
				case 0x35:
					temp_val = mem->read_u8(reg.hl_ex);
					cp_u8(temp_val, mem->read_u8(reg.pc_ex++));
					system_cycles = 20;

					reg.pc++;
					break;

				//CP [HL], [IX]
				case 0x36:
					temp_val = mem->read_u8(reg.hl_ex);
					cp_u8(temp_val, mem->read_u8(reg.ix_ex));
					system_cycles = 20;
					break;

				//CP [HL], [IY]
				case 0x37:
					temp_val = mem->read_u8(reg.hl_ex);
					cp_u8(temp_val, mem->read_u8(reg.iy_ex));
					system_cycles = 20;
					break;

				//XOR A, [IX + #ss]
				case 0x38:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = xor_u8(reg.a, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//XOR A, [IY + #ss]
				case 0x39:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = xor_u8(reg.a, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 16;

					reg.pc++;
					break;

				//XOR A, [IX + L]
				case 0x3A:
					reg.a = xor_u8(reg.a, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//XOR A, [IY + L]
				case 0x3B:
					reg.a = xor_u8(reg.a, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 16;
					break;

				//XOR [HL], A
				case 0x3C:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, xor_u8(temp_val, reg.a));
					system_cycles = 16;
					break;

				//XOR [HL], #nn
				case 0x3D:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, xor_u8(temp_val, mem->read_u8(reg.pc_ex++)));
					system_cycles = 20;

					reg.pc++;
					break;

				//XOR [HL], [IX]
				case 0x3E:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, xor_u8(temp_val, mem->read_u8(reg.ix_ex)));
					system_cycles = 20;
					break;

				//XOR [HL], [IY]
				case 0x3F:
					temp_val = mem->read_u8(reg.hl_ex);
					mem->write_u8(reg.hl_ex, xor_u8(temp_val, mem->read_u8(reg.iy_ex)));
					system_cycles = 20;
					break;

				//LD A, [IX + #ss]
				case 0x40:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = mem->read_u8(reg.ix_ex + s_temp);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD A, [IY + #ss]
				case 0x41:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.a = mem->read_u8(reg.iy_ex + s_temp);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD A, [IX + L]
				case 0x42:
					reg.a = mem->read_u8(reg.ix_ex + comp8(reg.l));
					system_cycles = 16;
					break;

				//LD A, [IY + L]
				case 0x43:
					reg.a = mem->read_u8(reg.iy_ex + comp8(reg.l));
					system_cycles = 16;
					break;

				//LD [IX + #ss], A
				case 0x44:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8((reg.ix_ex + s_temp), reg.a);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD [IY + #ss], A
				case 0x45:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8((reg.iy_ex + s_temp), reg.a);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD [IX + L], A
				case 0x46:
					mem->write_u8((reg.ix_ex + comp8(reg.l)), reg.a);
					system_cycles = 16;
					break;

				//LD [IY + L], A
				case 0x47:
					mem->write_u8((reg.iy_ex + comp8(reg.l)), reg.a);
					system_cycles = 16;
					break;

				//LD B, [IX + #ss]
				case 0x48:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.b = mem->read_u8(reg.ix_ex + s_temp);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD B, [IY + #ss]
				case 0x49:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.b = mem->read_u8(reg.iy_ex + s_temp);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD B, [IX + L]
				case 0x4A:
					reg.b = mem->read_u8(reg.ix_ex + comp8(reg.l));
					system_cycles = 16;
					break;

				//LD B, [IY + L]
				case 0x4B:
					reg.b = mem->read_u8(reg.iy_ex + comp8(reg.l));
					system_cycles = 16;
					break;

				//LD [IX + #ss], B
				case 0x4C:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8((reg.ix_ex + s_temp), reg.b);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD [IY + #ss], B
				case 0x4D:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8((reg.iy_ex + s_temp), reg.b);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD [IX + L], B
				case 0x4E:
					mem->write_u8((reg.ix_ex + comp8(reg.l)), reg.b);
					system_cycles = 16;
					break;

				//LD [IY + L], B
				case 0x4F:
					mem->write_u8((reg.iy_ex + comp8(reg.l)), reg.b);
					system_cycles = 16;
					break;

				//LD L, [IX + #ss]
				case 0x50:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.l = mem->read_u8(reg.ix_ex + s_temp);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD L, [IY + #ss]
				case 0x51:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.l = mem->read_u8(reg.iy_ex + s_temp);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD L, [IX + L]
				case 0x52:
					reg.l = mem->read_u8(reg.ix_ex + comp8(reg.l));
					system_cycles = 16;
					break;

				//LD L, [IY + L]
				case 0x53:
					reg.l = mem->read_u8(reg.iy_ex + comp8(reg.l));
					system_cycles = 16;
					break;

				//LD [IX + #ss], L
				case 0x54:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8((reg.ix_ex + s_temp), reg.l);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD [IY + #ss], L
				case 0x55:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8((reg.iy_ex + s_temp), reg.l);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD [IX + L], L
				case 0x56:
					mem->write_u8((reg.ix_ex + comp8(reg.l)), reg.l);
					system_cycles = 16;
					break;

				//LD [IY + L], L
				case 0x57:
					mem->write_u8((reg.iy_ex + comp8(reg.l)), reg.l);
					system_cycles = 16;
					break;

				//LD H, [IX + #ss]
				case 0x58:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.h = mem->read_u8(reg.ix_ex + s_temp);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD H, [IY + #ss]
				case 0x59:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.h = mem->read_u8(reg.iy_ex + s_temp);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD H, [IX + L]
				case 0x5A:
					reg.h = mem->read_u8(reg.ix_ex + comp8(reg.l));
					system_cycles = 16;
					break;

				//LD H, [IY + L]
				case 0x5B:
					reg.h = mem->read_u8(reg.iy_ex + comp8(reg.l));
					system_cycles = 16;
					break;

				//LD [IX + #ss], H
				case 0x5C:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8((reg.ix_ex + s_temp), reg.h);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD [IY + #ss], H
				case 0x5D:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8((reg.iy_ex + s_temp), reg.h);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD [IX + L], H
				case 0x5E:
					mem->write_u8((reg.ix_ex + comp8(reg.l)), reg.h);
					system_cycles = 16;
					break;

				//LD [IY + L], H
				case 0x5F:
					mem->write_u8((reg.iy_ex + comp8(reg.l)), reg.h);
					system_cycles = 16;
					break;

				//LD [HL], [IX + #ss]
				case 0x60:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8(reg.hl_ex, (reg.ix_ex + s_temp));
					system_cycles = 20;

					reg.pc++;
					break;

				//LD [HL], [IY + #ss]
				case 0x61:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8(reg.hl_ex, (reg.iy_ex + s_temp));
					system_cycles = 20;

					reg.pc++;
					break;

				//LD [HL], [IX + L]
				case 0x62:
					mem->write_u8(reg.hl_ex, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 20;
					break;

				//LD [HL], [IY + L]
				case 0x63:
					mem->write_u8(reg.hl_ex, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 20;
					break;

				//LD [IX], [IX + #ss]
				case 0x68:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8(reg.ix_ex, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 20;

					reg.pc++;
					break;

				//LD [IX], [IY + #ss]
				case 0x69:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8(reg.ix_ex, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 20;

					reg.pc++;
					break;

				//LD [IX], [IX + L]
				case 0x6A:
					mem->write_u8(reg.ix_ex, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 20;
					break;

				//LD [IX], [IY + L]
				case 0x6B:
					mem->write_u8(reg.ix_ex, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 20;
					break;

				//LD [IY], [IX + #ss]
				case 0x78:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8(reg.iy_ex, mem->read_u8(reg.ix_ex + s_temp));
					system_cycles = 20;

					reg.pc++;
					break;

				//LD [IY], [IY + #ss]
				case 0x79:
					s_temp = mem->read_s8(reg.pc_ex++);
					mem->write_u8(reg.iy_ex, mem->read_u8(reg.iy_ex + s_temp));
					system_cycles = 20;

					reg.pc++;
					break;

				//LD [IY], [IX + L]
				case 0x7A:
					mem->write_u8(reg.iy_ex, mem->read_u8(reg.ix_ex + comp8(reg.l)));
					system_cycles = 20;
					break;

				//LD [IY], [IY + L]
				case 0x7B:
					mem->write_u8(reg.iy_ex, mem->read_u8(reg.iy_ex + comp8(reg.l)));
					system_cycles = 20;
					break;

				//SLA A
				case 0x80:
					reg.a = sla_u8(reg.a);
					system_cycles = 12;
					break;

				//SLA B
				case 0x81:
					reg.b = sla_u8(reg.b);
					system_cycles = 12;
					break;

				//SLA [BR + #nn]
				case 0x82:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, sla_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//SLA [HL]
				case 0x83:
					mem->write_u8(reg.hl_ex, sla_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//SLL A
				case 0x84:
					reg.a = sll_u8(reg.a);
					system_cycles = 12;
					break;

				//SLL B
				case 0x85:
					reg.b = sll_u8(reg.b);
					system_cycles = 12;
					break;

				//SLL [BR + #nn]
				case 0x86:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, sll_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//SLL [HL]
				case 0x87:
					mem->write_u8(reg.hl_ex, sll_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//SRA A
				case 0x88:
					reg.a = sra_u8(reg.a);
					system_cycles = 12;
					break;

				//SRA B
				case 0x89:
					reg.b = sra_u8(reg.b);
					system_cycles = 12;
					break;

				//SRA [BR + #nn]
				case 0x8A:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, sra_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//SRA [HL]
				case 0x8B:
					mem->write_u8(reg.hl_ex, sra_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//SRL A
				case 0x8C:
					reg.a = srl_u8(reg.a);
					system_cycles = 12;
					break;

				//SRL B
				case 0x8D:
					reg.b = srl_u8(reg.b);
					system_cycles = 12;
					break;

				//SRL [BR + #nn]
				case 0x8E:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, srl_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//SRL [HL]
				case 0x8F:
					mem->write_u8(reg.hl_ex, srl_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//RL A
				case 0x90:
					reg.a = rl_u8(reg.a);
					system_cycles = 12;
					break;

				//RL B
				case 0x91:
					reg.b = rl_u8(reg.b);
					system_cycles = 12;
					break;

				//RL [BR + #nn]
				case 0x92:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, rl_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//RL [HL]
				case 0x93:
					mem->write_u8(reg.hl_ex, rl_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//RLC A
				case 0x94:
					reg.a = rlc_u8(reg.a);
					system_cycles = 12;
					break;

				//RLC B
				case 0x95:
					reg.b = rlc_u8(reg.b);
					system_cycles = 12;
					break;

				//RLC [BR + #nn]
				case 0x96:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, rlc_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//RLC [HL]
				case 0x97:
					mem->write_u8(reg.hl_ex, rlc_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//RR A
				case 0x98:
					reg.a = rr_u8(reg.a);
					system_cycles = 12;
					break;

				//RR B
				case 0x99:
					reg.b = rr_u8(reg.b);
					system_cycles = 12;
					break;

				//RR [BR + #nn]
				case 0x9A:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, rr_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//RR [HL]
				case 0x9B:
					mem->write_u8(reg.hl_ex, rr_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//RRC A
				case 0x9C:
					reg.a = rrc_u8(reg.a);
					system_cycles = 12;
					break;

				//RRC B
				case 0x9D:
					reg.b = rrc_u8(reg.b);
					system_cycles = 12;
					break;

				//RRC [BR + #nn]
				case 0x9E:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, rrc_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//RRC [HL]
				case 0x9F:
					mem->write_u8(reg.hl_ex, rrc_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//CPL A
				case 0xA0:
					reg.a = cpl_u8(reg.a);
					system_cycles = 12;
					break;

				//CPL B
				case 0xA1:
					reg.b = cpl_u8(reg.b);
					system_cycles = 12;
					break;

				//CPL [BR + #nn]
				case 0xA2:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, cpl_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//CPL [HL]
				case 0xA3:
					mem->write_u8(reg.hl_ex, cpl_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//NEG A
				case 0xA4:
					reg.a = neg_u8(reg.a);
					system_cycles = 12;
					break;

				//NEG B
				case 0xA5:
					reg.b = neg_u8(reg.b);
					system_cycles = 12;
					break;

				//NEG [BR + #nn]
				case 0xA6:
					temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
					mem->write_u8(temp_val, neg_u8(mem->read_u8(temp_val)));
					system_cycles = 20;

					reg.pc++;
					break;

				//NEG [HL]
				case 0xA7:
					mem->write_u8(reg.hl_ex, neg_u8(mem->read_u8(reg.hl_ex)));
					system_cycles = 16;
					break;

				//SEP
				case 0xA8:
					reg.b = (reg.a & 0x80) ? 0xFF : 0x00;
					system_cycles = 12;
					break;

				//HALT
				case 0xAE:
					halt = true;
					system_cycles = 12;
					break;

				//AND B, #nn
				case 0xB0:
					reg.b = and_u8(reg.b, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//AND L, #nn
				case 0xB1:
					reg.l = and_u8(reg.l, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//AND H, #nn
				case 0xB2:
					reg.h = and_u8(reg.h, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//OR B, #nn
				case 0xB4:
					reg.b = or_u8(reg.b, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//OR L, #nn
				case 0xB5:
					reg.l = or_u8(reg.l, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//OR H, #nn
				case 0xB6:
					reg.h = or_u8(reg.h, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//XOR B, #nn
				case 0xB8:
					reg.b = xor_u8(reg.b, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//XOR L, #nn
				case 0xB9:
					reg.l = xor_u8(reg.l, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//XOR H, #nn
				case 0xBA:
					reg.h = xor_u8(reg.h, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//CP B, #nn
				case 0xBC:
					cp_u8(reg.b, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//CP L, #nn
				case 0xBD:
					cp_u8(reg.l, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//CP H, #nn
				case 0xBE:
					cp_u8(reg.h, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//CP BR, #nn
				case 0xBF:
					cp_u8(reg.br, mem->read_u8(reg.pc_ex++));
					system_cycles = 12;

					reg.pc++;
					break;

				//LD A, BR
				case 0xC0:
					reg.a = reg.br;
					system_cycles = 8;
					break;

				//LD A, SC
				case 0xC1:
					reg.a = reg.sc;
					system_cycles = 8;
					break;

				//LD BR, A
				case 0xC2:
					reg.br = reg.a;
					system_cycles = 8;
					break;

				//LD SC, A
				case 0xC3:
					reg.sc = reg.a;
					system_cycles = 12;
					skip_irq = true;
					break;

				//LD NB, #nn
				case 0xC4:
					reg.nb = mem->read_u8(reg.pc_ex++);
					system_cycles = 16;
					skip_irq = true;

					reg.pc++;
					break;

				//LD EP, #nn
				case 0xC5:
					reg.ep = mem->read_u8(reg.pc_ex++);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD XP, #nn
				case 0xC6:
					reg.xp = mem->read_u8(reg.pc_ex++);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD YP, #nn
				case 0xC7:
					reg.yp = mem->read_u8(reg.pc_ex++);
					system_cycles = 16;

					reg.pc++;
					break;

				//LD A, NB
				case 0xC8:
					reg.a = reg.nb;
					system_cycles = 8;
					break;

				//LD A, EP
				case 0xC9:
					reg.a = reg.ep;
					system_cycles = 8;
					break;

				//LD A, XP
				case 0xCA:
					reg.a = reg.xp;
					system_cycles = 8;
					break;

				//LD A, YP
				case 0xCB:
					reg.a = reg.yp;
					system_cycles = 8;
					break;

				//LD NB, A
				case 0xCC:
					reg.nb = reg.a;
					system_cycles = 12;
					skip_irq = true;
					break;

				//LD EP, A
				case 0xCD:
					reg.ep = reg.a;
					system_cycles = 8;
					break;

				//LD XP, A
				case 0xCE:
					reg.xp = reg.a;
					system_cycles = 8;
					break;

				//LD YP, A
				case 0xCF:
					reg.yp = reg.a;
					system_cycles = 8;
					break;

				//LD A, [#nnnn]
				case 0xD0:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					reg.a = mem->read_u8(temp_val);
					system_cycles = 20;

					reg.pc += 2;
					break;

				//LD B, [#nnnn]
				case 0xD1:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					reg.b = mem->read_u8(temp_val);
					system_cycles = 20;

					reg.pc += 2;
					break;

				//LD L, [#nnnn]
				case 0xD2:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					reg.l = mem->read_u8(temp_val);
					system_cycles = 20;

					reg.pc += 2;
					break;

				//LD H, [#nnnn]
				case 0xD3:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					reg.h = mem->read_u8(temp_val);
					system_cycles = 20;

					reg.pc += 2;
					break;

				//LD [#nnnn], A
				case 0xD4:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					mem->write_u8(temp_val, reg.a);
					system_cycles = 20;

					reg.pc += 2;
					break;

				//LD [#nnnn], B
				case 0xD5:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					mem->write_u8(temp_val, reg.b);
					system_cycles = 20;

					reg.pc += 2;
					break;

				//LD [#nnnn], L
				case 0xD6:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					mem->write_u8(temp_val, reg.l);
					system_cycles = 20;

					reg.pc += 2;
					break;

				//LD [#nnnn], H
				case 0xD7:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					mem->write_u8(temp_val, reg.h);
					system_cycles = 20;

					reg.pc += 2;
					break;

				//MLT L, A
				case 0xD8:
					reg.hl = mlt_u8(reg.l, reg.a);
					system_cycles = 48;
					break;

				//DIV HL, A
				case 0xD9:
					reg.hl = div_u16(reg.hl, reg.a);
					system_cycles = 52;
					break;

				//JRS LT #ss
				case 0xE0:
					{
						s_temp = mem->read_s8(reg.pc_ex++);
						reg.pc++;

						u8 flag_1 = (reg.sc & OVERFLOW_FLAG) ? 1 : 0;
						u8 flag_2 = (reg.sc & NEGATIVE_FLAG) ? 1 : 0;

						if(flag_1 != flag_2)
						{
							reg.cb = reg.nb;
							reg.pc = log_addr + s_temp + 2;
						}

						system_cycles = 12;
					}

					break;

				//JRS LE #ss
				case 0xE1:
					{
						s_temp = mem->read_s8(reg.pc_ex++);
						reg.pc++;

						u8 flag_1 = (reg.sc & OVERFLOW_FLAG) ? 1 : 0;
						u8 flag_2 = (reg.sc & NEGATIVE_FLAG) ? 1 : 0;
						u8 flag_3 = (reg.sc & ZERO_FLAG) ? 1 : 0;

						if((flag_1 != flag_2) || (flag_3))
						{
							reg.cb = reg.nb;
							reg.pc = log_addr + s_temp + 2;
						}

						system_cycles = 12;
					}

					break;

				//JRS GT #ss
				case 0xE2:
					{
						s_temp = mem->read_s8(reg.pc_ex++);
						reg.pc++;

						u8 flag_1 = (reg.sc & OVERFLOW_FLAG) ? 1 : 0;
						u8 flag_2 = (reg.sc & NEGATIVE_FLAG) ? 1 : 0;
						u8 flag_3 = (reg.sc & ZERO_FLAG) ? 1 : 0;

						if((flag_1 == flag_2) && (flag_3 == 0))
						{
							reg.cb = reg.nb;
							reg.pc = log_addr + s_temp + 2;
						}

						system_cycles = 12;
					}

					break;

				//JRS GE #ss
				case 0xE3:
					{
						s_temp = mem->read_s8(reg.pc_ex++);
						reg.pc++;

						u8 flag_1 = (reg.sc & OVERFLOW_FLAG) ? 1 : 0;
						u8 flag_2 = (reg.sc & NEGATIVE_FLAG) ? 1 : 0;

						if(flag_1 == flag_2)
						{
							reg.cb = reg.nb;
							reg.pc = log_addr + s_temp + 2;
						}

						system_cycles = 12;
					}

					break;

				//JRS V #ss
				case 0xE4:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.pc++;

					if(reg.sc & OVERFLOW_FLAG)
					{
						reg.cb = reg.nb;
						reg.pc = log_addr + s_temp + 2;
					}

					system_cycles = 12;

					break;

				//JRS NV #ss
				case 0xE5:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.pc++;

					if((reg.sc & OVERFLOW_FLAG) == 0)
					{
						reg.cb = reg.nb;
						reg.pc = log_addr + s_temp + 2;
					}

					system_cycles = 12;

					break;

				//JRS P #ss
				case 0xE6:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.pc++;

					if((reg.sc & NEGATIVE_FLAG) == 0)
					{
						reg.cb = reg.nb;
						reg.pc = log_addr + s_temp + 2;
					}

					system_cycles = 12;

					break;

				//JRS M #ss
				case 0xE7:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.pc++;

					if(reg.sc & NEGATIVE_FLAG)
					{
						reg.cb = reg.nb;
						reg.pc = log_addr + s_temp + 2;
					}

					system_cycles = 12;

					break;

				//CARS LT #ss
				case 0xF0:
					{
						s_temp = mem->read_s8(reg.pc_ex++);
						reg.pc++;

						u8 flag_1 = (reg.sc & OVERFLOW_FLAG) ? 1 : 0;
						u8 flag_2 = (reg.sc & NEGATIVE_FLAG) ? 1 : 0;

						if(flag_1 != flag_2)
						{
							mem->write_u8(--reg.sp, (reg.cb));
							mem->write_u8(--reg.sp, (reg.pc >> 8));
							mem->write_u8(--reg.sp, (reg.pc & 0xFF));

							reg.cb = reg.nb;
							reg.pc = log_addr + s_temp + 2;

							system_cycles = 24;
						}

						else { system_cycles = 12; }
					}

					break;

				//CARS LE #ss
				case 0xF1:
					{
						s_temp = mem->read_s8(reg.pc_ex++);
						reg.pc++;

						u8 flag_1 = (reg.sc & OVERFLOW_FLAG) ? 1 : 0;
						u8 flag_2 = (reg.sc & NEGATIVE_FLAG) ? 1 : 0;
						u8 flag_3 = (reg.sc & ZERO_FLAG) ? 1 : 0;

						if((flag_1 != flag_2) || (flag_3))
						{
							mem->write_u8(--reg.sp, (reg.cb));
							mem->write_u8(--reg.sp, (reg.pc >> 8));
							mem->write_u8(--reg.sp, (reg.pc & 0xFF));

							reg.cb = reg.nb;
							reg.pc = log_addr + s_temp + 2;

							system_cycles = 24;
						}

						else { system_cycles = 12; }
					}

					break;

				//CARS GT #ss
				case 0xF2:
					{
						s_temp = mem->read_s8(reg.pc_ex++);
						reg.pc++;

						u8 flag_1 = (reg.sc & OVERFLOW_FLAG) ? 1 : 0;
						u8 flag_2 = (reg.sc & NEGATIVE_FLAG) ? 1 : 0;
						u8 flag_3 = (reg.sc & ZERO_FLAG) ? 1 : 0;

						if((flag_1 == flag_2) && (flag_3 == 0))
						{
							mem->write_u8(--reg.sp, (reg.cb));
							mem->write_u8(--reg.sp, (reg.pc >> 8));
							mem->write_u8(--reg.sp, (reg.pc & 0xFF));

							reg.cb = reg.nb;
							reg.pc = log_addr + s_temp + 2;

							system_cycles = 24;
						}

						else { system_cycles = 12; }
					}

					break;

				//CARS GE #ss
				case 0xF3:
					{
						s_temp = mem->read_s8(reg.pc_ex++);
						reg.pc++;

						u8 flag_1 = (reg.sc & OVERFLOW_FLAG) ? 1 : 0;
						u8 flag_2 = (reg.sc & NEGATIVE_FLAG) ? 1 : 0;

						if(flag_1 == flag_2)
						{
							mem->write_u8(--reg.sp, (reg.cb));
							mem->write_u8(--reg.sp, (reg.pc >> 8));
							mem->write_u8(--reg.sp, (reg.pc & 0xFF));

							reg.cb = reg.nb;
							reg.pc = log_addr + s_temp + 2;

							system_cycles = 24;
						}

						else { system_cycles = 12; }
					}

					break;

				//CARS V #ss
				case 0xF4:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.pc++;

					if(reg.sc & OVERFLOW_FLAG)
					{
						mem->write_u8(--reg.sp, (reg.cb));
						mem->write_u8(--reg.sp, (reg.pc >> 8));
						mem->write_u8(--reg.sp, (reg.pc & 0xFF));

						reg.cb = reg.nb;
						reg.pc = log_addr + s_temp + 2;

						system_cycles = 24;
					}

					else { system_cycles = 12; }

					break;

				//CARS NV #ss
				case 0xF5:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.pc++;

					if((reg.sc & OVERFLOW_FLAG) == 0)
					{
						mem->write_u8(--reg.sp, (reg.cb));
						mem->write_u8(--reg.sp, (reg.pc >> 8));
						mem->write_u8(--reg.sp, (reg.pc & 0xFF));

						reg.cb = reg.nb;
						reg.pc = log_addr + s_temp + 2;

						system_cycles = 24;
					}

					else { system_cycles = 12; }

					break;

				//CARS P #ss
				case 0xF6:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.pc++;

					if((reg.sc & NEGATIVE_FLAG) == 0)
					{
						mem->write_u8(--reg.sp, (reg.cb));
						mem->write_u8(--reg.sp, (reg.pc >> 8));
						mem->write_u8(--reg.sp, (reg.pc & 0xFF));

						reg.cb = reg.nb;
						reg.pc = log_addr + s_temp + 2;

						system_cycles = 24;
					}

					else { system_cycles = 12; }

					break;

				//CARS M #ss
				case 0xF7:
					s_temp = mem->read_s8(reg.pc_ex++);
					reg.pc++;

					if(reg.sc & NEGATIVE_FLAG)
					{
						mem->write_u8(--reg.sp, (reg.cb));
						mem->write_u8(--reg.sp, (reg.pc >> 8));
						mem->write_u8(--reg.sp, (reg.pc & 0xFF));

						reg.cb = reg.nb;
						reg.pc = log_addr + s_temp + 2;

						system_cycles = 24;
					}

					else { system_cycles = 12; }

					break;

				default:
					std::cout<<"CPU::Error - Unknown 0xCE opcode -> 0x" << std::hex << opcode << " @ 0x" << reg.pc << "\n";
					if(!config::ignore_illegal_opcodes) { running = false; }
			}

			break;

		//Extend 0xCF instructions
		case 0xCF:
			opcode = (opcode << 8) | mem->read_u8(reg.pc_ex++);
			reg.pc++;

			switch(opcode & 0xFF)
			{

				//ADD BA, BA
				case 0x00:
					reg.ba = add_u16(reg.ba, reg.ba);
					system_cycles = 16;
					break;

				//ADD BA, HL
				case 0x01:
					reg.ba = add_u16(reg.ba, reg.hl);
					system_cycles = 16;
					break;

				//ADD BA, IX
				case 0x02:
					reg.ba = add_u16(reg.ba, reg.ix);
					system_cycles = 16;
					break;

				//ADD BA, IY
				case 0x03:
					reg.ba = add_u16(reg.ba, reg.iy);
					system_cycles = 16;
					break;

				//ADC BA, BA
				case 0x04:
					reg.ba = adc_u16(reg.ba, reg.ba);
					system_cycles = 16;
					break;

				//ADC BA, HL
				case 0x05:
					reg.ba = adc_u16(reg.ba, reg.hl);
					system_cycles = 16;
					break;

				//ADC BA, IX
				case 0x06:
					reg.ba = adc_u16(reg.ba, reg.ix);
					system_cycles = 16;
					break;

				//ADC BA, IY
				case 0x07:
					reg.ba = adc_u16(reg.ba, reg.iy);
					system_cycles = 16;
					break;

				//SUB BA, BA
				case 0x08:
					reg.ba = sub_u16(reg.ba, reg.ba);
					system_cycles = 16;
					break;

				//SUB BA, HL
				case 0x09:
					reg.ba = sub_u16(reg.ba, reg.hl);
					system_cycles = 16;
					break;

				//SUB BA, IX
				case 0x0A:
					reg.ba = sub_u16(reg.ba, reg.ix);
					system_cycles = 16;
					break;

				//SUB BA, IY
				case 0x0B:
					reg.ba = sub_u16(reg.ba, reg.iy);
					system_cycles = 16;
					break;

				//SBC BA, BA
				case 0x0C:
					reg.ba = sbc_u16(reg.ba, reg.ba);
					system_cycles = 16;
					break;

				//SBC BA, HL
				case 0x0D:
					reg.ba = sbc_u16(reg.ba, reg.hl);
					system_cycles = 16;
					break;

				//SBC BA, IX
				case 0x0E:
					reg.ba = sbc_u16(reg.ba, reg.ix);
					system_cycles = 16;
					break;

				//SBC BA, IY
				case 0x0F:
					reg.ba = sbc_u16(reg.ba, reg.iy);
					system_cycles = 16;
					break;

				//CP BA, BA
				case 0x18:
					cp_u16(reg.ba, reg.ba);
					system_cycles = 16;
					break;

				//CP BA, HL
				case 0x19:
					cp_u16(reg.ba, reg.hl);
					system_cycles = 16;
					break;

				//CP BA, IX
				case 0x1A:
					cp_u16(reg.ba, reg.ix);
					system_cycles = 16;
					break;

				//CP BA, IY
				case 0x1B:
					cp_u16(reg.ba, reg.iy);
					system_cycles = 16;
					break;

				//ADD HL, BA
				case 0x20:
					reg.hl = add_u16(reg.hl, reg.ba);
					system_cycles = 16;
					break;

				//ADD HL, HL
				case 0x21:
					reg.hl = add_u16(reg.hl, reg.hl);
					system_cycles = 16;
					break;

				//ADD HL, IX
				case 0x22:
					reg.hl = add_u16(reg.hl, reg.ix);
					system_cycles = 16;
					break;

				//ADD HL, IY
				case 0x23:
					reg.hl = add_u16(reg.hl, reg.iy);
					system_cycles = 16;
					break;

				//ADC HL, BA
				case 0x24:
					reg.hl = adc_u16(reg.hl, reg.ba);
					system_cycles = 16;
					break;

				//ADC HL, HL
				case 0x25:
					reg.hl = adc_u16(reg.hl, reg.hl);
					system_cycles = 16;
					break;

				//ADC HL, IX
				case 0x26:
					reg.hl = adc_u16(reg.hl, reg.ix);
					system_cycles = 16;
					break;

				//ADC HL, IY
				case 0x27:
					reg.hl = adc_u16(reg.hl, reg.iy);
					system_cycles = 16;
					break;

				//SUB HL, BA
				case 0x28:
					reg.hl = sub_u16(reg.hl, reg.ba);
					system_cycles = 16;
					break;

				//SUB HL, HL
				case 0x29:
					reg.hl = sub_u16(reg.hl, reg.hl);
					system_cycles = 16;
					break;

				//SUB HL, IX
				case 0x2A:
					reg.hl = sub_u16(reg.hl, reg.ix);
					system_cycles = 16;
					break;

				//SUB HL, IY
				case 0x2B:
					reg.hl = sub_u16(reg.hl, reg.iy);
					system_cycles = 16;
					break;

				//SBC HL, BA
				case 0x2C:
					reg.hl = sbc_u16(reg.hl, reg.ba);
					system_cycles = 16;
					break;

				//SBC HL, HL
				case 0x2D:
					reg.hl = sbc_u16(reg.hl, reg.hl);
					system_cycles = 16;
					break;

				//SBC HL, IX
				case 0x2E:
					reg.hl = sbc_u16(reg.hl, reg.ix);
					system_cycles = 16;
					break;

				//SBC HL, IY
				case 0x2F:
					reg.hl = sbc_u16(reg.hl, reg.iy);
					system_cycles = 16;
					break;

				//CP HL, BA
				case 0x38:
					cp_u16(reg.hl, reg.ba);
					system_cycles = 16;
					break;

				//CP HL, HL
				case 0x39:
					cp_u16(reg.hl, reg.hl);
					system_cycles = 16;
					break;

				//CP HL, IX
				case 0x3A:
					cp_u16(reg.hl, reg.ix);
					system_cycles = 16;
					break;

				//CP HL, IY
				case 0x3B:
					cp_u16(reg.hl, reg.iy);
					system_cycles = 16;
					break;

				//ADD IX, BA
				case 0x40:
					reg.ix = add_u16(reg.ix, reg.ba);
					system_cycles = 16;
					break;

				//ADD IX, HL
				case 0x41:
					reg.ix = add_u16(reg.ix, reg.hl);
					system_cycles = 16;
					break;

				//ADD IY, BA
				case 0x42:
					reg.iy = add_u16(reg.iy, reg.ba);
					system_cycles = 16;
					break;

				//ADD IY, HL
				case 0x43:
					reg.iy = add_u16(reg.iy, reg.hl);
					system_cycles = 16;
					break;

				//ADD SP, BA
				case 0x44:
					reg.sp = add_u16(reg.sp, reg.ba);
					system_cycles = 16;
					break;

				//ADD SP, HL
				case 0x45:
					reg.sp = add_u16(reg.sp, reg.hl);
					system_cycles = 16;
					break;

				//SUB IX, BA
				case 0x48:
					reg.ix = sub_u16(reg.ix, reg.ba);
					system_cycles = 16;
					break;

				//SUB IX, HL
				case 0x49:
					reg.ix = sub_u16(reg.ix, reg.hl);
					system_cycles = 16;
					break;

				//SUB IY, BA
				case 0x4A:
					reg.iy = sub_u16(reg.iy, reg.ba);
					system_cycles = 16;
					break;

				//SUB IY, HL
				case 0x4B:
					reg.iy = sub_u16(reg.iy, reg.hl);
					system_cycles = 16;
					break;

				//SUB SP, BA
				case 0x4C:
					reg.sp = sub_u16(reg.sp, reg.ba);
					system_cycles = 16;
					break;

				//SUB SP, HL
				case 0x4D:
					reg.sp = sub_u16(reg.sp, reg.hl);
					system_cycles = 16;
					break;

				//CP SP, BA
				case 0x5C:
					cp_u16(reg.sp, reg.ba);
					system_cycles = 16;
					break;

				//CP SP, HL
				case 0x5D:
					cp_u16(reg.sp, reg.hl);
					system_cycles = 16;
					break;

				//ADC BA, #nnnn
				case 0x60:
					temp_val = mem->read_u16(reg.pc_ex);
					reg.ba = adc_u16(reg.ba, temp_val);
					system_cycles = 12;

					reg.pc += 2;
					break;

				//ADC HL, #nnnn
				case 0x61:
					temp_val = mem->read_u16(reg.pc_ex);
					reg.hl = adc_u16(reg.hl, temp_val);
					system_cycles = 12;

					reg.pc += 2;
					break;

				//SBC BA, #nnnn
				case 0x62:
					temp_val = mem->read_u16(reg.pc_ex);
					reg.ba = sbc_u16(reg.ba, temp_val);
					system_cycles = 12;

					reg.pc += 2;
					break;

				//SBC HL, #nnnn
				case 0x63:
					temp_val = mem->read_u16(reg.pc_ex);
					reg.hl = sbc_u16(reg.hl, temp_val);
					system_cycles = 12;

					reg.pc += 2;
					break;

				//ADD SP, #nnnn
				case 0x68:
					temp_val = mem->read_u16(reg.pc_ex);
					reg.sp = add_u16(reg.sp, temp_val);
					system_cycles = 12;

					reg.pc += 2;
					break;

				//SUB SP, #nnnn
				case 0x6A:
					temp_val = mem->read_u16(reg.pc_ex);
					reg.sp = sub_u16(reg.sp, temp_val);
					system_cycles = 12;

					reg.pc += 2;
					break;

				//CP SP, #nnnn
				case 0x6C:
					temp_val = mem->read_u16(reg.pc_ex);
					cp_u16(reg.sp, temp_val);
					system_cycles = 12;

					reg.pc += 2;
					break;

				//LD SP, #nnnn
				case 0x6E:
					reg.sp = mem->read_u16(reg.pc_ex);
					system_cycles = 16;

					reg.pc += 2;
					break;

				//LD BA, [SP + #ss]
				case 0x70:
					s_temp = mem->read_s8(reg.pc_ex++);
					temp_val = reg.sp + s_temp;
					reg.ba = mem->read_u16(temp_val);
					system_cycles = 24;

					reg.pc++;
					break;

				//LD HL, [SP + #ss]
				case 0x71:
					s_temp = mem->read_s8(reg.pc_ex++);
					temp_val = reg.sp + s_temp;
					reg.hl = mem->read_u16(temp_val);
					system_cycles = 24;

					reg.pc++;
					break;

				//LD IX, [SP + #ss]
				case 0x72:
					s_temp = mem->read_s8(reg.pc_ex++);
					temp_val = reg.sp + s_temp;
					reg.ix = mem->read_u16(temp_val);
					system_cycles = 24;

					reg.pc++;
					break;

				//LD IY, [SP + #ss]
				case 0x73:
					s_temp = mem->read_s8(reg.pc_ex++);
					temp_val = reg.sp + s_temp;
					reg.iy = mem->read_u16(temp_val);
					system_cycles = 24;

					reg.pc++;
					break;

				//LD [SP + #ss], BA
				case 0x74:
					s_temp = mem->read_s8(reg.pc_ex++);
					temp_val = reg.sp + s_temp;
					mem->write_u16(temp_val, reg.ba);
					system_cycles = 24;

					reg.pc++;
					break;

				//LD [SP + #ss], HL
				case 0x75:
					s_temp = mem->read_s8(reg.pc_ex++);
					temp_val = reg.sp + s_temp;
					mem->write_u16(temp_val, reg.hl);
					system_cycles = 24;

					reg.pc++;
					break;

				//LD [SP + #ss], IX
				case 0x76:
					s_temp = mem->read_s8(reg.pc_ex++);
					temp_val = reg.sp + s_temp;
					mem->write_u16(temp_val, reg.ix);
					system_cycles = 24;

					reg.pc++;
					break;

				//LD [SP + #ss], IY
				case 0x77:
					s_temp = mem->read_s8(reg.pc_ex++);
					temp_val = reg.sp + s_temp;
					mem->write_u16(temp_val, reg.iy);
					system_cycles = 24;

					reg.pc++;
					break;
					
				//LD SP, [#nnnn]
				case 0x78:
					temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
					reg.sp = mem->read_u16(temp_val);
					system_cycles = 24;

					reg.pc += 2;
					break;

				//PUSH A
				case 0xB0:
					mem->write_u8(--reg.sp, reg.a);
					system_cycles = 12;
					break;

				//PUSH B
				case 0xB1:
					mem->write_u8(--reg.sp, reg.b);
					system_cycles = 12;
					break;

				//PUSH L
				case 0xB2:
					mem->write_u8(--reg.sp, reg.l);
					system_cycles = 12;
					break;

				//PUSH H
				case 0xB3:
					mem->write_u8(--reg.sp, reg.h);
					system_cycles = 12;
					break;

				//POP A
				case 0xB4:
					reg.a = mem->read_u8(reg.sp++);
					system_cycles = 12;
					break;

				//POP B
				case 0xB5:
					reg.b = mem->read_u8(reg.sp++);
					system_cycles = 12;
					break;

				//POP L
				case 0xB6:
					reg.l = mem->read_u8(reg.sp++);
					system_cycles = 12;
					break;

				//POP H
				case 0xB7:
					reg.h = mem->read_u8(reg.sp++);
					system_cycles = 12;
					break;

				//PUSH ALL
				case 0xB8:
					mem->write_u8(--reg.sp, reg.b);
					mem->write_u8(--reg.sp, reg.a);
					mem->write_u8(--reg.sp, reg.h);
					mem->write_u8(--reg.sp, reg.l);
					mem->write_u8(--reg.sp, (reg.ix >> 8));
					mem->write_u8(--reg.sp, (reg.ix & 0xFF));
					mem->write_u8(--reg.sp, (reg.iy >> 8));
					mem->write_u8(--reg.sp, (reg.iy & 0xFF));
					mem->write_u8(--reg.sp, reg.br);
					system_cycles = 48;
					break;

				//PUSH ALE
				case 0xB9:
					mem->write_u8(--reg.sp, reg.b);
					mem->write_u8(--reg.sp, reg.a);
					mem->write_u8(--reg.sp, reg.h);
					mem->write_u8(--reg.sp, reg.l);
					mem->write_u8(--reg.sp, (reg.ix >> 8));
					mem->write_u8(--reg.sp, (reg.ix & 0xFF));
					mem->write_u8(--reg.sp, (reg.iy >> 8));
					mem->write_u8(--reg.sp, (reg.iy & 0xFF));
					mem->write_u8(--reg.sp, reg.br);
					mem->write_u8(--reg.sp, reg.ep);
					mem->write_u8(--reg.sp, reg.xp);
					mem->write_u8(--reg.sp, reg.yp);
					system_cycles = 60;
					break;

				//POP ALL
				case 0xBC:
					reg.br = mem->read_u8(reg.sp++);
					reg.iy = mem->read_u8(reg.sp++);
					reg.iy |= (mem->read_u8(reg.sp++) << 8);
					reg.ix = mem->read_u8(reg.sp++);
					reg.ix |= (mem->read_u8(reg.sp++) << 8);
					reg.l = mem->read_u8(reg.sp++);
					reg.h = mem->read_u8(reg.sp++);
					reg.a = mem->read_u8(reg.sp++);
					reg.b = mem->read_u8(reg.sp++);
					system_cycles = 32;
					break;

				//POP ALE
				case 0xBD:
					reg.yp = mem->read_u8(reg.sp++);
					reg.xp = mem->read_u8(reg.sp++);
					reg.ep = mem->read_u8(reg.sp++);
					reg.br = mem->read_u8(reg.sp++);
					reg.iy = mem->read_u8(reg.sp++);
					reg.iy |= (mem->read_u8(reg.sp++) << 8);
					reg.ix = mem->read_u8(reg.sp++);
					reg.ix |= (mem->read_u8(reg.sp++) << 8);
					reg.l = mem->read_u8(reg.sp++);
					reg.h = mem->read_u8(reg.sp++);
					reg.a = mem->read_u8(reg.sp++);
					reg.b = mem->read_u8(reg.sp++);
					system_cycles = 40;
					break;

				//LD BA, [HL]
				case 0xC0:
					reg.ba = mem->read_u16(reg.hl_ex);
					system_cycles = 20;
					break;

				//LD HL, [HL]
				case 0xC1:
					reg.hl = mem->read_u16(reg.hl_ex);
					system_cycles = 20;
					break;

				//LD IX, [HL]
				case 0xC2:
					reg.ix = mem->read_u16(reg.hl_ex);
					system_cycles = 20;
					break;

				//LD IY, [HL]
				case 0xC3:
					reg.iy = mem->read_u16(reg.hl_ex);
					system_cycles = 20;
					break;

				//LD [HL], BA
				case 0xC4:
					mem->write_u16(reg.hl_ex, reg.ba);
					system_cycles = 20;
					break;

				//LD [HL], HL
				case 0xC5:
					mem->write_u16(reg.hl_ex, reg.hl);
					system_cycles = 20;
					break;

				//LD [HL], IX
				case 0xC6:
					mem->write_u16(reg.hl_ex, reg.ix);
					system_cycles = 20;
					break;

				//LD [HL], IY
				case 0xC7:
					mem->write_u16(reg.hl_ex, reg.iy);
					system_cycles = 20;
					break;
				
				//LD BA, [IX]
				case 0xD0:
					reg.ba = mem->read_u16(reg.ix_ex);
					system_cycles = 20;
					break;

				//LD HL, [IX]
				case 0xD1:
					reg.hl = mem->read_u16(reg.ix_ex);
					system_cycles = 20;
					break;

				//LD IX, [IX]
				case 0xD2:
					reg.ix = mem->read_u16(reg.ix_ex);
					system_cycles = 20;
					break;

				//LD IY, [IX]
				case 0xD3:
					reg.iy = mem->read_u16(reg.ix_ex);
					system_cycles = 20;
					break;

				//LD IX, [BA]
				case 0xD4:
					mem->write_u16(reg.ix_ex, reg.ba);
					system_cycles = 20;
					break;

				//LD [IX], HL
				case 0xD5:
					mem->write_u16(reg.ix_ex, reg.hl);
					system_cycles = 20;
					break;

				//LD [IX], IX
				case 0xD6:
					mem->write_u16(reg.ix_ex, reg.ix);
					system_cycles = 20;
					break;

				//LD [IX], IY
				case 0xD7:
					mem->write_u16(reg.ix_ex, reg.iy);
					system_cycles = 20;
					break;

				//LD BA, [IY]
				case 0xD8:
					reg.ba = mem->read_u16(reg.iy_ex);
					system_cycles = 20;
					break;

				//LD HL, [IY]
				case 0xD9:
					reg.hl = mem->read_u16(reg.iy_ex);
					system_cycles = 20;
					break;

				//LD IX, [IY]
				case 0xDA:
					reg.ix = mem->read_u16(reg.iy_ex);
					system_cycles = 20;
					break;

				//LD IY, [IY]
				case 0xDB:
					reg.iy = mem->read_u16(reg.iy_ex);
					system_cycles = 20;
					break;

				//LD [IY], BA
				case 0xDC:
					mem->write_u16(reg.iy_ex, reg.ba);
					system_cycles = 20;
					break;

				//LD [IY], HL
				case 0xDD:
					mem->write_u16(reg.iy_ex, reg.hl);
					system_cycles = 20;
					break;

				//LD [IY], IX
				case 0xDE:
					mem->write_u16(reg.iy_ex, reg.ix);
					system_cycles = 20;
					break;

				//LD [IY], IY
				case 0xDF:
					mem->write_u16(reg.iy_ex, reg.iy);
					system_cycles = 20;
					break;

				//LD BA, BA
				case 0xE0:
					system_cycles = 8;
					break;

				//LD BA, HL
				case 0xE1:
					reg.ba = reg.hl;
					system_cycles = 8;
					break;

				//LD BA, IX
				case 0xE2:
					reg.ba = reg.ix;
					system_cycles = 8;
					break;

				//LD BA, IY
				case 0xE3:
					reg.ba = reg.iy;
					system_cycles = 8;
					break;

				//LD HL, BA
				case 0xE4:
					reg.hl = reg.ba;
					system_cycles = 8;
					break;

				//LD HL, HL
				case 0xE5:
					system_cycles = 8;
					break;

				//LD HL, IX
				case 0xE6:
					reg.hl = reg.ix;
					system_cycles = 8;
					break;

				//LD HL, IY
				case 0xE7:
					reg.hl = reg.iy;
					system_cycles = 8;
					break;

				//LD IX, BA
				case 0xE8:
					reg.ix = reg.ba;
					system_cycles = 8;
					break;

				//LD IX, HL
				case 0xE9:
					reg.ix = reg.hl;
					system_cycles = 8;
					break;

				//LD IX, IX
				case 0xEA:
					system_cycles = 8;
					break;

				//LD IX, IY
				case 0xEB:
					reg.ix = reg.iy;
					system_cycles = 8;
					break;

				//LD IY, BA
				case 0xEC:
					reg.iy = reg.ba;
					system_cycles = 8;
					break;

				//LD IY, HL
				case 0xED:
					reg.iy = reg.hl;
					system_cycles = 8;
					break;

				//LD IY, IX
				case 0xEE:
					reg.iy = reg.ix;
					system_cycles = 8;
					break;

				//LD IY, IY
				case 0xEF:
					system_cycles = 8;
					break;

				//LD SP, BA
				case 0xF0:
					reg.sp = reg.ba;
					system_cycles = 8;
					break;

				//LD SP, HL
				case 0xF1:
					reg.sp = reg.hl;
					system_cycles = 8;
					break;

				//LD SP, IX
				case 0xF2:
					reg.sp = reg.ix;
					system_cycles = 8;
					break;

				//LD SP, IY
				case 0xF3:
					reg.sp = reg.iy;
					system_cycles = 8;
					break;

				//LD HL, SP
				case 0xF4:
					reg.hl = reg.sp;
					system_cycles = 8;
					break;

				//LD HL, PC
				case 0xF5:
					reg.hl = reg.pc;
					system_cycles = 8;
					break;

				//LD BA, SP
				case 0xF8:
					reg.ba = reg.sp;
					system_cycles = 8;
					break;

				//LD BA, PC
				case 0xF9:
					reg.ba = reg.pc;
					system_cycles = 8;
					break;

				//LD IX, SP
				case 0xFA:
					reg.ix = reg.sp;
					system_cycles = 8;
					break;

				//LD IY, SP
				case 0xFE:
					reg.iy = reg.sp;
					system_cycles = 8;
					break;

				default:
				std::cout<<"CPU::Error - Unknown 0xCF opcode -> 0x" << std::hex << opcode << "\n";
				if(!config::ignore_illegal_opcodes) { running = false; }
			}

			break;

		//SUB BA, #nnnn
		case 0xD0:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.ba = sub_u16(reg.ba, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//SUB HL, #nnnn
		case 0xD1:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.hl = sub_u16(reg.hl, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//SUB IX, #nnnn
		case 0xD2:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.ix = sub_u16(reg.ix, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//SUB IY, #nnnn
		case 0xD3:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.iy = sub_u16(reg.iy, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//CP BA, #nnnn
		case 0xD4:
			temp_val = mem->read_u16(reg.pc_ex);
			cp_u16(reg.ba, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//CP HL, #nnnn
		case 0xD5:
			temp_val = mem->read_u16(reg.pc_ex);
			cp_u16(reg.hl, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//CP IX, #nnnn
		case 0xD6:
			temp_val = mem->read_u16(reg.pc_ex);
			cp_u16(reg.ix, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//CP IY, #nnnn
		case 0xD7:
			temp_val = mem->read_u16(reg.pc_ex);
			cp_u16(reg.iy, temp_val);
			system_cycles = 12;

			reg.pc += 2;
			break;

		//AND [BR + #nn], #nn
		case 0xD8:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, and_u8(mem->read_u8(temp_val), mem->read_u8(reg.pc_ex++)));
			system_cycles = 20;

			reg.pc += 2;
			break;

		//OR [BR + #nn], #nn
		case 0xD9:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, or_u8(mem->read_u8(temp_val), mem->read_u8(reg.pc_ex++)));
			system_cycles = 20;

			reg.pc += 2;
			break;

		//XOR [BR + #nn], #nn
		case 0xDA:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, xor_u8(mem->read_u8(temp_val), mem->read_u8(reg.pc_ex++)));
			system_cycles = 20;

			reg.pc += 2;
			break;
			
		//CP [BR + #nn], #nn
		case 0xDB:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			cp_u8(temp_val, mem->read_u8(reg.pc_ex++));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//BIT [BR + #nn], #nn
		case 0xDC:
			temp_val = mem->read_u8((reg.br_ex << 8) | mem->read_u8(reg.pc_ex++));
			bit_u8(temp_val, mem->read_u8(reg.pc_ex++));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//LD [BR + #nn], #nn
		case 0xDD:
			temp_val = (reg.br_ex << 8) | mem->read_u8(reg.pc_ex++);
			mem->write_u8(temp_val, mem->read_u8(reg.pc_ex++));
			system_cycles = 16;

			reg.pc += 2;
			break;

		//PCK
		case 0xDE:
			reg.a &= 0xF;
			reg.a |= ((reg.b & 0xF) << 4);
			system_cycles = 8;
			break;

		//UPCK
		case 0xDF:
			reg.b = (reg.a >> 4);
			reg.a &= 0xF;
			system_cycles = 8;
			break;

		//CARS C #ss
		case 0xE0:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			if(reg.sc & CARRY_FLAG)
			{
				mem->write_u8(--reg.sp, reg.cb);
				mem->write_u8(--reg.sp, (reg.pc >> 8));
				mem->write_u8(--reg.sp, (reg.pc & 0xFF));

				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;

				system_cycles = 20;
			}

			else { system_cycles = 8; }

			break;

		//CARS NC #ss
		case 0xE1:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			if((reg.sc & CARRY_FLAG) == 0)
			{
				mem->write_u8(--reg.sp, reg.cb);
				mem->write_u8(--reg.sp, (reg.pc >> 8));
				mem->write_u8(--reg.sp, (reg.pc & 0xFF));

				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;

				system_cycles = 20;
			}

			else { system_cycles = 8; }

			break;

		//CARS Z #ss
		case 0xE2:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			if(reg.sc & ZERO_FLAG)
			{
				mem->write_u8(--reg.sp, reg.cb);
				mem->write_u8(--reg.sp, (reg.pc >> 8));
				mem->write_u8(--reg.sp, (reg.pc & 0xFF));

				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;

				system_cycles = 20;
			}

			else { system_cycles = 8; }

			break;

		//CARS NZ #ss
		case 0xE3:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			if((reg.sc & ZERO_FLAG) == 0)
			{
				mem->write_u8(--reg.sp, reg.cb);
				mem->write_u8(--reg.sp, (reg.pc >> 8));
				mem->write_u8(--reg.sp, (reg.pc & 0xFF));

				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;

				system_cycles = 20;
			}

			else { system_cycles = 8; }

			break;

		//JRS C #ss
		case 0xE4:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			if(reg.sc & CARRY_FLAG)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;
			}

			system_cycles = 8;

			break;

		//JRS NC #ss
		case 0xE5:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			if((reg.sc & CARRY_FLAG) == 0)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;
			}

			system_cycles = 8;

			break;

		//JRS Z #ss
		case 0xE6:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			if(reg.sc & ZERO_FLAG)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;
			}

			system_cycles = 8;

			break;

		//JRS NZ #ss
		case 0xE7:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			if((reg.sc & ZERO_FLAG) == 0)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;
			}

			system_cycles = 8;

			break;

		//CARL C #ssss
		case 0xE8:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			if(reg.sc & CARRY_FLAG)
			{
				mem->write_u8(--reg.sp, reg.cb);
				mem->write_u8(--reg.sp, (reg.pc >> 8));
				mem->write_u8(--reg.sp, (reg.pc & 0xFF));

				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 2;

				system_cycles = 24;
			}

			else { system_cycles = 12; }

			break;

		//CARL NC #ssss
		case 0xE9:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			if((reg.sc & CARRY_FLAG) == 0)
			{
				mem->write_u8(--reg.sp, reg.cb);
				mem->write_u8(--reg.sp, (reg.pc >> 8));
				mem->write_u8(--reg.sp, (reg.pc & 0xFF));

				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 2;

				system_cycles = 24;
			}

			else { system_cycles = 12; }

			break;

		//CARL Z #ssss
		case 0xEA:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			if(reg.sc & ZERO_FLAG)
			{
				mem->write_u8(--reg.sp, reg.cb);
				mem->write_u8(--reg.sp, (reg.pc >> 8));
				mem->write_u8(--reg.sp, (reg.pc & 0xFF));

				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 2;

				system_cycles = 24;
			}

			else { system_cycles = 12; }

			break;

		//CARL NZ #ssss
		case 0xEB:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			if((reg.sc & ZERO_FLAG) == 0)
			{
				mem->write_u8(--reg.sp, reg.cb);
				mem->write_u8(--reg.sp, (reg.pc >> 8));
				mem->write_u8(--reg.sp, (reg.pc & 0xFF));

				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 2;

				system_cycles = 24;
			}

			else { system_cycles = 12; }

			break;

		//JRL C #ssss
		case 0xEC:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			if(reg.sc & CARRY_FLAG)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 2;
			}

			system_cycles = 12;

			break;

		//JRL NC #ssss
		case 0xED:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			if((reg.sc & CARRY_FLAG) == 0)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 2;
			}

			system_cycles = 12;

			break;

		//JRL Z #ssss
		case 0xEE:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			if(reg.sc & ZERO_FLAG)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 2;
			}

			system_cycles = 12;

			break;

		//JRL NZ #ssss
		case 0xEF:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			if((reg.sc & ZERO_FLAG) == 0)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 2;
			}

			system_cycles = 12;

			break;

		//CARS #ss
		case 0xF0:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			mem->write_u8(--reg.sp, reg.cb);
			mem->write_u8(--reg.sp, (reg.pc >> 8));
			mem->write_u8(--reg.sp, (reg.pc & 0xFF));
			reg.cb = reg.nb;
			reg.pc = log_addr + s_temp + 1;
			system_cycles = 20;

			break;

		//JRS #ss
		case 0xF1:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;

			reg.cb = reg.nb;
			reg.pc = log_addr + s_temp + 1;
			system_cycles = 8;
			break;

		//CARL #ssss
		case 0xF2:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			mem->write_u8(--reg.sp, reg.cb);
			mem->write_u8(--reg.sp, (reg.pc >> 8));
			mem->write_u8(--reg.sp, (reg.pc & 0xFF));
			reg.cb = reg.nb;
			reg.pc = log_addr + s_temp + 2;
			break;

		//JRL #ssss
		case 0xF3:
			s_temp = mem->read_s16(reg.pc_ex);
			reg.pc += 2;

			reg.cb = reg.nb;
			reg.pc = log_addr + s_temp + 2;
			system_cycles = 12;
			break;

		//JP HL
		case 0xF4:
			reg.cb = reg.nb;
			reg.pc = reg.hl;
			system_cycles = 8;
			break;

		//DJR NZ #ss
		case 0xF5:
			s_temp = mem->read_s8(reg.pc_ex++);
			reg.pc++;
			reg.b = dec_u8(reg.b);

			if((reg.sc & ZERO_FLAG) == 0)
			{
				reg.cb = reg.nb;
				reg.pc = log_addr + s_temp + 1;
			}

			system_cycles = 16;

			break;

		//SWAP A
		case 0xF6:
			temp_val = reg.a;
			reg.a >>= 4;
			reg.a |= (temp_val << 4);
			system_cycles = 8;
			break;

		//SWAP [HL]
		case 0xF7:
			temp_val = mem->read_u8(reg.hl_ex) >> 4;
			temp_val |= (mem->read_u8(reg.hl_ex) << 4);
			mem->write_u8(reg.hl_ex, temp_val);
			system_cycles = 12;
			break;

		//RET
		case 0xF8:
			reg.pc = mem->read_u16(reg.sp);
			reg.cb = mem->read_u8(reg.sp + 2);
			reg.sp += 3;
			reg.nb = reg.cb;
			system_cycles = 16;
			break;

		//RETE
		case 0xF9:
			reg.sc = mem->read_u8(reg.sp);
			reg.pc = mem->read_u16(reg.sp + 1);
			reg.cb = mem->read_u8(reg.sp + 3);
			reg.sp += 4;
			reg.nb = reg.cb;
			system_cycles = 20;
			break;

		//RETS
		case 0xFA:
			reg.pc = mem->read_u16(reg.sp) + 2;
			reg.cb = mem->read_u8(reg.sp + 2);
			reg.sp += 3;
			reg.nb = reg.cb;
			system_cycles = 24;
			break;

		//CALL [#nnnn]
		case 0xFB:
			temp_val = (reg.ep << 16) | mem->read_u16(reg.pc_ex);
			reg.pc += 2;

			mem->write_u8(--reg.sp, reg.cb);
			mem->write_u8(--reg.sp, (reg.pc >> 8));
			mem->write_u8(--reg.sp, (reg.pc & 0xFF));
			reg.cb = reg.nb;
			reg.pc = mem->read_u16(temp_val);
			system_cycles = 20;
			break;

		//INT
		case 0xFC:
			temp_val = mem->read_u8(reg.pc_ex++);
			reg.pc++;

			mem->write_u8(--reg.sp, reg.cb);
			mem->write_u8(--reg.sp, (reg.pc >> 8));
			mem->write_u8(--reg.sp, (reg.pc & 0xFF));
			mem->write_u8(--reg.sp, reg.sc);
			reg.cb = reg.nb;
			reg.pc = mem->read_u16(temp_val);
			system_cycles = 20;
			break;

		//JP INT
		case 0xFD:
			temp_val = mem->read_u8(reg.pc_ex++);
			reg.pc++;

			reg.cb = reg.nb;
			reg.pc = mem->read_u16(temp_val);
			system_cycles = 8;
			break;

		//NOP
		case 0xFF:
			system_cycles = 8;
			break;

		default:
			std::cout<<"CPU::Error - Unknown opcode -> 0x" << std::hex << opcode << "\n";
			if(!config::ignore_illegal_opcodes) { running = false; }
	}

	//Update logical PC address
	log_addr = reg.pc;
}

/****** Process hardware interrupts ******/
void S1C88::handle_interrupt()
{
	if(skip_irq) { return; }

	u8 irq_mask = (reg.sc >> 6);

	//Cycle through all 4 interrupt levels and jump to vector with highest priority
	for(s32 x = 4; x > irq_mask; x--)
	{
		for(s32 y = 0; y < 32; y++)
		{
			if((mem->irq_enable[y]) && (mem->master_irq_flags & (1 << y)) && (mem->irq_priority[y] == x))
			{
				//Assume System Reset IRQ is not treated like a normal IRQ (i.e. just jump to IRQ vector)
				if(y > 0)
				{
					halt = false;

					mem->write_u8(--reg.sp, reg.cb);
					mem->write_u8(--reg.sp, (reg.pc >> 8));
					mem->write_u8(--reg.sp, (reg.pc & 0xFF));
					mem->write_u8(--reg.sp, reg.sc);

					reg.cb = 0;
					reg.sc |= 0xC0;
				}

				else { mem->master_irq_flags &= ~0x1; }

				reg.pc = mem->irq_vectors[y];

				x = 0;
				y = 32;
			}
		}
	}
}

/****** Calculates most recent extended values for certain paged registers ******/
void S1C88::update_regs()
{
	//Calculate extended PC
	if((reg.pc & 0xFFFF) > 0x7FFF) { reg.pc_ex = (reg.pc & 0x7FFF) + (0x8000 * reg.cb); }
	else { reg.pc_ex = reg.pc; }

	//Calculate extended IX
	if(reg.xp) { reg.ix_ex = reg.ix + (0x10000 * reg.xp); }
	else { reg.ix_ex = reg.ix; }

	//Calculate extended IY
	if(reg.yp) { reg.iy_ex = reg.iy + (0x10000 * reg.yp); }
	else { reg.iy_ex = reg.iy; }

	//Calculate extended BR and HL
	if(reg.ep)
	{
		reg.hl_ex = reg.hl + (0x10000 * reg.ep);
		reg.br_ex = reg.br + (0x10000 * reg.ep);
	}

	else
	{
		reg.hl_ex = reg.hl;
		reg.br_ex = reg.br;
	}
}

/****** Clocks CPU and Pokemon Mini subsystems ******/
void S1C88::clock_system()
{
	//Clock CPU cycles for PRC
	controllers.video.lcd_stat.prc_clock += system_cycles;
	
	//Increment PRC Count
	if(controllers.video.lcd_stat.prc_clock >= (controllers.video.lcd_stat.prc_counter * PRC_COUNT_TIME))
	{
		controllers.video.lcd_stat.prc_counter++;

		//Wait on PRC counts after rendering or copying frames to send PRC Copy Complete IRQ
		if(controllers.video.lcd_stat.prc_copy_wait)
		{
			controllers.video.lcd_stat.prc_copy_wait--;
			
			//Trigger PRC Copy Complete IRQ
			if(!controllers.video.lcd_stat.prc_copy_wait) { mem->update_irq_flags(PRC_COPY_IRQ); }
		}
			
		//Reset PRC Count and increment frame counter
		if(controllers.video.lcd_stat.prc_counter == 0x42)
		{
			u8 frame_counter = (controllers.video.lcd_stat.prc_rate & 0xF0) >> 4;
			frame_counter++;

			//Update LCD with new rendering
			if(frame_counter >= controllers.video.lcd_stat.prc_rate_div)
			{
				//Trigger PRC Overflow IRQ
				mem->update_irq_flags(PRC_OVERFLOW_IRQ);

				frame_counter = 0;

				if(controllers.video.lcd_stat.enable_copy)
				{
					controllers.video.new_frame = true;

					//Wait 11 PRC counts if only frame copying
					if((controllers.video.lcd_stat.prc_mode & 0xE) == 0x8) { controllers.video.lcd_stat.prc_copy_wait = 11; }

					//Wait 44 PRC counts if rendering and frame copying
					else if(controllers.video.lcd_stat.prc_mode & 0x6) { controllers.video.lcd_stat.prc_copy_wait = 44; }
				}

				else { controllers.video.new_frame = false; }
			}

			//Otherwise refresh using existing pixel data
			else { controllers.video.new_frame = false; }

			controllers.video.update();

			controllers.video.lcd_stat.prc_rate &= ~0xF0;
			controllers.video.lcd_stat.prc_rate |= (frame_counter << 4);
			controllers.video.lcd_stat.prc_counter = 0x1;
		}

		if((controllers.video.lcd_stat.prc_counter == 0x1) || (controllers.video.lcd_stat.prc_counter == 0x21))
		{
			//Generate audio buffer for sound channels on VBlank
			if(controllers.audio.apu_stat.pwm_needs_fill) { controllers.audio.buffer_channel(); }
			controllers.audio.apu_stat.pwm_needs_fill = true;
		}

		//Reset counter for CPU cycles
		if(controllers.video.lcd_stat.prc_clock >= 55634) { controllers.video.lcd_stat.prc_clock -= 55634; }
	}

	//Increment Timer Counts
	for(u32 x = 0; x < 4; x++)
	{
		//Clock Low Timers
		if(controllers.timer[x].enable_lo)
		{
			u32 lo_count = 0;
			u32 prescalar = 0;
			u16 count_mask = 0;

			//16-bit mode
			if(controllers.timer[x].full_mode)
			{
				lo_count = controllers.timer[x].counter;
				count_mask = 0xFFFF;
				prescalar = controllers.timer[x].prescalar_lo;
			}
			
			//8-bit mode
			else
			{
				lo_count = controllers.timer[x].counter & 0xFF00;
				count_mask = 0xFF00;
				prescalar = controllers.timer[x].prescalar_lo;
			} 

			controllers.timer[x].clock_lo += system_cycles;

			if(controllers.timer[x].clock_lo >= prescalar)
			{
				controllers.timer[x].clock_lo -= prescalar;

				//Timers 1 - 3 decrement
				if(x < 3) { lo_count--; }

				//256Hz Timer increments 
				else
				{
					lo_count = controllers.timer[x].counter & 0xFF;
					lo_count++;
					count_mask = 0xFF;
				}

				//Adjust counter
				controllers.timer[x].counter = (controllers.timer[x].counter & ~count_mask);
				lo_count &= count_mask;
				controllers.timer[x].counter |= lo_count;

				//Trigger interrupts
				switch(x)
				{
					//Timer 1
					case 0:
						//Lower Underflow, 8-bit mode
						if(((lo_count & 0xFF) == 0xFF) && (!controllers.timer[0].full_mode))
						{
							mem->update_irq_flags(TIMER1_LOWER_UNDERFLOW_IRQ);

							//Copy preset value
							controllers.timer[0].counter = (controllers.timer[0].counter & ~0xFF);
							controllers.timer[0].counter |= (controllers.timer[0].reload_value & 0xFF);
						}

						//Upper Underflow, 16-bit mode
						else if(((lo_count & 0xFFFF) == 0xFFFF) && (controllers.timer[0].full_mode))
						{
							mem->update_irq_flags(TIMER1_UPPER_UNDERFLOW_IRQ);

							//Copy preset value
							controllers.timer[0].counter = controllers.timer[0].reload_value;
						}

						break; 

					//Timer 2
					case 1:
						//Lower Underflow, 8-bit mode
						if(((lo_count & 0xFF) == 0xFF) && (!controllers.timer[1].full_mode))
						{
							mem->update_irq_flags(TIMER2_LOWER_UNDERFLOW_IRQ);

							//Copy preset value
							controllers.timer[1].counter = (controllers.timer[1].counter & ~0xFF);
							controllers.timer[1].counter |= (controllers.timer[1].reload_value & 0xFF);
						}

						//Upper Underflow, 16-bit mode
						else if(((lo_count & 0xFFFF) == 0xFFFF) && (controllers.timer[1].full_mode))
						{
							mem->update_irq_flags(TIMER2_UPPER_UNDERFLOW_IRQ);

							//Copy preset value
							controllers.timer[1].counter = controllers.timer[1].reload_value;
						}

						break; 

					//Timer 3
					case 2:
						{
							bool old_pivot = controllers.timer[2].pivot_status;

							//Upper Underflow, 16-bit mode only
							if(((lo_count & 0xFFFF) == 0xFFFF) && (controllers.timer[2].full_mode))
							{
								mem->update_irq_flags(TIMER3_UPPER_UNDERFLOW_IRQ);

								//Copy preset value
								controllers.timer[2].counter = controllers.timer[2].reload_value;
								controllers.timer[2].pivot_status = 0;
							}

							//Pivot
							if(((lo_count & count_mask) <= controllers.timer[2].pivot) && (old_pivot == 0))
							{
								mem->update_irq_flags(TIMER3_PIVOT_IRQ);
								controllers.timer[2].pivot_status = 1;
							}
						}
						
						break;

					//256Hz Timer
					case 3:
						//1Hz IRQ
						if(controllers.timer[3].counter == 0) { mem->update_irq_flags(TIMER_1HZ_IRQ); }

						//2Hz IRQ
						if((controllers.timer[3].counter % 128) == 0) { mem->update_irq_flags(TIMER_2HZ_IRQ); }

						//8Hz IRQ
						if((controllers.timer[3].counter % 32) == 0) { mem->update_irq_flags(TIMER_8HZ_IRQ); }

						//32Hz IRQ
						if((controllers.timer[3].counter % 8) == 0) { mem->update_irq_flags(TIMER_32HZ_IRQ); }

						break;
				}
			}
		}

		//Clock High Timers
		if(controllers.timer[x].enable_hi && !controllers.timer[x].full_mode)
		{
			u32 hi_count = controllers.timer[x].counter & 0xFF00;

			controllers.timer[x].clock_hi += system_cycles;

			if(controllers.timer[x].clock_hi >= controllers.timer[x].prescalar_hi)
			{
				controllers.timer[x].clock_hi -= controllers.timer[x].prescalar_hi;

				//Timers 1 - 3 decrement
				hi_count--;

				//Adjust counter
				controllers.timer[x].counter = (controllers.timer[x].counter & ~0xFF00);
				hi_count &= 0xFF00;
				controllers.timer[x].counter |= hi_count;

				//Trigger interrupts
				switch(x)
				{
					//Timer 1
					case 0:
						//Upper Underflow, 8-bit mode
						if((hi_count & 0xFF00) == 0xFF00)
						{
							mem->update_irq_flags(TIMER1_UPPER_UNDERFLOW_IRQ);

							//Copy preset value
							controllers.timer[0].counter = (controllers.timer[0].counter & ~0xFF00);
							controllers.timer[0].counter |= (controllers.timer[0].reload_value & 0xFF00);
						}

						break; 

					//Timer 2
					case 1:
						//Lower Underflow, 8-bit mode
						if((hi_count & 0xFF00) == 0xFF00)
						{
							mem->update_irq_flags(TIMER2_UPPER_UNDERFLOW_IRQ);

							//Copy preset value
							controllers.timer[1].counter = (controllers.timer[1].counter & ~0xFF00);
							controllers.timer[1].counter |= (controllers.timer[1].reload_value & 0xFF00);
						}

						break; 

					//Timer 3
					case 2: break;
				}
			}
		}
	}	

	//Fade active IR signal
	if(mem->ir_stat.fade != 0)
	{
		mem->ir_stat.fade -= system_cycles;

		if(mem->ir_stat.fade <= 0)
		{
			mem->ir_stat.fade = 0;
			mem->ir_stat.signal = 0;
			mem->memory_map[PM_IO_DATA] |= 0x2;
		}
	}

	//Update RTC
	if(mem->enable_rtc)
	{
		mem->rtc_cycles += system_cycles;
		
		if(mem->rtc_cycles >= 4000000)
		{
			mem->rtc_cycles -= 4000000;
			mem->rtc++;
		}
	}
			
	if(mem->ir_stat.debug_cycles != 0xDEADBEEF) { mem->ir_stat.debug_cycles += system_cycles; }
}

/****** Read CPU data from save state ******/
bool S1C88::cpu_read(u32 offset, std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	
	if(!file.is_open()) { return false; }

	//Go to offset
	file.seekg(offset);

	//Serialize CPU registers data from file stream
	file.read((char*)&reg, sizeof(reg));

	//Serialize misc CPU data from file stream
	file.read((char*)&opcode, sizeof(opcode));
	file.read((char*)&log_addr, sizeof(log_addr));
	file.read((char*)&system_cycles, sizeof(system_cycles));
	file.read((char*)&debug_cycles, sizeof(debug_cycles));
	file.read((char*)&halt, sizeof(halt));
	file.read((char*)&debug_opcode, sizeof(debug_opcode));
	file.read((char*)&running, sizeof(running));
	file.read((char*)&skip_irq, sizeof(skip_irq));

	//Serialize timers from file stream
	file.read((char*)&controllers.timer[0], sizeof(controllers.timer[0]));
	file.read((char*)&controllers.timer[1], sizeof(controllers.timer[1]));
	file.read((char*)&controllers.timer[2], sizeof(controllers.timer[2]));
	file.read((char*)&controllers.timer[3], sizeof(controllers.timer[3]));

	file.close();
	return true;
}

/****** Write CPU data to save state ******/
bool S1C88::cpu_write(std::string filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::app);
	
	if(!file.is_open()) { return false; }

	//Serialize CPU registers data to save state
	file.write((char*)&reg, sizeof(reg));

	//Serialize misc CPU data to save state
	file.write((char*)&opcode, sizeof(opcode));
	file.write((char*)&log_addr, sizeof(log_addr));
	file.write((char*)&system_cycles, sizeof(system_cycles));
	file.write((char*)&debug_cycles, sizeof(debug_cycles));
	file.write((char*)&halt, sizeof(halt));
	file.write((char*)&debug_opcode, sizeof(debug_opcode));
	file.write((char*)&running, sizeof(running));
	file.write((char*)&skip_irq, sizeof(skip_irq));

	//Serialize timers from file stream
	file.write((char*)&controllers.timer[0], sizeof(controllers.timer[0]));
	file.write((char*)&controllers.timer[1], sizeof(controllers.timer[1]));
	file.write((char*)&controllers.timer[2], sizeof(controllers.timer[2]));
	file.write((char*)&controllers.timer[3], sizeof(controllers.timer[3]));

	file.close();
	return true;
}

/****** Gets the size of CPU data for serialization ******/
u32 S1C88::size()
{
	u32 cpu_size = 0;

	cpu_size += sizeof(reg);
	cpu_size += sizeof(opcode);
	cpu_size += sizeof(log_addr);
	cpu_size += sizeof(system_cycles);
	cpu_size += sizeof(debug_cycles);
	cpu_size += sizeof(halt);
	cpu_size += sizeof(debug_opcode);
	cpu_size += sizeof(running);
	cpu_size += sizeof(skip_irq);

	//Serialize timers from file stream
	cpu_size += sizeof(controllers.timer[0]);
	cpu_size += sizeof(controllers.timer[1]);
	cpu_size += sizeof(controllers.timer[2]);
	cpu_size += sizeof(controllers.timer[3]);

	return cpu_size;
}
