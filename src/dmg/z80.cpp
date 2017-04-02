// GB Enhanced Copyright Daniel Baxter 2013
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : z80.cpp
// Date : July 27, 2013
// Description : Game Boy Z80 CPU emulator
//
// Emulates the GB Z80 in software

#include "z80.h"

/****** Z80 Constructor ******/
Z80::Z80() 
{
	if(config::use_bios) { reset_bios(); }
	else { reset(); }
}

/****** Z80 Deconstructor ******/
Z80::~Z80() 
{ 
	std::cout<<"CPU::Shutdown\n";
}

/****** Z80 Reset ******/
void Z80::reset() 
{
	//Values represent HLE BIOS
	reg.a = (config::gb_type == 2) ? 0x11 : 0x01;
	reg.b = (config::gba_enhance) ? 0x01 : 0x00;
	reg.c = 0x13;
	reg.d = 0x00;
	reg.e = 0xD8;
	reg.h = 0x01;
	reg.l = 0x4D;
	reg.f = 0xB0;
	reg.pc = 0x100;
	reg.sp = 0xFFFE;
	temp_byte = 0;
	temp_word = 0;
	cpu_clock_m = 0;
	cpu_clock_t = 0;
	div_counter = 0;
	tima_counter = 0;
	tima_speed = 0;
	cycles = 0;
	running = false;
	halt = false;
	pause = false;
	interrupt = false;
	interrupt_delay = false;
	double_speed = false;
	skip_instruction = false;

	mem = NULL;

	std::cout<<"CPU::Initialized\n";
}

/****** Z80 Reset - For BIOS ******/
void Z80::reset_bios() 
{
	reg.a = 0x00;
	reg.b = 0x00;
	reg.c = 0x00;
	reg.d = 0x00;
	reg.e = 0x00;
	reg.h = 0x00;
	reg.l = 0x00;
	reg.f = 0x00;
	reg.pc = 0x00;
	reg.sp = 0x0000;
	temp_byte = 0;
	temp_word = 0;
	cpu_clock_m = 0;
	cpu_clock_t = 0;
	div_counter = 0;
	tima_counter = 0;
	tima_speed = 0;
	cycles = 0;
	running = false;
	halt = false;
	pause = false;
	interrupt = false;
	interrupt_delay = false;
	double_speed = false;
	skip_instruction = false;

	mem = NULL;

	std::cout<<"CPU::Initialized\n";
}

/****** Read CPU data from save state ******/
bool Z80::cpu_read(u32 offset, std::string filename)
{
	std::ifstream file(filename.c_str(), std::ios::binary);
	
	if(!file.is_open()) { return false; }

	//Go to offset
	file.seekg(offset);

	//Serialize CPU registers data to file stream
	file.read((char*)&reg.a, sizeof(reg.a));
	file.read((char*)&reg.b, sizeof(reg.b));
	file.read((char*)&reg.c, sizeof(reg.c));
	file.read((char*)&reg.d, sizeof(reg.d));
	file.read((char*)&reg.e, sizeof(reg.e));
	file.read((char*)&reg.h, sizeof(reg.h));
	file.read((char*)&reg.l, sizeof(reg.l));
	file.read((char*)&reg.f, sizeof(reg.f));
	file.read((char*)&reg.pc, sizeof(reg.pc));
	file.read((char*)&reg.sp, sizeof(reg.sp));

	//Serialize CPU clock data to file stream
	file.read((char*)&cpu_clock_m, sizeof(cpu_clock_m));
	file.read((char*)&cpu_clock_t, sizeof(cpu_clock_t));
	file.read((char*)&div_counter, sizeof(div_counter));
	file.read((char*)&tima_counter, sizeof(tima_counter));
	file.read((char*)&tima_speed, sizeof(tima_speed));
	file.read((char*)&cycles, sizeof(cycles));
	
	//Serialize misc CPU data to filestream
	file.read((char*)&running, sizeof(running));
	file.read((char*)&halt, sizeof(halt));
	file.read((char*)&pause, sizeof(pause));
	file.read((char*)&interrupt, sizeof(interrupt));
	file.read((char*)&double_speed, sizeof(double_speed));
	file.read((char*)&interrupt_delay, sizeof(interrupt_delay));
	file.read((char*)&skip_instruction, sizeof(skip_instruction));

	file.close();
	return true;
}

/****** Write CPU data to save state ******/
bool Z80::cpu_write(std::string filename)
{
	std::ofstream file(filename.c_str(), std::ios::binary | std::ios::trunc);
	
	if(!file.is_open()) { return false; }

	//Serialize CPU registers data to file stream
	file.write((char*)&reg.a, sizeof(reg.a));
	file.write((char*)&reg.b, sizeof(reg.b));
	file.write((char*)&reg.c, sizeof(reg.c));
	file.write((char*)&reg.d, sizeof(reg.d));
	file.write((char*)&reg.e, sizeof(reg.e));
	file.write((char*)&reg.h, sizeof(reg.h));
	file.write((char*)&reg.l, sizeof(reg.l));
	file.write((char*)&reg.f, sizeof(reg.f));
	file.write((char*)&reg.pc, sizeof(reg.pc));
	file.write((char*)&reg.sp, sizeof(reg.sp));

	//Serialize CPU clock data to file stream
	file.write((char*)&cpu_clock_m, sizeof(cpu_clock_m));
	file.write((char*)&cpu_clock_t, sizeof(cpu_clock_t));
	file.write((char*)&div_counter, sizeof(div_counter));
	file.write((char*)&tima_counter, sizeof(tima_counter));
	file.write((char*)&tima_speed, sizeof(tima_speed));
	file.write((char*)&cycles, sizeof(cycles));
	
	//Serialize misc CPU data to filestream
	file.write((char*)&running, sizeof(running));
	file.write((char*)&halt, sizeof(halt));
	file.write((char*)&pause, sizeof(pause));
	file.write((char*)&interrupt, sizeof(interrupt));
	file.write((char*)&double_speed, sizeof(double_speed));
	file.write((char*)&interrupt_delay, sizeof(interrupt_delay));
	file.write((char*)&skip_instruction, sizeof(skip_instruction));

	file.close();
	return true;
}

/****** Gets the size of CPU data for serialization ******/
u32 Z80::size()
{
	u32 cpu_size = 0; 

	cpu_size += sizeof(reg.a);
	cpu_size += sizeof(reg.b);
	cpu_size += sizeof(reg.c);
	cpu_size += sizeof(reg.d);
	cpu_size += sizeof(reg.e);
	cpu_size += sizeof(reg.h);
	cpu_size += sizeof(reg.l);
	cpu_size += sizeof(reg.f);
	cpu_size += sizeof(reg.pc);
	cpu_size += sizeof(reg.sp);

	cpu_size += sizeof(cpu_clock_m);
	cpu_size += sizeof(cpu_clock_t);
	cpu_size += sizeof(div_counter);
	cpu_size += sizeof(tima_counter);
	cpu_size += sizeof(tima_speed);
	cpu_size += sizeof(cycles);
	
	cpu_size += sizeof(running);
	cpu_size += sizeof(halt);
	cpu_size += sizeof(pause);
	cpu_size += sizeof(interrupt);
	cpu_size += sizeof(double_speed);
	cpu_size += sizeof(interrupt_delay);
	cpu_size += sizeof(skip_instruction);

	return cpu_size;
}

/****** Handle Interrupts to Z80 ******/
bool Z80::handle_interrupts()
{
	//Delay interrupts when EI is called
	if(interrupt_delay)
	{
		interrupt_delay = false;
		interrupt = true;
		return true;
	}

	//Only perform interrupts when the IME is enabled
	else if(interrupt)
	{
		//Perform VBlank Interrupt
		if((mem->memory_map[IE_FLAG] & 0x01) && (mem->memory_map[IF_FLAG] & 0x01))
		{
			interrupt = false;
			halt = false;
			mem->memory_map[IF_FLAG] &= ~0x01;
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x40;
			cycles += 36;
			return true;
		}

		//Perform LCD Status Interrupt
		if((mem->memory_map[IE_FLAG] & 0x02) && (mem->memory_map[IF_FLAG] & 0x02))
		{
			interrupt = false;
			halt = false;
			mem->memory_map[IF_FLAG] &= ~0x02;
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x48;
			cycles += 36;
			return true;
		}

		//Perform Timer Overflow Interrupt
		if((mem->memory_map[IE_FLAG] & 0x04) && (mem->memory_map[IF_FLAG] & 0x04))
		{
			interrupt = false;
			halt = false;
			mem->memory_map[IF_FLAG] &= ~0x04;
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x50;
			cycles += 36;
			return true;
		}

		//Perform Serial Input-Output Interrupt
		if((mem->memory_map[IE_FLAG] & 0x08) && (mem->memory_map[IF_FLAG] & 0x08))
		{
			interrupt = false;
			halt = false;
			mem->memory_map[IF_FLAG] &= ~0x08;
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x58;
			cycles += 36;
			return true;
		}

		//Perform Joypad Interrupt
		if((mem->memory_map[IE_FLAG] & 0x10) && (mem->memory_map[IF_FLAG] & 0x10))
		{
			interrupt = false;
			halt = false;
			mem->memory_map[IF_FLAG] &= ~0x10;
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x60;
			cycles += 36;
			return true;
		}

		else { return false; }
	}

	//When IME is disabled, pending interrupts will exit the HALT state
	else if((mem->memory_map[IF_FLAG] & mem->memory_map[IE_FLAG] & 0x1F) && (!skip_instruction)) { halt = false; return false; }

	else { return false; }
}	

/****** Relative jump by signed immediate ******/
void Z80::jr(u8 reg_one)
{
	if ((reg_one & 0x80) == 0x80) {
		--reg_one; 
		reg_one = ~reg_one;
		reg.pc -= reg_one;
	} 

	else { reg.pc += reg_one; }
}

/****** Swaps nibbles ******/
u8 Z80::swap(u8 reg_one)
{
	reg.f = 0;
	u8 temp_one = (reg_one & 0xF) << 4;
	u8 temp_two = (reg_one >> 4) & 0xF;
	reg_one = (temp_one | temp_two);
	
	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }

	return reg_one;
}

/****** 8-bit addition ******/
u8 Z80::add_byte(u8 reg_one, u8 reg_two)
{
	reg.f = 0;

	//Carry
	if(reg_one + reg_two > 0xFF) { reg.f |= 0x10; }

	//Half-Carry
	if((reg_one & 0xF) + (reg_two & 0xF) > 0xF) { reg.f |= 0x20; }

	//Zero
	if(u8(reg_one + reg_two) == 0) { reg.f |= 0x80; }

	return reg_one + reg_two;
}

/****** 8-bit addition - Carry ******/
u8 Z80::add_carry(u8 reg_one, u8 reg_two)
{
	u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
	u8 carry_set = 0;
	u8 half_carry_set = 0;

	u8 temp_1, temp_2 = 0;

	temp_1 = reg_one + carry_flag;

	if(reg_one + carry_flag > 0xFF) { carry_set = 1; }
	if((reg_one & 0x0F) + (carry_flag & 0x0F) > 0x0F) { half_carry_set = 1; }

	temp_2 = temp_1 + reg_two;

	if(temp_1 + reg_two > 0xFF) { carry_set = 1; }
	if((temp_1 & 0x0F) + (reg_two & 0x0F) > 0x0F) { half_carry_set = 1; }

	reg.f = 0;

	//Carry
	if(carry_set == 1) { reg.f |= 0x10; }

	//Half Carry
	if(half_carry_set == 1) { reg.f |= 0x20; }

	//Zero
	if(temp_2 == 0) { reg.f |= 0x80; }

	return temp_2;
}

/****** 16-bit addition ******/
u16 Z80::add_word(u16 reg_one, u16 reg_two)
{
	u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
	reg.f = 0;

	//Carry
	if(reg_one + reg_two > 0xFFFF) { reg.f |= 0x10; }

	//Half-Carry
	if((reg_one & 0x0FFF) + (reg_two & 0x0FFF) > 0x0FFF) { reg.f |= 0x20; }

	//Zero
	if(zero_flag == 1) { reg.f |= 0x80; }

	return reg_one + reg_two;
}

/****** 16-bit addition with signed byte ******/
u16 Z80::add_signed_byte(u16 reg_one, u8 reg_two)
{
	s16 reg_two_bsx = (s16)((s8)reg_two);
	u16 result = reg_one + reg_two_bsx;

	reg.f = 0;

	//Carry
	if((reg_one & 0xFF) + reg_two > 0xFF) { reg.f |= 0x10; }

	//Half-Carry
	if((reg_one & 0xF) + (reg_two & 0xF) > 0xF) { reg.f |= 0x20; }

	return result;
}

/****** 8-bit subtraction ******/
u8 Z80::sub_byte(u8 reg_one, u8 reg_two)
{
	reg.f = 0;

	//Carry
	if(reg_one < reg_two) { reg.f |= 0x10; }

	//Half Carry
	if((reg_one & 0xF) < (reg_two & 0xF)) { reg.f |= 0x20; }

	//Subtract
	reg.f |= 0x40;

	//Zero
	if(u8(reg_one - reg_two) == 0) { reg.f |= 0x80; }

	return reg_one - reg_two;
}

/****** 8-bit subtraction - Carry ******/
u8 Z80::sub_carry(u8 reg_one, u8 reg_two)
{
	u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
	u8 carry_set = 0;
	u8 half_carry_set = 0;

	u8 temp_1, temp_2 = 0;

	temp_1 = reg_one - carry_flag;

	if(reg_one < carry_flag) { carry_set = 1; }
	if ((reg_one & 0x0F) < (carry_flag & 0x0F)) { half_carry_set = 1; }

	temp_2 = temp_1 - reg_two;

	if(temp_1 < reg_two) { carry_set = 1; }
	if ((temp_1 & 0x0F) < (reg_two & 0x0F)) { half_carry_set = 1; }
	
	reg.f = 0;

	//Carry
	if(carry_set == 1) { reg.f |= 0x10; }

	//Half Carry
	if(half_carry_set == 1) { reg.f |= 0x20; }

	//Subtract
	reg.f |= 0x40;

	//Zero
	if(temp_2 == 0) { reg.f |= 0x80; }

	return temp_2;
}

/****** 8-bit AND ******/
u8 Z80::and_byte(u8 reg_one, u8 reg_two)
{
	reg.f = 0;

	//Half Carry
	reg.f |= 0x20;

	//Zero
	if((reg_one & reg_two) == 0) { reg.f |= 0x80; }

	return reg_one & reg_two;
}

/****** 8-bit OR ******/
u8 Z80::or_byte(u8 reg_one, u8 reg_two)
{
	reg.f = 0;

	//Zero
	if((reg_one | reg_two) == 0) { reg.f |= 0x80; }

	return reg_one | reg_two;
}

/****** 8-bit XOR ******/
u8 Z80::xor_byte(u8 reg_one, u8 reg_two)
{
	reg.f = 0;
	
	//Zero
	if((reg_one ^ reg_two) == 0) { reg.f |= 0x80; }

	return reg_one ^ reg_two;
}

/****** 8-bit increment ******/
u8 Z80::inc_byte(u8 reg_one)
{
	u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
	reg_one++;

	reg.f = 0;

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Half Carry
	if((reg_one & 0xF) == 0) { reg.f |= 0x20; }

	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }

	return reg_one;
}

/****** 8-bit decrement ******/
u8 Z80::dec_byte(u8 reg_one)
{
	u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
	reg_one--;

	reg.f = 0;

	//Subtract
	reg.f |= 0x40;

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Half Carry
	if((reg_one & 0xF) == 0xF) { reg.f |= 0x20; }

	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }

	return reg_one;
}

/****** Check bit ******/
void Z80::bit(u8 reg_one, u8 check_bit)
{
	u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
	reg.f = 0;

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Half Carry
	reg.f |= 0x20;

	//Zero
	if((reg_one & check_bit) == 0) { reg.f |= 0x80; }
}

/****** Reset bit ******/
u8 Z80::res(u8 reg_one, u8 reset_bit)
{
	reg_one &= ~reset_bit;
	return reg_one;
}

/****** Set bit ******/
u8 Z80::set(u8 reg_one, u8 set_bit)
{
	reg_one |=set_bit;
	return reg_one;
}

/****** Rotate byte left *****/
u8 Z80::rotate_left(u8 reg_one)
{
	u8 old_carry = (reg.f & 0x10) ? 1 : 0;
	reg.f = 0;
	
	u8 carry_flag = (reg_one & 0x80) >> 7;
	reg_one = (reg_one << 1) + old_carry;

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }

	return reg_one;
}

/****** Rotate byte left through carry *****/
u8 Z80::rotate_left_carry(u8 reg_one)
{
	reg.f = 0;	

	u8 carry_flag = (reg_one & 0x80) >> 7;
	reg_one = (reg_one << 1) + carry_flag;

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }
	
	return reg_one;
}

/****** Rotate byte right  ******/
u8 Z80::rotate_right(u8 reg_one)
{
	u8 old_carry = (reg.f & 0x10) ? 1 : 0;
	reg.f = 0;

	u8 carry_flag = (reg_one & 0x01);
	reg_one = (reg_one >> 1) + (old_carry << 7);

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Zero
	if(reg_one == 0) {reg.f |= 0x80; }

	return reg_one;
}

/****** Rotate byte right through carry ******/
u8 Z80::rotate_right_carry(u8 reg_one)
{
	reg.f = 0;

	//Store 1st bit in Carry Flag
	u8 carry_flag = (reg_one & 0x01);
	reg_one = (reg_one >> 1) + (carry_flag << 7);

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Zero
	if(reg_one == 0) {reg.f |= 0x80; }

	return reg_one;
}

/****** Shift byte left into carry - Preserve sign ******/
u8 Z80::sla(u8 reg_one)
{
	reg.f = 0;
	u8 carry_flag = (reg_one & 0x80) ? 1 : 0;
	reg_one <<= 1;

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }

	return reg_one;
}

/****** Shift byte right into carry - Preserve sign ******/
u8 Z80::sra(u8 reg_one)
{
	reg.f = 0;
	u8 carry_flag = (reg_one & 0x01) ? 1 : 0;
	reg_one >>= 1;
	reg_one |= ((reg_one & 0x40) << 1);

	//Carry
	if(carry_flag == 1) { reg.f = 0x10; }
	
	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }

	return reg_one;
}

/****** Shift byte right into carry ******/
u8 Z80::srl(u8 reg_one)
{
	reg.f = 0;
	u8 carry_flag = (reg_one & 0x01) ? 1 : 0;
	reg_one >>= 1;

	//Carry
	if(carry_flag == 1) { reg.f |= 0x10; }

	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }

	return reg_one;
}

/****** Decimal adjust accumulator ******/
u8 Z80::daa()
{
	u32 reg_one = reg.a;
	
	//Add or subtract correction values based on Subtract Flag
	if(!(reg.f & 0x40))
	{
		if((reg.f & 0x20) || ((reg_one & 0xF) > 0x09)) { reg_one += 0x06; }
		if((reg.f & 0x10) || (reg_one > 0x9F)) { reg_one += 0x60; }
	}

	else 
	{
		if(reg.f & 0x20) { reg_one = (reg_one - 0x06) & 0xFF; }
		if(reg.f & 0x10) { reg_one -= 0x60; }
	}

	//Carry
	if(reg_one & 0x100) { reg.f |= 0x10; }
	reg_one &= 0xFF;

	//Half-Carry
	reg.f &= ~0x20;

	//Zero
	if(reg_one == 0) { reg.f |= 0x80; }
	else { reg.f &= ~0x80; }
	 
	return reg_one;	
}

/****** Execute 8-bit opcodes ******/
void Z80::exec_op(u8 opcode)
{
	switch (opcode)
	{
		//NOP
		case 0x00 :
			cycles += 4;
			break;

		//LD BC, nn
		case 0x01 :
			reg.bc = mem->read_u16(reg.pc);
			reg.pc += 2;
			cycles += 12;
			break;

		//LD BC, A
		case 0x02 :
			mem->write_u8(reg.bc, reg.a);
			cycles += 8;
			break;

		//INC BC
		case 0x03 :
			reg.bc++;
			cycles += 8;
			break;

		//INC B
		case 0x04 :
			reg.b = inc_byte(reg.b);
			cycles += 4;
			break;

		//DEC B
		case 0x05 :
			reg.b = dec_byte(reg.b);
			cycles += 4;
			break;

		//LD B, n
		case 0x06 :
			reg.b = mem->read_u8(reg.pc++);
			cycles += 8;
			break;

		//RLC A
		case 0x07 :
			reg.a = rotate_left_carry(reg.a);
			reg.f &= ~0x80;
			cycles += 4;
			break;

		//LD nn, SP
		case 0x08 :
			mem->write_u16(mem->read_u16(reg.pc), reg.sp);
			reg.pc += 2;
			cycles += 20;
			break;

		//ADD HL, BC
		case 0x09 :
			reg.hl = add_word(reg.hl, reg.bc);
			cycles += 8;
			break;

		//LD A, BC
		case 0x0A :
			reg.a = mem->read_u8(reg.bc);
			cycles += 8;
			break;

		//DEC BC
		case 0x0B :
			reg.bc--;
			cycles += 8;
			break;

		//INC C
		case 0x0C :
			reg.c = inc_byte(reg.c);
			cycles += 4;
			break;

		//DEC C
		case 0x0D :
			reg.c = dec_byte(reg.c);
			cycles += 4;
			break;

		//LD C, n
		case 0x0E :
			reg.c = mem->read_u8(reg.pc++);
			cycles += 8;
			break;

		//RRC A
		case 0x0F :
			reg.a = rotate_right_carry(reg.a);
			reg.f &= ~0x80;
			cycles += 8;
			break;

		//STOP
		case 0x10 :
			//GBC - Normal to double speed mode
			if((config::gb_type == 2) && (mem->memory_map[REG_KEY1] & 0x1) && ((mem->memory_map[REG_KEY1] & 0x80) == 0))
			{
				double_speed = true;
				mem->memory_map[REG_KEY1] = 0x80;

				//Set SIO clock - 16384Hz - Bit 1 cleared, Double Speed
				if((mem->memory_map[REG_SC] & 0x2) == 0) { controllers.serial_io.sio_stat.shift_clock = 256; }

				//Set SIO clock - 524288Hz - Bit 1 set, Double Speed
				else { controllers.serial_io.sio_stat.shift_clock = 8; }
			}

			//GBC - Double to normal speed mode
			if((config::gb_type == 2) && (mem->memory_map[REG_KEY1] & 0x1) && (mem->memory_map[REG_KEY1] & 0x80))
			{
				double_speed = false;
				mem->memory_map[REG_KEY1] = 0;

				//Set SIO clock - 8192Hz - Bit 1 cleared, Normal Speed
				if((mem->memory_map[REG_SC] & 0x2) == 0) { controllers.serial_io.sio_stat.shift_clock = 512; }

				//Set SIO clock - 262144Hz - Bit 1 set, Normal Speed
				else { controllers.serial_io.sio_stat.shift_clock = 16; }
			}
			break;	

		//LD DE, nn
		case 0x11 :
			reg.de = mem->read_u16(reg.pc);
			reg.pc += 2;
			cycles += 12;
			break;

		//LD DE, A
		case 0x12 :
			mem->write_u8(reg.de, reg.a);
			cycles += 8;
			break;

		//INC DE
		case 0x13 :
			reg.de++;
			cycles += 8;
			break;

		//INC D
		case 0x14 :
			reg.d = inc_byte(reg.d);
			cycles += 4;
			break;

		//DEC D
		case 0x15 :
			reg.d = dec_byte(reg.d);
			cycles += 4;
			break;

		//LD D, n
		case 0x16 :
			reg.d = mem->read_u8(reg.pc++);
			cycles += 8;
			break;

		//RL A
		case 0x17 :
			reg.a = rotate_left(reg.a);
			reg.f &= ~0x80;
			cycles += 8;
			break;

		//JR, n
		case 0x18 :
			jr(mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//ADD HL, DE
		case 0x19 :
			reg.hl = add_word(reg.hl, reg.de);
			cycles += 8;
			break;

		//LD A, DE
		case 0x1A :
			reg.a = mem->read_u8(reg.de);
			cycles += 8;
			break;

		//DEC DE
		case 0x1B :
			reg.de--;
			cycles += 8;
			break;

		//INC E
		case 0x1C :
			reg.e = inc_byte(reg.e);
			cycles += 4;
			break;

		//DEC E
		case 0x1D :
			reg.e = dec_byte(reg.e);
			cycles += 4;
			break;

		//LD E, n
		case 0x1E :
			reg.e = mem->read_u8(reg.pc++);
			cycles += 8;
			break;

		//RR  A
		case 0x1F :
			reg.a = rotate_right(reg.a);
			reg.f &= ~0x80;
			cycles += 8;
			break;		

		//JR NZ, n
		case 0x20 :	
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				if(zero_flag == 0) { jr(mem->read_u8(reg.pc));}
				cycles += 8;
				reg.pc++;
			}
			break;

		//LD HL, nn
		case 0x21 :
			reg.hl = mem->read_u16(reg.pc);
			reg.pc+=2;
			cycles += 12;
			break;

		//LDI HL, A
		case 0x22 :
			mem->write_u8(reg.hl, reg.a);
			reg.hl++;
			cycles += 8;
			break;

		//INC HL
		case 0x23 :
			reg.hl++;
			cycles += 8;
			break;

		//INC H
		case 0x24 :
			reg.h = inc_byte(reg.h);
			cycles += 4;
			break;

		//DEC H
		case 0x25 :
			reg.h = dec_byte(reg.h);
			cycles += 4;
			break;

		//LD H, n
		case 0x26 : 
			reg.h = mem->read_u8(reg.pc++);
			cycles += 8;
			break;

		//DAA
		case 0x27 :
			reg.a = daa();
			cycles += 4;
			break;

		//JR Z, n
		case 0x28 :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				if(zero_flag == 1) { jr(mem->read_u8(reg.pc));}
				cycles += 8;
				reg.pc++;
			}
			break;

		//ADD HL, HL
		case 0x29 :
			reg.hl = add_word(reg.hl, reg.hl);
			cycles += 8;
			break;

		//LDI A, HL
		case 0x2A :
			reg.a = mem->read_u8(reg.hl);
			reg.hl++;
			cycles += 8;
			break;

		//DEC HL
		case 0x2B :
			reg.hl--;
			cycles += 8;
			break;

		//INC L
		case 0x2C :
			reg.l = inc_byte(reg.l);
			cycles += 4;
			break;

		//DEC L
		case 0x2D :
			reg.l = dec_byte(reg.l);
			cycles += 4;
			break;

		//LD L, n
		case 0x2E :
			reg.l = mem->read_u8(reg.pc++);
			cycles += 8;
			break;

		//CPL
		case 0x2F :
			reg.a = ~reg.a;
			reg.f |= 0x60;
			cycles += 4;
			break;

		//JR NC, n
		case 0x30 :	
			{
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				if(carry_flag == 0) { jr(mem->read_u8(reg.pc));}
				cycles += 8;
				reg.pc++;
			}
			break;

		//LD SP, nn
		case 0x31 : 
			reg.sp = mem->read_u16(reg.pc);
			reg.pc += 2;
			cycles += 12;
			break;

		//LDD HL, A
		case 0x32 :
			mem->write_u8(reg.hl--, reg.a);
			cycles += 8;
			break;

		//INC SP
		case 0x33 :
			reg.sp++;
			cycles += 8;
			break;

		//INC HL
		case 0x34 :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = inc_byte(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 12;
			break;

		//DEC HL
		case 0x35 :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = dec_byte(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 12;
			break;

		//LD HL, n
		case 0x36 :
			mem->write_u8(reg.hl, mem->read_u8(reg.pc++));
			cycles += 12;
			break;

		//SCF
		case 0x37 :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				reg.f = 0;
				if(zero_flag == 1) { reg.f |= 0x80; }
				reg.f |= 0x10;
				cycles += 4;
			}
			break;

		//JR C, n
		case 0x38 :
			{
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				if(carry_flag == 1) { jr(mem->read_u8(reg.pc));}
				cycles += 8;
				reg.pc++;
			}
			break;

		//ADD HL, SP
		case 0x39 :
			reg.hl = add_word(reg.hl, reg.sp);
			cycles += 8;
			break;

		//LDD A, HL
		case 0x3A :
			reg.a = mem->read_u8(reg.hl);
			reg.hl--;
			cycles += 8;
			break;

		//DEC SP
		case 0x3B :
			reg.sp--;
			cycles += 8;
			break;

		//INC A
		case 0x3C :
			reg.a = inc_byte(reg.a);
			cycles += 4;
			break;

		//DEC A
		case 0x3D :
			reg.a = dec_byte(reg.a);
			cycles += 4;
			break;

		//LD A, n
		case 0x3E :
			reg.a = mem->read_u8(reg.pc++);
			cycles += 8;
			break;

		//CCF
		case 0x3F :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				reg.f = 0;

				if(zero_flag == 1) { reg.f |= 0x80; }

				if(carry_flag == 0) { reg.f |= 0x10; }
				else { reg.f &= ~0x10; }
	
				cycles += 4;
			}
			break;

		//LD B, B
		case 0x40 :
			reg.b = reg.b;
			cycles += 4;
			break;

		//LD B, C
		case 0x41 :
			reg.b = reg.c;
			cycles += 4;
			break;

		//LD B, D
		case 0x42 :
			reg.b = reg.d;
			cycles += 4;
			break;

		//LD B, E
		case 0x43 :
			reg.b = reg.e;
			cycles += 4;
			break;

		//LD B, H
		case 0x44 :
			reg.b = reg.h;
			cycles += 4;
			break;

		//LD B, L
		case 0x45 :
			reg.b = reg.l;
			cycles += 4;
			break;
		
		//LD B, HL
		case 0x46 :
			reg.b = mem->read_u8(reg.hl);
			cycles += 8;
			break;

		//LD B, A
		case 0x47 :
			reg.b = reg.a;
			cycles += 4;
			break;

		//LD C, B
		case 0x48 :
			reg.c = reg.b;
			cycles += 4;
			break;

		//LD C, C
		case 0x49 :
			reg.c = reg.c;
			cycles += 4;
			break;

		//LD C, D
		case 0x4A :
			reg.c = reg.d;
			cycles += 4;
			break;

		//LD C, E
		case 0x4B :
			reg.c = reg.e;
			cycles += 4;
			break;
		
		//LD C, H
		case 0x4C :
			reg.c = reg.h;
			cycles += 4;
			break;

		//LD C, L
		case 0x4D :
			reg.c = reg.l;
			cycles += 4;
			break;

		//LD C, HL
		case 0x4E :
			reg.c = mem->read_u8(reg.hl);
			cycles += 8;
			break;

		//LD C, A
		case 0x4F :
			reg.c = reg.a;
			cycles += 4;
			break;

		//LD D, B
		case 0x50 :
			reg.d = reg.b;
			cycles += 4;
			break;

		//LD D, C
		case 0x51 :
			reg.d = reg.c;
			cycles += 4;
			break;

		//LD D, D
		case 0x52 :
			reg.d = reg.d;
			cycles += 4;
			break;

		//LD D, E
		case 0x53 :
			reg.d = reg.e;
			cycles += 4;
			break;

		//LD D, H
		case 0x54 :
			reg.d = reg.h;
			cycles += 4;
			break;

		//LD D, L
		case 0x55 : 
			reg.d = reg.l;
			cycles += 4;
			break;

		//LD D, HL
		case 0x56 :
			reg.d = mem->read_u8(reg.hl);
			cycles += 8;
			break;

		//LD D, A
		case 0x57 :
			reg.d = reg.a;
			cycles += 4;
			break;

		//LD E, B
		case 0x58 :
			reg.e = reg.b;
			cycles += 4;
			break;

		//LD E, C
		case 0x59 :
			reg.e = reg.c;
			cycles += 4;
			break;

		//LD E, D
		case 0x5A :
			reg.e = reg.d;
			cycles += 4;
			break;

		//LD E, E
		case 0x5B :
			reg.e = reg.e;
			cycles += 4;
			break;

		//LD E, H
		case 0x5C :
			reg.e = reg.h;
			cycles += 4;
			break;

		//LD E, L
		case 0x5D :
			reg.e = reg.l;
			cycles += 4;
			break;

		//LD E, HL
		case 0x5E :
			reg.e = mem->read_u8(reg.hl);
			cycles += 8;
			break;

		//LD E, A
		case 0x5F : 
			reg.e = reg.a;
			cycles += 4;
			break;

		//LD H, B
		case 0x60 :
			reg.h = reg.b;
			cycles += 4;
			break;

		//LD H, C
		case 0x61 :
			reg.h = reg.c;
			cycles += 4;
			break;

		//LD H, D
		case 0x62 :
			reg.h = reg.d;
			cycles += 4;
			break;

		//LD H, E
		case 0x63 :
			reg.h = reg.e;
			cycles += 4;
			break;

		//LD H, H
		case 0x64 :
			reg.h = reg.h;
			cycles += 4;
			break;

		//LD H, L
		case 0x65 :
			reg.h = reg.l;
			cycles += 4;
			break;

		//LD H, HL
		case 0x66 :
			reg.h = mem->read_u8(reg.hl);
			cycles += 8;
			break;

		//LD H, A
		case 0x67 :
			reg.h = reg.a;
			cycles += 4;
			break;

		//LD L, B
		case 0x68 :
			reg.l = reg.b;
			cycles += 4;
			break;

		//LD L, C
		case 0x69 :
			reg.l = reg.c;
			cycles += 4;
			break;

		//LD L, D
		case 0x6A :
			reg.l = reg.d;
			cycles += 4;
			break;

		//LD L, E
		case 0x6B :
			reg.l = reg.e;
			cycles += 4;
			break;

		//LD L, H
		case 0x6C :
			reg.l = reg.h;
			cycles += 4;
			break;

		//LD L, L
		case 0x6D :
			reg.l = reg.l;
			cycles += 4;
			break;

		//LD L, HL
		case 0x6E :
			reg.l = mem->read_u8(reg.hl);
			cycles += 8;
			break;

		//LD L, A 
		case 0x6F :
			reg.l = reg.a;
			cycles += 4;
			break;

		//LD HL, B
		case 0x70 :
			mem->write_u8(reg.hl, reg.b);
			cycles += 8;
			break;

		//LD HL, C
		case 0x71 :
			mem->write_u8(reg.hl, reg.c);
			cycles += 8;
			break;

		//LD HL, D
		case 0x72 :
			mem->write_u8(reg.hl, reg.d);
			cycles += 8;
			break;

		//LD HL, E
		case 0x73 :
			mem->write_u8(reg.hl, reg.e);
			cycles += 8;
			break;

		//LD HL, H
		case 0x74 :
			mem->write_u8(reg.hl, reg.h);
			cycles += 8;
			break;

		//LD HL, L
		case 0x75 :
			mem->write_u8(reg.hl, reg.l);
			cycles += 8;
			break;

		//HALT
		case 0x76 :
			halt = true;
			skip_instruction = ((mem->memory_map[IE_FLAG] & mem->memory_map[IF_FLAG] & 0x1F) && (!interrupt)) ? true : false;
			cycles += 4;
			break; 

		//LD HL, A
		case 0x77 :
			mem->write_u8(reg.hl, reg.a);
			cycles += 8;
			break;

		//LD A, B
		case 0x78 :
			reg.a = reg.b;
			cycles += 4;
			break;

		//LD A, C
		case 0x79 :
			reg.a = reg.c;
			cycles += 4;
			break;

		//LD A, D
		case 0x7A :
			reg.a = reg.d;
			cycles += 4;
			break;

		//LD A, E
		case 0x7B : 
			reg.a = reg.e;
			cycles += 4;
			break;

		//LD A, H
		case 0x7C :
			reg.a = reg.h;
			cycles += 4;
			break;

		//LD A, L
		case 0x7D :
			reg.a = reg.l;
			cycles += 4;
			break;

		//LD A, HL
		case 0x7E : 
			reg.a = mem->read_u8(reg.hl);
			cycles += 8;
			break;

		//LD A, A
		case 0x7F :
			reg.a = reg.a;
			cycles += 4;
			break;

		//ADD A, B
		case 0x80 :
			reg.a = add_byte(reg.a, reg.b);
			cycles += 4;
			break;

		//ADD A, C
		case 0x81 :
			reg.a = add_byte(reg.a, reg.c);
			cycles += 4;
			break;

		//ADD A, D
		case 0x82 :
			reg.a = add_byte(reg.a, reg.d);
			cycles += 4;
			break;

		//ADD A, E
		case 0x83 :
			reg.a = add_byte(reg.a, reg.e);
			cycles += 4;
			break;	

		//ADD A, H
		case 0x84 :
			reg.a = add_byte(reg.a, reg.h);
			cycles += 4;
			break;

		//ADD A, L
		case 0x85 :
			reg.a = add_byte(reg.a, reg.l);
			cycles += 4;
			break;

		//ADD A, HL
		case 0x86 :
			reg.a = add_byte(reg.a, mem->read_u8(reg.hl));
			cycles += 8;
			break;

		//ADD A, A
		case 0x87 :
			reg.a = add_byte(reg.a, reg.a);
			cycles += 4;
			break;

		//ADC A, B
		case 0x88 :
			reg.a = add_carry(reg.a, reg.b);
			cycles += 4;
			break;

		//ADC A, C
		case 0x89 :
			reg.a = add_carry(reg.a, reg.c);
			cycles += 4;
			break;

		//ADC A, D
		case 0x8A :
			reg.a = add_carry(reg.a, reg.d);
			cycles += 4;
			break;

		//ADC A, E
		case 0x8B :
			reg.a = add_carry(reg.a, reg.e);
			cycles += 4;
			break;

		//ADC A, H
		case 0x8C :
			reg.a = add_carry(reg.a, reg.h);
			cycles += 4;
			break;

		//ADC A, L
		case 0x8D :
			reg.a = add_carry(reg.a, reg.l);
			cycles += 4;
			break;

		//ADC A, HL
		case 0x8E :
			reg.a = add_carry(reg.a, mem->read_u8(reg.hl));
			cycles += 8;
			break;

		//ADC A, A
		case 0x8F :
			reg.a = add_carry(reg.a, reg.a);
			cycles += 4;
			break;

		//SUB A, B
		case 0x90 :
			reg.a = sub_byte(reg.a, reg.b);
			cycles += 4;
			break;

		//SUB A, C
		case 0x91 :
			reg.a = sub_byte(reg.a, reg.c);
			cycles += 4;
			break;

		//SUB A, D
		case 0x92 :
			reg.a = sub_byte(reg.a, reg.d);
			cycles += 4;
			break;

		//SUB A, E
		case 0x93 :
			reg.a = sub_byte(reg.a, reg.e);
			cycles += 4;
			break;

		//SUB A, H
		case 0x94 :
			reg.a = sub_byte(reg.a, reg.h);
			cycles += 4;
			break;

		//SUB A, L
		case 0x95 :
			reg.a = sub_byte(reg.a, reg.l);
			cycles += 4;
			break;

		//SUB A, HL
		case 0x96 :
			reg.a = sub_byte(reg.a, mem->read_u8(reg.hl));
			cycles += 8;
			break;

		//SUB A, A
		case 0x97 :
			reg.a = sub_byte(reg.a, reg.a);
			cycles += 4;
			break;

		//SBC A, B
		case 0x98 : 
			reg.a = sub_carry(reg.a, reg.b);
			cycles += 4;
			break;

		//SBC A, C
		case 0x99 :
			reg.a = sub_carry(reg.a, reg.c);
			cycles += 4;
			break;

		//SBC A, D
		case 0x9A :
			reg.a = sub_carry(reg.a, reg.d);
			cycles += 4;
			break;

		//SBC A, E
		case 0x9B :
			reg.a = sub_carry(reg.a, reg.e);
			cycles += 4;
			break;

		//SBC A, H
		case 0x9C : 
			reg.a = sub_carry(reg.a, reg.h);
			cycles += 4;
			break;

		//SBC A, L
		case 0x9D :
			reg.a = sub_carry(reg.a, reg.l);
			cycles += 4;
			break;

		//SBC A, HL
		case 0x9E :
			reg.a = sub_carry(reg.a, mem->read_u8(reg.hl));
			cycles += 8;
			break;

		//SBC A, A
		case 0x9F : 
			reg.a = sub_carry(reg.a, reg.a);
			cycles += 4;
			break; 

		//AND B
		case 0xA0 :
			reg.a = and_byte(reg.a, reg.b);
			cycles += 4;
			break;

		//AND C
		case 0xA1 :
			reg.a = and_byte(reg.a, reg.c);
			cycles += 4;
			break;

		//AND D
		case 0xA2 :
			reg.a = and_byte(reg.a, reg.d);
			cycles += 4;
			break;

		//AND E
		case 0xA3 :
			reg.a = and_byte(reg.a, reg.e);
			cycles += 4;
			break;

		//AND H
		case 0xA4 :
			reg.a = and_byte(reg.a, reg.h);
			cycles += 4;
			break;

		//AND L
		case 0xA5 :
			reg.a = and_byte(reg.a, reg.l);
			cycles += 4;
			break;

		//AND HL
		case 0xA6 :
			reg.a = and_byte(reg.a, mem->read_u8(reg.hl));
			cycles += 8;
			break;

		//AND A
		case 0xA7 :
			reg.a = and_byte(reg.a, reg.a);
			cycles += 4;
			break;

		//XOR B
		case 0xA8 :
			reg.a = xor_byte(reg.a, reg.b);
			cycles += 4;
			break;

		//XOR C
		case 0xA9 :
			reg.a = xor_byte(reg.a, reg.c);
			cycles += 4;
			break;

		//XOR D
		case 0xAA :
			reg.a = xor_byte(reg.a, reg.d);
			cycles += 4;
			break;

		//XOR E
		case 0xAB :
			reg.a = xor_byte(reg.a, reg.e);
			cycles += 4;
			break;

		//XOR H
		case 0xAC :
			reg.a = xor_byte(reg.a, reg.h);
			cycles += 4;
			break;

		//XOR L
		case 0xAD :
			reg.a = xor_byte(reg.a, reg.l);
			cycles += 4;
			break;

		//XOR HL
		case 0xAE :
			reg.a = xor_byte(reg.a, mem->read_u8(reg.hl));
			cycles += 8;
			break;

		//XOR A
		case 0xAF :
			reg.a = xor_byte(reg.a, reg.a);
			cycles += 4;
			break;

		//OR B
		case 0xB0 :
			reg.a = or_byte(reg.a, reg.b);
			cycles += 4;
			break;

		//OR C
		case 0xB1 :
			reg.a = or_byte(reg.a, reg.c);
			cycles += 4;
			break;

		//OR D
		case 0xB2 :
			reg.a = or_byte(reg.a, reg.d);
			cycles += 4;
			break;

		//OR E
		case 0xB3 :
			reg.a = or_byte(reg.a, reg.e);
			cycles += 4;
			break;

		//OR H
		case 0xB4 :
			reg.a = or_byte(reg.a, reg.h);
			cycles += 4;
			break;

		//OR L
		case 0xB5 :
			reg.a = or_byte(reg.a, reg.l);
			cycles += 4;
			break;

		//OR HL
		case 0xB6 :
			reg.a = or_byte(reg.a, mem->read_u8(reg.hl));
			cycles += 8;
			break;

		//OR A
		case 0xB7 :
			reg.a = or_byte(reg.a, reg.a);
			cycles += 4;
			break;

		//CP B
		case 0xB8 :
			sub_byte(reg.a, reg.b);
			cycles += 4;
			break;

		//CP C
		case 0xB9 :
			sub_byte(reg.a, reg.c);
			cycles += 4;
			break;

		//CP D
		case 0xBA :
			sub_byte(reg.a, reg.d);
			cycles += 4;
			break;

		//CP E
		case 0xBB :
			sub_byte(reg.a, reg.e);
			cycles += 4;
			break;

		//CP H
		case 0xBC :
			sub_byte(reg.a, reg.h);
			cycles += 4;
			break;

		//CP L
		case 0xBD :
			sub_byte(reg.a, reg.l);
			cycles += 4;
			break;

		//CP HL
		case 0xBE :
			sub_byte(reg.a, mem->read_u8(reg.hl));
			cycles += 8;
			break;

		//CP A
		case 0xBF :
			sub_byte(reg.a, reg.a);
			cycles += 4;
			break;

		//RET NZ
		case 0xC0 :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				if(zero_flag == 0) { reg.pc = mem->read_u16(reg.sp); reg.sp += 2; }
				cycles += 8;
			}
			break;

		//POP BC
		case 0xC1 :
			reg.c = mem->read_u8(reg.sp++);
			reg.b = mem->read_u8(reg.sp++);
			cycles += 12;
			break;

		//JP NZ nn
		case 0xC2 :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				if(zero_flag == 0) { reg.pc = mem->read_u16(reg.pc); }
				else { reg.pc += 2; }
				cycles += 12; 
			}
			break;

		//JP nn
		case 0xC3 :
			reg.pc = mem->read_u16(reg.pc);
			cycles += 12;
			break;

		//CALL NZ, nn
		case 0xC4 :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				
				if(zero_flag == 0) 
				{
					reg.sp -= 2;
					mem->write_u16(reg.sp, reg.pc+2);
					reg.pc = mem->read_u16(reg.pc);
				}
				
				else { reg.pc += 2; }
				cycles += 12;
			}
			break;

		//PUSH BC
		case 0xC5 :
			mem->write_u8(--reg.sp, reg.b);
			mem->write_u8(--reg.sp, reg.c);
			cycles += 16;
			break;

		//ADD A, n
		case 0xC6 :
			reg.a = add_byte(reg.a, mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//RST 0
		case 0xC7 :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x00;
			cycles += 32;
			break;

		//RET Z
		case 0xC8 :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				if(zero_flag == 1) { reg.pc = mem->read_u16(reg.sp); reg.sp += 2;} 
				cycles += 8;
			}
			break;

		//RET
		case 0xC9 :
			reg.pc = mem->read_u16(reg.sp);
			reg.sp += 2;
			cycles += 8;
			break;

		//JP Z nn
		case 0xCA :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				if(zero_flag == 1) { reg.pc = mem->read_u16(reg.pc); }
				else { reg.pc += 2; }
				cycles += 12;
			}
			break;

		//EXT OPS
		case 0xCB :
			temp_word = 0xCB00;
			temp_word |= mem->read_u8(reg.pc++);
			exec_op(temp_word);
			break;

		//CALL Z, nn
		case 0xCC :
			{
				u8 zero_flag = (reg.f & 0x80) ? 1 : 0;
				
				if(zero_flag == 1) 
				{
					reg.sp -= 2;
					mem->write_u16(reg.sp, reg.pc+2);
					reg.pc = mem->read_u16(reg.pc);
				}
				
				else { reg.pc += 2; }
				cycles += 12;
			}
			break;

		//CALL nn
		case 0xCD :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc+2);
			reg.pc = mem->read_u16(reg.pc);
			cycles += 12;
			break;

		//ADC A, n
		case 0xCE :
			reg.a = add_carry(reg.a, mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//RST 8
		case 0xCF :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x08;
			cycles += 32;
			break;

		//RET NC
		case 0xD0 :
			{
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				if(carry_flag == 0) { reg.pc = mem->read_u16(reg.sp); reg.sp += 2; }
				cycles += 8;
			}
			break;

		//POP DE
		case 0xD1 :
			reg.e = mem->read_u8(reg.sp++);
			reg.d = mem->read_u8(reg.sp++);
			cycles += 12;
			break;

		//JP NC nn
		case 0xD2 :
			{
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				if(carry_flag == 0) { reg.pc = mem->read_u16(reg.pc); }
				else { reg.pc += 2; }
				cycles += 12; 
			}
			break;

		//CALL NC nn
		case 0xD4 :
			{
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				
				if(carry_flag == 0) 
				{
					reg.sp -= 2;
					mem->write_u16(reg.sp, reg.pc+2);
					reg.pc = mem->read_u16(reg.pc);
				}
				
				else { reg.pc += 2; }
				cycles += 12;
			}
			break;
				
		//PUSH DE
		case 0xD5 :
			mem->write_u8(--reg.sp, reg.d);
			mem->write_u8(--reg.sp, reg.e);
			cycles += 16;
			break;

		//SUB A, n
		case 0xD6 :
			reg.a = sub_byte(reg.a, mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//RST 10
		case 0xD7 :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x10;
			cycles += 32;
			break;

		//RET C
		case 0xD8 :
			{
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				if(carry_flag == 1) { reg.pc = mem->read_u16(reg.sp); reg.sp += 2;} 
				cycles += 8;
			}
			break;

		//RETI
		case 0xD9 :
			reg.pc = mem->read_u16(reg.sp);
			reg.sp += 2;
			cycles += 8;
			interrupt = true;
			break;

		//JP C nn
		case 0xDA :
			{
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				if(carry_flag == 1) { reg.pc = mem->read_u16(reg.pc); }
				else { reg.pc += 2; }
				cycles += 12;
			}
			break;

		//CALL C, nn
		case 0xDC :
			{
				u8 carry_flag = (reg.f & 0x10) ? 1 : 0;
				
				if(carry_flag == 1) 
				{
					reg.sp -= 2;
					mem->write_u16(reg.sp, reg.pc+2);
					reg.pc = mem->read_u16(reg.pc);
				}
				
				else { reg.pc += 2; }
				cycles += 12;
			}
			break;

		//SBC A, n
		case 0xDE :
			reg.a = sub_carry(reg.a, mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//RST 18
		case 0xDF :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x18;
			cycles += 32;
			break;

		//LDH n, A
		case 0xE0 :
			temp_word = (mem->read_u8(reg.pc++) | 0xFF00);
			mem->write_u8(temp_word, reg.a);
			cycles += 12;
			break;

		//POP HL
		case 0xE1 :
			reg.l = mem->read_u8(reg.sp++);
			reg.h = mem->read_u8(reg.sp++);
			cycles += 12;
			break;

		//LDH C, A
		case 0xE2 : 
			temp_word = (reg.c | 0xFF00);
			mem->write_u8(temp_word, reg.a);
			cycles += 8;
			break;

		//PUSH HL
		case 0xE5 :
			mem->write_u8(--reg.sp, reg.h);
			mem->write_u8(--reg.sp, reg.l);
			cycles += 16;
			break;

		//AND n
		case 0xE6 :
			reg.a = and_byte(reg.a, mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//RST 20
		case 0xE7 :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x20;
			cycles += 32;
			break;

		//ADD SP, n
		case 0xE8 :
			reg.sp = add_signed_byte(reg.sp, mem->read_s8(reg.pc++));
			cycles += 16;
			break;

		//JP HL
		case 0xE9 :
			reg.pc = reg.hl;
			cycles += 4;
			break;

		//LD nn, A
		case 0xEA :
			mem->write_u8(mem->read_u16(reg.pc), reg.a);
			reg.pc += 2;
			cycles += 16;
			break;

		//XOR n
		case 0xEE :
			reg.a = xor_byte(reg.a, mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//RST 28
		case 0xEF :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x28;
			cycles += 32;
			break;

		//LDH A, n
		case 0xF0 :
			temp_word = (mem->read_u8(reg.pc++) | 0xFF00);
			reg.a = mem->read_u8(temp_word);
			cycles += 12;
			break;

		//POP AF
		case 0xF1 :
			reg.f = mem->read_u8(reg.sp++) & 0xF0;
			reg.a = mem->read_u8(reg.sp++);
			cycles += 12;
			break;

		//DI
		case 0xF3 :
			interrupt = false;
			cycles += 4;
			break;

		//LDH A, C
		case 0xF2 :
			reg.a = mem->read_u8(0xFF00 | reg.c);
			cycles += 8;
			break;

		//PUSH AF
		case 0xF5 :
			mem->write_u8(--reg.sp, reg.a);
			mem->write_u8(--reg.sp, reg.f);
			cycles += 16;
			break;

		//OR n
		case 0xF6 :
			reg.a = or_byte(reg.a, mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//RST 30
		case 0xF7 :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x30;
			cycles += 32;
			break;

		//LDHL SP, n
		case 0xF8 :
			reg.hl = add_signed_byte(reg.sp, mem->read_s8(reg.pc++)); 
			cycles += 12;
			break;

		//LD SP, HL
		case 0xF9 :
			reg.sp = reg.hl;
			cycles += 8;
			break;

		//LD A, nn
		case 0xFA :
			reg.a = mem->read_u8(mem->read_u16(reg.pc));
			reg.pc+=2;
			cycles += 16;
			break;

		//EI
		case 0xFB :
			interrupt_delay = true;
			cycles += 4;
			break;

		//CP n
		case 0xFE : 
			sub_byte(reg.a, mem->read_u8(reg.pc++));
			cycles += 8;
			break;

		//RST 38
		case 0xFF :
			reg.sp -= 2;
			mem->write_u16(reg.sp, reg.pc);
			reg.pc = 0x38;
			cycles += 32;
			break;
			
		default :
			std::cout<<"CPU::Error - Unknown Opcode : 0x" << std::hex << (int) opcode << "\n";
			running = false;
	}
}

/****** Execute 16-bit opcodes ******/
void Z80::exec_op(u16 opcode)
{
	switch (opcode)
	{
		//RLC B
		case 0xCB00 :
			reg.b = rotate_left_carry(reg.b);
			cycles += 8;
			break;

		//RLC C
		case 0xCB01 :
			reg.c = rotate_left_carry(reg.c);
			cycles += 8;
			break;

		//RLC D
		case 0xCB02 :
			reg.d = rotate_left_carry(reg.d);
			cycles += 8;
			break;

		//RLC E
		case 0xCB03 :
			reg.e = rotate_left_carry(reg.e);
			cycles += 8;
			break;

		//RLC H
		case 0xCB04 :
			reg.h = rotate_left_carry(reg.h);
			cycles += 8;
			break;

		//RLC L
		case 0xCB05 :
			reg.l = rotate_left_carry(reg.l);
			cycles += 8;
			break;

		//RLC HL
		case 0xCB06 :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = rotate_left_carry(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RLC A
		case 0xCB07 :
			reg.a = rotate_left_carry(reg.a);
			cycles += 8;
			break;

		//RRC B
		case 0xCB08 :
			reg.b = rotate_right_carry(reg.b);
			cycles += 8;
			break;

		//RRC C
		case 0xCB09 :
			reg.c = rotate_right_carry(reg.c);
			cycles += 8;
			break;

		//RRC D
		case 0xCB0A :
			reg.d = rotate_right_carry(reg.d);
			cycles += 8;
			break;

		//RRC E
		case 0xCB0B :
			reg.e = rotate_right_carry(reg.e);
			cycles += 8;
			break;

		//RRC H
		case 0xCB0C :
			reg.h = rotate_right_carry(reg.h);
			cycles += 8;
			break;

		//RRC L
		case 0xCB0D :
			reg.l = rotate_right_carry(reg.l);
			cycles += 8;
			break;
	
		//RRC HL
		case 0xCB0E :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = rotate_right_carry(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RRC A
		case 0xCB0F :
			reg.a = rotate_right_carry(reg.a);
			cycles += 8;
			break;

		//RL B
		case 0xCB10 :
			reg.b = rotate_left(reg.b);
			cycles += 8;
			break;

		//RL C
		case 0xCB11 :
			reg.c = rotate_left(reg.c);
			cycles += 8;
			break;

		//RL D
		case 0xCB12 :
			reg.d = rotate_left(reg.d);
			cycles += 8;
			break;

		//RL E
		case 0xCB13 :
			reg.e = rotate_left(reg.e);
			cycles += 8;
			break;

		//RL H
		case 0xCB14 :
			reg.h = rotate_left(reg.h);
			cycles += 8;
			break;

		//RL L
		case 0xCB15 :
			reg.l = rotate_left(reg.l);
			cycles += 8;
			break;

		//RL HL
		case 0xCB16 :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = rotate_left(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RL A
		case 0xCB17 :
			reg.a = rotate_left(reg.a);
			cycles += 8;
			break;

		//RR B
		case 0xCB18 :
			reg.b = rotate_right(reg.b);
			cycles += 8;
			break;

		//RR C
		case 0xCB19 :
			reg.c = rotate_right(reg.c);
			cycles += 8;
			break;

		//RR D
		case 0xCB1A :
			reg.d = rotate_right(reg.d);
			cycles += 8;
			break;

		//RR E
		case 0xCB1B :
			reg.e = rotate_right(reg.e);
			cycles += 8;
			break;

		//RR H
		case 0xCB1C :
			reg.h = rotate_right(reg.h);
			cycles += 8;
			break;

		//RR L
		case 0xCB1D :
			reg.l = rotate_right(reg.l);
			cycles += 8;
			break;

		//RR HL
		case 0xCB1E :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = rotate_right(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RR A
		case 0xCB1F :
			reg.a = rotate_right(reg.a);
			cycles += 8;
			break;

		//SLA B
		case 0xCB20 :
			reg.b = sla(reg.b);
			cycles += 8;
			break;

		//SLA C
		case 0xCB21 :
			reg.c = sla(reg.c);
			cycles += 8;
			break;

		//SLA D
		case 0xCB22 :
			reg.d = sla(reg.d);
			cycles += 8;
			break;

		//SLA E
		case 0xCB23 :
			reg.e = sla(reg.e);
			cycles += 8;
			break;

		//SLA H
		case 0xCB24 :
			reg.h = sla(reg.h);
			cycles += 8;
			break;

		//SLA L
		case 0xCB25 :
			reg.l = sla(reg.l);
			cycles += 8;
			break;

		//SLA HL
		case 0xCB26 :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = sla(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;
		
		//SLA  A
		case 0xCB27 :
			reg.a = sla(reg.a);
			cycles += 8;
			break;

		//SRA B
		case 0xCB28 :
			reg.b = sra(reg.b);
			cycles += 8;
			break;

		//SRA C
		case 0xCB29 :
			reg.c = sra(reg.c);
			cycles += 8;
			break;

		//SRA D
		case 0xCB2A :
			reg.d = sra(reg.d);
			cycles += 8;
			break;
		
		//SRA E
		case 0xCB2B :
			reg.e = sra(reg.e);
			cycles += 8;
			break;

		//SRA H
		case 0xCB2C :
			reg.h = sra(reg.h);
			cycles += 8;
			break;

		//SRA L
		case 0xCB2D :
			reg.l = sra(reg.l);
			cycles += 8;
			break;

		//SRA HL
		case 0xCB2E :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = sra(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SRA A
		case 0xCB2F :
			reg.a = sra(reg.a);
			cycles += 8;
			break;

		//SWAP B
		case 0xCB30 :
			reg.b = swap(reg.b);
			cycles += 8;
			break;

		//SWAP C
		case 0xCB31 :
			reg.c = swap(reg.c);
			cycles += 8;
			break;

		//SWAP D
		case 0xCB32 :
			reg.d = swap(reg.d);
			cycles += 8;
			break;

		//SWAP E
		case 0xCB33 :
			reg.e = swap(reg.e);
			cycles += 8;
			break;

		//SWAP H
		case 0xCB34 :
			reg.h = swap(reg.h);
			cycles += 8;
			break;

		//SWAP L
		case 0xCB35 :
			reg.l = swap(reg.l);
			cycles += 8;
			break;

		//SWAP HL
		case 0xCB36 :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = swap(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SWAP A
		case 0xCB37 :
			reg.a = swap(reg.a);
			cycles += 8;
			break;

		//SRL B
		case 0xCB38 :
			reg.b = srl(reg.b);
			cycles += 8;
			break;

		//SRL C
		case 0xCB39 :
			reg.c = srl(reg.c);
			cycles += 8;
			break;

		//SRL D
		case 0xCB3A :
			reg.d = srl(reg.d);
			cycles += 8;
			break;

		//SRL E
		case 0xCB3B :
			reg.e = srl(reg.e);
			cycles += 8;
			break;

		//SRL H
		case 0xCB3C :
			reg.h = srl(reg.h);
			cycles += 8;
			break;

		//SRL L
		case 0xCB3D :
			reg.l = srl(reg.l);
			cycles += 8;
			break;

		//SRL HL
		case 0xCB3E :
			temp_byte = mem->read_u8(reg.hl);
			temp_byte = srl(temp_byte);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SRL A
		case 0xCB3F :
			reg.a = srl(reg.a);
			cycles += 8;
			break;

		//BIT 0, B
		case 0xCB40 :
			bit(reg.b, 0x01);
			cycles += 8;
			break;

		//BIT 0, C
		case 0xCB41 :
			bit(reg.c, 0x01);
			cycles += 8;
			break;

		//BIT 0, D
		case 0xCB42 :
			bit(reg.d, 0x01);
			cycles += 8;
			break;

		//BIT 0, E
		case 0xCB43 :
			bit(reg.e, 0x01);
			cycles += 8;
			break;

		//BIT 0, H
		case 0xCB44 :
			bit(reg.h, 0x01);
			cycles += 8;
			break;

		//BIT 0, L
		case 0xCB45 :
			bit(reg.l, 0x01);
			cycles += 8;
			break;

		//BIT 0, HL
		case 0xCB46 :
			bit(mem->read_u8(reg.hl), 0x01);
			cycles += 16;
			break;

		//BIT 0, A
		case 0xCB47 :
			bit(reg.a, 0x01);
			cycles += 8;
			break;

		//BIT 1, B
		case 0xCB48 :
			bit(reg.b, 0x02);
			cycles += 8;
			break;

		//BIT 1, C
		case 0xCB49 :
			bit(reg.c, 0x02);
			cycles += 8;
			break;

		//BIT 1, D
		case 0xCB4A :
			bit(reg.d, 0x02);
			cycles += 8;
			break;

		//BIT 1, E
		case 0xCB4B :
			bit(reg.e, 0x02);
			cycles += 8;
			break;

		//BIT 1, H
		case 0xCB4C :
			bit(reg.h, 0x02);
			cycles += 8;
			break;

		//BIT 1, L
		case 0xCB4D :
			bit(reg.l, 0x02);
			cycles += 8;
			break;

		//BIT 1, HL
		case 0xCB4E :
			bit(mem->read_u8(reg.hl), 0x02);
			cycles += 16;
			break;

		//BIT 1, A
		case 0xCB4F :
			bit(reg.a, 0x02);
			cycles += 8;
			break;

		//BIT 2, B
		case 0xCB50 :
			bit(reg.b, 0x04);
			cycles += 8;
			break;

		//BIT 2, C
		case 0xCB51 :
			bit(reg.c, 0x04);
			cycles += 8;
			break;

		//BIT 2, D
		case 0xCB52 :
			bit(reg.d, 0x04);
			cycles += 8;
			break;

		//BIT 2, E
		case 0xCB53 :
			bit(reg.e, 0x04);
			cycles += 8;
			break;

		//BIT 2, H
		case 0xCB54 :
			bit(reg.h, 0x04);
			cycles += 8;
			break;

		//BIT 2, L
		case 0xCB55 :
			bit(reg.l, 0x04);
			cycles += 8;
			break;

		//BIT 2, HL
		case 0xCB56 :
			bit(mem->read_u8(reg.hl), 0x04);
			cycles += 16;
			break;

		//BIT 2, A
		case 0xCB57 :
			bit(reg.a, 0x04);
			cycles += 8;
			break;

		//BIT 3, B
		case 0xCB58 :
			bit(reg.b, 0x08);
			cycles += 8;
			break;

		//BIT 3, C
		case 0xCB59 :
			bit(reg.c, 0x08);
			cycles += 8;
			break;

		//BIT 3, D
		case 0xCB5A :
			bit(reg.d, 0x08);
			cycles += 8;
			break;

		//BIT 3, E
		case 0xCB5B :
			bit(reg.e, 0x08);
			cycles += 8;
			break;

		//BIT 3, H
		case 0xCB5C :
			bit(reg.h, 0x08);
			cycles += 8;
			break;

		//BIT 3, L
		case 0xCB5D :
			bit(reg.l, 0x08);
			cycles += 8;
			break;

		//BIT 3, HL
		case 0xCB5E :
			bit(mem->read_u8(reg.hl), 0x08);
			cycles += 16;
			break;

		//BIT 3, A
		case 0xCB5F :
			bit(reg.a, 0x08);
			cycles += 8;
			break;

		//BIT 4, B
		case 0xCB60 :
			bit(reg.b, 0x10);
			cycles += 8;
			break;

		//BIT 4, C
		case 0xCB61 :
			bit(reg.c, 0x10);
			cycles += 8;
			break;

		//BIT 4, D
		case 0xCB62 :
			bit(reg.d, 0x10);
			cycles += 8;
			break;

		//BIT 4, E
		case 0xCB63 :
			bit(reg.e, 0x10);
			cycles += 8;
			break;

		//BIT 4, H
		case 0xCB64 :
			bit(reg.h, 0x10);
			cycles += 8;
			break;

		//BIT 4, L
		case 0xCB65 :
			bit(reg.l, 0x10);
			cycles += 8;
			break;

		//BIT 4, HL
		case 0xCB66 :
			bit(mem->read_u8(reg.hl), 0x10);
			cycles += 16;
			break;

		//BIT 4, A
		case 0xCB67 :
			bit(reg.a, 0x10);
			cycles += 8;
			break;

		//BIT 5, B
		case 0xCB68 :
			bit(reg.b, 0x20);
			cycles += 8;
			break;

		//BIT 5, C
		case 0xCB69 :
			bit(reg.c, 0x20);
			cycles += 8;
			break;

		//BIT 5, D
		case 0xCB6A :
			bit(reg.d, 0x20);
			cycles += 8;
			break;

		//BIT 5, E
		case 0xCB6B :
			bit(reg.e, 0x20);
			cycles += 8;
			break;

		//BIT 5, H
		case 0xCB6C :
			bit(reg.h, 0x20);
			cycles += 8;
			break;

		//BIT 5, L
		case 0xCB6D :
			bit(reg.l, 0x20);
			cycles += 8;
			break;

		//BIT 5, HL
		case 0xCB6E :
			bit(mem->read_u8(reg.hl), 0x20);
			cycles += 16;
			break;

		//BIT 5, A
		case 0xCB6F :
			bit(reg.a, 0x20);
			cycles += 8;
			break;

		//BIT 6, B
		case 0xCB70 :
			bit(reg.b, 0x40);
			cycles += 8;
			break;

		//BIT 6, C
		case 0xCB71 :
			bit(reg.c, 0x40);
			cycles += 8;
			break;

		//BIT 6, D
		case 0xCB72 :
			bit(reg.d, 0x40);
			cycles += 8;
			break;

		//BIT 6, E
		case 0xCB73 :
			bit(reg.e, 0x40);
			cycles += 8;
			break;

		//BIT 6, H
		case 0xCB74 :
			bit(reg.h, 0x40);
			cycles += 8;
			break;

		//BIT 6, L
		case 0xCB75 :
			bit(reg.l, 0x40);
			cycles += 8;
			break;

		//BIT 6, HL
		case 0xCB76 :
			bit(mem->read_u8(reg.hl), 0x40);
			cycles += 16;
			break;

		//BIT 6, A
		case 0xCB77 :
			bit(reg.a, 0x40);
			cycles += 8;
			break;

		//BIT 7, B
		case 0xCB78 :
			bit(reg.b, 0x80);
			cycles += 8;
			break;

		//BIT 7, C
		case 0xCB79 :
			bit(reg.c, 0x80);
			cycles += 8;
			break;

		//BIT 7, D
		case 0xCB7A :
			bit(reg.d, 0x80);
			cycles += 8;
			break;

		//BIT 7, E
		case 0xCB7B :
			bit(reg.e, 0x80);
			cycles += 8;
			break;

		//BIT 7, H
		case 0xCB7C :
			bit(reg.h, 0x80);
			cycles += 8;
			break;

		//BIT 7, L
		case 0xCB7D :
			bit(reg.l, 0x80);
			cycles += 8;
			break;

		//BIT 7, HL
		case 0xCB7E :
			bit(mem->read_u8(reg.hl), 0x80);
			cycles += 16;
			break;

		//BIT 7, A
		case 0xCB7F :
			bit(reg.a, 0x80);
			cycles += 8;
			break;

		//RES 0, B
		case 0xCB80 :
			reg.b = res(reg.b, 0x01);
			cycles += 8;
			break;

		//RES 0, C
		case 0xCB81 :
			reg.c = res(reg.c, 0x01);
			cycles += 8;
			break;

		//RES 0, D
		case 0xCB82 :
			reg.d = res(reg.d, 0x01);
			cycles += 8;
			break;

		//RES 0, E
		case 0xCB83 :
			reg.e = res(reg.e, 0x01);
			cycles += 8;
			break;

		//RES 0, H
		case 0xCB84 :
			reg.h = res(reg.h, 0x01);
			cycles += 8;
			break;

		//RES 0, L
		case 0xCB85 :
			reg.l = res(reg.l, 0x01);
			cycles += 8;
			break;

		//RES 0, HL
		case 0xCB86 :
			temp_byte = res(mem->read_u8(reg.hl), 0x01);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RES 0, A
		case 0xCB87 :
			reg.a = res(reg.a, 0x01);
			cycles += 8;
			break;

		//RES 1, B
		case 0xCB88 :
			reg.b = res(reg.b, 0x02);
			cycles += 8;
			break;

		//RES 1, C
		case 0xCB89 :
			reg.c = res(reg.c, 0x02);
			cycles += 8;
			break;

		//RES 1, D
		case 0xCB8A :
			reg.d = res(reg.d, 0x02);
			cycles += 8;
			break;

		//RES 1, E
		case 0xCB8B :
			reg.e = res(reg.e, 0x02);
			cycles += 8;
			break;

		//RES 1, H
		case 0xCB8C :
			reg.h = res(reg.h, 0x02);
			cycles += 8;
			break;

		//RES 1, L
		case 0xCB8D :
			reg.l = res(reg.l, 0x02);
			cycles += 8;
			break;

		//RES 1, HL
		case 0xCB8E :
			temp_byte = res(mem->read_u8(reg.hl), 0x02);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RES 1, A
		case 0xCB8F :
			reg.a = res(reg.a, 0x02);
			cycles += 8;
			break;

		//RES 2, B
		case 0xCB90 :
			reg.b = res(reg.b, 0x04);
			cycles += 8;
			break;

		//RES 2, C
		case 0xCB91 :
			reg.c = res(reg.c, 0x04);
			cycles += 8;
			break;

		//RES 2, D
		case 0xCB92 :
			reg.d = res(reg.d, 0x04);
			cycles += 8;
			break;

		//RES 2, E
		case 0xCB93 :
			reg.e = res(reg.e, 0x04);
			cycles += 8;
			break;

		//RES 2, H
		case 0xCB94 :
			reg.h = res(reg.h, 0x04);
			cycles += 8;
			break;

		//RES 2, L
		case 0xCB95 :
			reg.l = res(reg.l, 0x04);
			cycles += 8;
			break;

		//RES 2, HL
		case 0xCB96 :
			temp_byte = res(mem->read_u8(reg.hl), 0x04);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RES 2, A
		case 0xCB97 :
			reg.a = res(reg.a, 0x04);
			cycles += 8;
			break;

		//RES 3, B
		case 0xCB98 :
			reg.b = res(reg.b, 0x08);
			cycles += 8;
			break;

		//RES 3, C
		case 0xCB99 :
			reg.c = res(reg.c, 0x08);
			cycles += 8;
			break;

		//RES 3, D
		case 0xCB9A :
			reg.d = res(reg.d, 0x08);
			cycles += 8;
			break;

		//RES 3, E
		case 0xCB9B :
			reg.e = res(reg.e, 0x08);
			cycles += 8;
			break;

		//RES 3, H
		case 0xCB9C :
			reg.h = res(reg.h, 0x08);
			cycles += 8;
			break;

		//RES 3, L
		case 0xCB9D :
			reg.l = res(reg.l, 0x08);
			cycles += 8;
			break;

		//RES 3, HL
		case 0xCB9E :
			temp_byte = res(mem->read_u8(reg.hl), 0x08);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RES 3, A
		case 0xCB9F :
			reg.a = res(reg.a, 0x08);
			cycles += 8;
			break;

		//RES 4, B
		case 0xCBA0 :
			reg.b = res(reg.b, 0x10);
			cycles += 8;
			break;

		//RES 4, C
		case 0xCBA1 :
			reg.c = res(reg.c, 0x10);
			cycles += 8;
			break;

		//RES 4, D
		case 0xCBA2 :
			reg.d = res(reg.d, 0x10);
			cycles += 8;
			break;

		//RES 4, E
		case 0xCBA3 :
			reg.e = res(reg.e, 0x10);
			cycles += 8;
			break;

		//RES 4, H
		case 0xCBA4 :
			reg.h = res(reg.h, 0x10);
			cycles += 8;
			break;

		//RES 4, L
		case 0xCBA5 :
			reg.l = res(reg.l, 0x10);
			cycles += 8;
			break;

		//RES 4, HL
		case 0xCBA6 :
			temp_byte = res(mem->read_u8(reg.hl), 0x10);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RES 4, A
		case 0xCBA7 :
			reg.a = res(reg.a, 0x10);
			cycles += 8;
			break;

		//RES 5, B
		case 0xCBA8 :
			reg.b = res(reg.b, 0x20);
			cycles += 8;
			break;

		//RES 5, C
		case 0xCBA9 :
			reg.c = res(reg.c, 0x20);
			cycles += 8;
			break;

		//RES 5, D
		case 0xCBAA :
			reg.d = res(reg.d, 0x20);
			cycles += 8;
			break;

		//RES 5, E
		case 0xCBAB :
			reg.e = res(reg.e, 0x20);
			cycles += 8;
			break;

		//RES 5, H
		case 0xCBAC :
			reg.h = res(reg.h, 0x20);
			cycles += 8;
			break;

		//RES 5, L
		case 0xCBAD :
			reg.l = res(reg.l, 0x20);
			cycles += 8;
			break;

		//RES 5, HL
		case 0xCBAE :
			temp_byte = res(mem->read_u8(reg.hl), 0x20);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 8;
			break;

		//RES 5, A
		case 0xCBAF :
			reg.a = res(reg.a, 0x20);
			cycles += 8;
			break;

		//RES 6, B
		case 0xCBB0 :
			reg.b = res(reg.b, 0x40);
			cycles += 8;
			break;

		//RES 6, C
		case 0xCBB1 :
			reg.c = res(reg.c, 0x40);
			cycles += 8;
			break;

		//RES 6, D
		case 0xCBB2 :
			reg.d = res(reg.d, 0x40);
			cycles += 8;
			break;

		//RES 6, E
		case 0xCBB3 :
			reg.e = res(reg.e, 0x40);
			cycles += 8;
			break;

		//RES 6, H
		case 0xCBB4 :
			reg.h = res(reg.h, 0x40);
			cycles += 8;
			break;

		//RES 6, L
		case 0xCBB5 :
			reg.l = res(reg.l, 0x40);
			cycles += 8;
			break;

		//RES 6, HL
		case 0xCBB6 :
			temp_byte = res(mem->read_u8(reg.hl), 0x40);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RES 6, A
		case 0xCBB7 :
			reg.a = res(reg.a, 0x40);
			cycles += 8;
			break;

		//RES 7, B
		case 0xCBB8 :
			reg.b = res(reg.b, 0x80);
			cycles += 8;
			break;

		//RES 7, C
		case 0xCBB9 :
			reg.c = res(reg.c, 0x80);
			cycles += 8;
			break;

		//RES 7, D
		case 0xCBBA :
			reg.d = res(reg.d, 0x80);
			cycles += 8;
			break;

		//RES 7, E
		case 0xCBBB :
			reg.e = res(reg.e, 0x80);
			cycles += 8;
			break;

		//RES 7, H
		case 0xCBBC :
			reg.h = res(reg.h, 0x80);
			cycles += 8;
			break;

		//RES 7, L
		case 0xCBBD :
			reg.l = res(reg.l, 0x80);
			cycles += 8;
			break;

		//RES 7, HL
		case 0xCBBE :
			temp_byte = res(mem->read_u8(reg.hl), 0x80);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//RES 7, A
		case 0xCBBF :
			reg.a = res(reg.a, 0x80);
			cycles += 8;
			break;

		//SET 0, B
		case 0xCBC0 :
			reg.b = set(reg.b, 0x01);
			cycles += 8;
			break;

		//SET 0, C
		case 0xCBC1 :
			reg.c = set(reg.c, 0x01);
			cycles += 8;
			break;

		//SET 0, D
		case 0xCBC2 :
			reg.d = set(reg.d, 0x01);
			cycles += 8;
			break;

		//SET 0, E
		case 0xCBC3 :
			reg.e = set(reg.e, 0x01);
			cycles += 8;
			break;

		//SET 0, H
		case 0xCBC4 :
			reg.h = set(reg.h, 0x01);
			cycles += 8;
			break;

		//SET 0, L
		case 0xCBC5 :
			reg.l = set(reg.l, 0x01);
			cycles += 8;
			break;

		//SET 0, HL
		case 0xCBC6 :
			temp_byte = set(mem->read_u8(reg.hl), 0x01);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SET 0, A
		case 0xCBC7 :
			reg.a = set(reg.a, 0x01);
			cycles += 8;
			break;

		//SET 1, B
		case 0xCBC8 :
			reg.b = set(reg.b, 0x02);
			cycles += 8;
			break;

		//SET 1, C
		case 0xCBC9 :
			reg.c = set(reg.c, 0x02);
			cycles += 8;
			break;

		//SET 1, D
		case 0xCBCA :
			reg.d = set(reg.d, 0x02);
			cycles += 8;
			break;

		//SET 1, E
		case 0xCBCB :
			reg.e = set(reg.e, 0x02);
			cycles += 8;
			break;

		//SET 1, H
		case 0xCBCC :
			reg.h = set(reg.h, 0x02);
			cycles += 8;
			break;

		//SET 1, L
		case 0xCBCD :
			reg.l = set(reg.l, 0x02);
			cycles += 8;
			break;

		//SET 1, HL
		case 0xCBCE :
			temp_byte = set(mem->read_u8(reg.hl), 0x02);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SET 1, A
		case 0xCBCF :
			reg.a = set(reg.a, 0x02);
			cycles += 8;
			break;

		//SET 2, B
		case 0xCBD0 :
			reg.b = set(reg.b, 0x04);
			cycles += 8;
			break;

		//SET 2, C
		case 0xCBD1 :
			reg.c = set(reg.c, 0x04);
			cycles += 8;
			break;

		//SET 2, D
		case 0xCBD2 :
			reg.d = set(reg.d, 0x04);
			cycles += 8;
			break;

		//SET 2, E
		case 0xCBD3 :
			reg.e = set(reg.e, 0x04);
			cycles += 8;
			break;

		//SET 2, H
		case 0xCBD4 :
			reg.h = set(reg.h, 0x04);
			cycles += 8;
			break;

		//SET 2, L
		case 0xCBD5 :
			reg.l = set(reg.l, 0x04);
			cycles += 8;
			break;

		//SET 2, HL
		case 0xCBD6 :
			temp_byte = set(mem->read_u8(reg.hl), 0x04);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SET 2, A
		case 0xCBD7 :
			reg.a = set(reg.a, 0x04);
			cycles += 8;
			break;

		//SET 3, B
		case 0xCBD8 :
			reg.b = set(reg.b, 0x08);
			cycles += 8;
			break;

		//SET 3, C
		case 0xCBD9 :
			reg.c = set(reg.c, 0x08);
			cycles += 8;
			break;

		//SET 3, D
		case 0xCBDA :
			reg.d = set(reg.d, 0x08);
			cycles += 8;
			break;

		//SET 3, E
		case 0xCBDB :
			reg.e = set(reg.e, 0x08);
			cycles += 8;
			break;

		//SET 3, H
		case 0xCBDC :
			reg.h = set(reg.h, 0x08);
			cycles += 8;
			break;

		//SET 3, L
		case 0xCBDD :
			reg.l = set(reg.l, 0x08);
			cycles += 8;
			break;

		//SET 3, HL
		case 0xCBDE :
			temp_byte = set(mem->read_u8(reg.hl), 0x08);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SET 3, A
		case 0xCBDF :
			reg.a = set(reg.a, 0x08);
			cycles += 8;
			break;

		//SET 4, B
		case 0xCBE0 :
			reg.b = set(reg.b, 0x10);
			cycles += 8;
			break;

		//SET 4, C
		case 0xCBE1 :
			reg.c = set(reg.c, 0x10);
			cycles += 8;
			break;

		//SET 4, D
		case 0xCBE2 :
			reg.d = set(reg.d, 0x10);
			cycles += 8;
			break;

		//SET 4, E
		case 0xCBE3 :
			reg.e = set(reg.e, 0x10);
			cycles += 8;
			break;

		//SET 4, H
		case 0xCBE4 :
			reg.h = set(reg.h, 0x10);
			cycles += 8;
			break;

		//SET 4, L
		case 0xCBE5 :
			reg.l = set(reg.l, 0x10);
			cycles += 8;
			break;

		//SET 4, HL
		case 0xCBE6 :
			temp_byte = set(mem->read_u8(reg.hl), 0x10);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SET 4, A
		case 0xCBE7 :
			reg.a = set(reg.a, 0x10);
			cycles += 8;
			break;

		//SET 5, B
		case 0xCBE8 :
			reg.b = set(reg.b, 0x20);
			cycles += 8;
			break;

		//SET 5, C
		case 0xCBE9 :
			reg.c = set(reg.c, 0x20);
			cycles += 8;
			break;

		//SET 5, D
		case 0xCBEA :
			reg.d = set(reg.d, 0x20);
			cycles += 8;
			break;

		//SET 5, E
		case 0xCBEB :
			reg.e = set(reg.e, 0x20);
			cycles += 8;
			break;

		//SET 5, H
		case 0xCBEC :
			reg.h = set(reg.h, 0x20);
			cycles += 8;
			break;

		//SET 5, L
		case 0xCBED :
			reg.l = set(reg.l, 0x20);
			cycles += 8;
			break;

		//SET 5, HL
		case 0xCBEE :
			temp_byte = set(mem->read_u8(reg.hl), 0x20);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SET 5, A
		case 0xCBEF :
			reg.a = set(reg.a, 0x20);
			cycles += 8;
			break;

		//SET 6, B
		case 0xCBF0 :
			reg.b = set(reg.b, 0x40);
			cycles += 8;
			break;

		//SET 6, C
		case 0xCBF1 :
			reg.c = set(reg.c, 0x40);
			cycles += 8;
			break;
			
		//SET 6, D
		case 0xCBF2 :
			reg.d = set(reg.d, 0x40);
			cycles += 8;
			break;

		//SET 6, E
		case 0xCBF3 :
			reg.e = set(reg.e, 0x40);
			cycles += 8;
			break;

		//SET 6, H
		case 0xCBF4 :
			reg.h = set(reg.h, 0x40);
			cycles += 8;
			break;

		//SET 6, L
		case 0xCBF5 :
			reg.l = set(reg.l, 0x40);
			cycles += 8;
			break;

		//SET 6, HL
		case 0xCBF6 :
			temp_byte = set(mem->read_u8(reg.hl), 0x40);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SET 6, A
		case 0xCBF7 :
			reg.a = set(reg.a, 0x40);
			cycles += 8;
			break;

		//SET 7, B
		case 0xCBF8 :
			reg.b = set(reg.b, 0x80);
			cycles += 8;
			break;

		//SET 7, C
		case 0xCBF9 :
			reg.c = set(reg.c, 0x80);
			cycles += 8;
			break;

		//SET 7, D
		case 0xCBFA :
			reg.d = set(reg.d, 0x80);
			cycles += 8;
			break;

		//SET 7, E
		case 0xCBFB :
			reg.e = set(reg.e, 0x80);
			cycles += 8;
			break;

		//SET 7, H
		case 0xCBFC :
			reg.h = set(reg.h, 0x80);
			cycles += 8;
			break;

		//SET 7, L
		case 0xCBFD :
			reg.l = set(reg.l, 0x80);
			cycles += 8;
			break;

		//SET 7, HL
		case 0xCBFE :
			temp_byte = set(mem->read_u8(reg.hl), 0x80);
			mem->write_u8(reg.hl, temp_byte);
			cycles += 16;
			break;

		//SET 7, A
		case 0xCBFF :
			reg.a = set(reg.a, 0x80);
			cycles += 8;
			break;

		default : 
			running = false;
			std::cout<<"CPU::Error - Unknown Opcode : 0xCB" << std::hex << (int) opcode << "\n";
	}
}
