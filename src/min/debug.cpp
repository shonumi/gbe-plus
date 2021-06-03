// GB Enhanced+ Copyright Daniel Baxter 2021
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : core.cpp
// Date : January 02, 2021
// Description : Debugging interface
//
// Pokemon Mini debugging via the CLI
// Allows emulator to pause on breakpoints, view and edit memory

#include <iomanip>

#include "common/util.h"

#include "core.h" 

/****** Debugger - Allow core to run until a breaking condition occurs ******/
void MIN_core::debug_step()
{
	//Use external interface (GUI) for all debugging
	if(config::use_external_interfaces)
	{
		config::debug_external();
		return;
	}

	core_cpu.update_regs();

	//Use CLI for all debugging
	bool printed = false;

	//In continue mode, if breakpoints exist, try to stop on one
	if((db_unit.breakpoints.size() > 0) && (db_unit.last_command == "c"))
	{
		for(int x = 0; x < db_unit.breakpoints.size(); x++)
		{
			//When a BP is matched, display info, wait for next input command
			if(core_cpu.reg.pc_ex == db_unit.breakpoints[x])
			{
				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc_ex);

				debug_display();
				debug_process_command();
				printed = true;
			}
		}
	}

	//In continue mode, if a watch point is triggered, try to stop on one
	else if((db_unit.watchpoint_addr.size() > 0) && (db_unit.last_command == "c"))
	{
		for(int x = 0; x < db_unit.watchpoint_addr.size(); x++)
		{
			//When a watchpoint is triggered, display info, wait for next input command
			if((core_mmu.read_u8(db_unit.watchpoint_addr[x]) == db_unit.watchpoint_val[x])
			&& (core_mmu.read_u8(db_unit.watchpoint_addr[x]) != db_unit.watchpoint_old_val[x]))
			{
				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc_ex);

				debug_display();
				debug_process_command();
				printed = true;
			}

			db_unit.watchpoint_old_val[x] = core_mmu.read_u8(db_unit.watchpoint_addr[x]);
		}
	}
				
	//When in next instruction mode, simply display info, wait for next input command
	else if(db_unit.last_command == "n")
	{
		db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc_ex);

		debug_display();
		debug_process_command();
		printed = true;
	}

	//When running for a given amount of instructions, stop when done.
	else if(db_unit.run_count)
	{
		db_unit.run_count--;

		//Stop run count and re-enter debugging mode
		if(!db_unit.run_count)
		{
			db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc_ex);

			debug_display();
			debug_process_command();
			printed = true;
		}
	}

	//Advanced debugging
	#ifdef GBE_DEBUG

	//In continue mode, if a write-breakpoint is triggered, try to stop on one
	else if((db_unit.write_addr.size() > 0) && (db_unit.last_command == "c") && (core_mmu.debug_write))
	{
		for(int x = 0; x < db_unit.write_addr.size(); x++)
		{
			if(db_unit.write_addr[x] == core_mmu.debug_addr)
			{
				db_unit.last_mnemonic = debug_get_mnemonic(db_unit.last_pc);
				debug_display();

				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc_ex);
				debug_display();

				debug_process_command();
				printed = true;
			}
		}
	}

	//In continue mode, if a read-breakpoint is triggered, try to stop on one
	else if((db_unit.read_addr.size() > 0) && (db_unit.last_command == "c") && (core_mmu.debug_read))
	{
		for(int x = 0; x < db_unit.read_addr.size(); x++)
		{
			if(db_unit.read_addr[x] == core_mmu.debug_addr)
			{
				db_unit.last_mnemonic = debug_get_mnemonic(db_unit.last_pc);
				debug_display();

				db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc_ex);
				debug_display();

				debug_process_command();
				printed = true;
			}
		}
	}

	//Reset read-write alerts
	core_mmu.debug_read = false;
	core_mmu.debug_write = false;

	#endif

	//Display every instruction when print all is enabled
	if((!printed) && (db_unit.print_all)) 
	{
		db_unit.last_mnemonic = debug_get_mnemonic(core_cpu.reg.pc_ex);
		debug_display();
	}

	//Display current PC when print PC is enabled
	if(db_unit.print_pc) { std::cout<<"PC -> 0x" << core_cpu.reg.pc_ex << " :: " << debug_get_mnemonic(core_cpu.reg.pc_ex) << "\n"; }

	//Display CPU cycles
	if(db_unit.display_cycles) { std::cout<<"CYCLES : " << std::dec << core_cpu.debug_cycles << "\n"; }

	//Update last PC
	db_unit.last_pc = core_cpu.reg.pc_ex;
}

/****** Returns a string with the mnemonic assembly instruction ******/
std::string MIN_core::debug_get_mnemonic(u32 addr)
{
	u8 opcode = core_mmu.read_u8(addr);
	u8 op1 = core_mmu.read_u8(addr + 1);
	u8 op3 = core_mmu.read_u8(addr + 2);
	u16 op2 = core_mmu.read_u16(addr + 1);
	u16 op4 = core_mmu.read_u16(addr + 2);

	s8 sop1 = core_mmu.read_s8(addr + 1);
	s8 sop3 = core_mmu.read_s8(addr + 2);
	s16 sop2 = core_mmu.read_s16(addr + 1);

	core_cpu.debug_opcode = opcode;

	switch(opcode)
	{
		case 0x00: return "ADD A, A";
		case 0x01: return "ADD A, B";
		case 0x02: return "ADD A, " + util::to_hex_str(op1);
		case 0x03: return "ADD A, [HL]";
		case 0x04: return "ADD A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x05: return "ADD A, [" + util::to_hex_str(op2) + "]";
		case 0x06: return "ADD A, [IX]";
		case 0x07: return "ADD A, [IY]";
		case 0x08: return "ADC A, A";
		case 0x09: return "ADC A, B";
		case 0x0A: return "ADC A, " + util::to_hex_str(op1);
		case 0x0B: return "ADC A, [HL]";
		case 0x0C: return "ADC A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x0D: return "ADC A, [" + util::to_hex_str(op2) + "]";
		case 0x0E: return "ADC A, [IX]";
		case 0x0F: return "ADC A, [IY]";
		case 0x10: return "SUB A, A";
		case 0x11: return "SUB A, B";
		case 0x12: return "SUB A, " + util::to_hex_str(op1);
		case 0x13: return "SUB A, [HL]";
		case 0x14: return "SUB A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x15: return "SUB A, [" + util::to_hex_str(op2) + "]";
		case 0x16: return "SUB A, [IX]";
		case 0x17: return "SUB A, [IY]";
		case 0x18: return "SBC A, A";
		case 0x19: return "SBC A, B";
		case 0x1A: return "SBC A, " + util::to_hex_str(op1);
		case 0x1B: return "SBC A, [HL]";
		case 0x1C: return "SBC A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x1D: return "SBC A, [" + util::to_hex_str(op2) + "]";
		case 0x1E: return "SBC A, [IX]";
		case 0x1F: return "SBC A, [IY]";
		case 0x20: return "AND A, A";
		case 0x21: return "AND A, B";
		case 0x22: return "AND A, " + util::to_hex_str(op1);
		case 0x23: return "AND A, [HL]";
		case 0x24: return "AND A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x25: return "AND A, [" + util::to_hex_str(op2) + "]";
		case 0x26: return "AND A, [IX]";
		case 0x27: return "AND A, [IY]";
		case 0x28: return "OR A, A";
		case 0x29: return "OR A, B";
		case 0x2A: return "OR A, " + util::to_hex_str(op1);
		case 0x2B: return "OR A, [HL]";
		case 0x2C: return "OR A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x2D: return "OR A, [" + util::to_hex_str(op2) + "]";
		case 0x2E: return "OR A, [IX]";
		case 0x2F: return "OR A, [IY]";
		case 0x30: return "CP A, A";
		case 0x31: return "CP A, B";
		case 0x32: return "CP A, " + util::to_hex_str(op1);
		case 0x33: return "CP A, [HL]";
		case 0x34: return "CP A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x35: return "CP A, [" + util::to_hex_str(op2) + "]";
		case 0x36: return "CP A, [IX]";
		case 0x37: return "CP A, [IY]";
		case 0x38: return "XOR A, A";
		case 0x39: return "XOR A, B";
		case 0x3A: return "XOR A, " + util::to_hex_str(op1);
		case 0x3B: return "XOR A, [HL]";
		case 0x3C: return "XOR A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x3D: return "XOR A, [" + util::to_hex_str(op2) + "]";
		case 0x3E: return "XOR A, [IX]";
		case 0x3F: return "XOR A, [IY]";
		case 0x40: return "LD A, A";
		case 0x41: return "LD A, B";
		case 0x42: return "LD A, " + util::to_hex_str(op1);
		case 0x43: return "LD A, [HL]";
		case 0x44: return "LD A, [BR + " + util::to_hex_str(op1) + "]";
		case 0x45: return "LD A, [" + util::to_hex_str(op2) + "]";
		case 0x46: return "LD A, [IX]";
		case 0x47: return "LD A, [IY]";
		case 0x48: return "LD B, A";
		case 0x49: return "LD B, B";
		case 0x4A: return "LD B, " + util::to_hex_str(op1);
		case 0x4B: return "LD B, [HL]";
		case 0x4C: return "LD B, [BR + " + util::to_hex_str(op1) + "]";
		case 0x4D: return "LD B, [" + util::to_hex_str(op2) + "]";
		case 0x4E: return "LD B, [IX]";
		case 0x4F: return "LD B, [IY]";
		case 0x50: return "LD L, A";
		case 0x51: return "LD L, B";
		case 0x52: return "LD L, " + util::to_hex_str(op1);
		case 0x53: return "LD L, [HL]";
		case 0x54: return "LD L, [BR + " + util::to_hex_str(op1) + "]";
		case 0x55: return "LD L, [" + util::to_hex_str(op2) + "]";
		case 0x56: return "LD L, [IX]";
		case 0x57: return "LD L, [IY]";
		case 0x58: return "LD H, A";
		case 0x59: return "LD H, B";
		case 0x5A: return "LD H, " + util::to_hex_str(op1);
		case 0x5B: return "LD H, [HL]";
		case 0x5C: return "LD H, [BR + " + util::to_hex_str(op1) + "]";
		case 0x5D: return "LD H, [" + util::to_hex_str(op2) + "]";
		case 0x5E: return "LD H, [IX]";
		case 0x5F: return "LD H, [IY]";
		case 0x60: return "LD [IX], A";
		case 0x61: return "LD [IX], B";
		case 0x62: return "LD [IX], " + util::to_hex_str(op1);
		case 0x63: return "LD [IX], [HL]";
		case 0x64: return "LD [IX], [BR + " + util::to_hex_str(op1) + "]";
		case 0x65: return "LD [IX], [" + util::to_hex_str(op2) + "]";
		case 0x66: return "LD [IX], [IX]";
		case 0x67: return "LD [IX], [IY]";
		case 0x68: return "LD [HL], A";
		case 0x69: return "LD [HL], B";
		case 0x6A: return "LD [HL], " + util::to_hex_str(op1);
		case 0x6B: return "LD [HL], [HL]";
		case 0x6C: return "LD [HL], [BR + " + util::to_hex_str(op1) + "]";
		case 0x6D: return "LD [HL], [" + util::to_hex_str(op2) + "]";
		case 0x6E: return "LD [HL], [IX]";
		case 0x6F: return "LD [HL], [IY]";
		case 0x70: return "LD [IY], A";
		case 0x71: return "LD [IY], B";
		case 0x72: return "LD [IY], " + util::to_hex_str(op1);
		case 0x73: return "LD [IY], [HL]";
		case 0x74: return "LD [IY], [BR + " + util::to_hex_str(op1) + "]";
		case 0x75: return "LD [IY], [" + util::to_hex_str(op2) + "]";
		case 0x76: return "LD [IY], [IX]";
		case 0x77: return "LD [IY], [IY]";
		case 0x78: return "LD [BR + " + util::to_hex_str(op1) + "], A";
		case 0x79: return "LD [BR + " + util::to_hex_str(op1) + "], B";
		case 0x7A: return "LD [BR + " + util::to_hex_str(op1) + "], L";
		case 0x7B: return "LD [BR + " + util::to_hex_str(op1) + "], H";
		case 0x7D: return "LD [BR + " + util::to_hex_str(op1) + "], [HL]";
		case 0x7E: return "LD [BR + " + util::to_hex_str(op1) + "], [IX]";
		case 0x7F: return "LD [BR + " + util::to_hex_str(op1) + "], [IY]";
		case 0x80: return "INC A";
		case 0x81: return "INC B";
		case 0x82: return "INC L";
		case 0x83: return "INC H";
		case 0x84: return "INC BR";
		case 0x85: return "INC [BR + " + util::to_hex_str(op1) + "]";
		case 0x86: return "INC [HL]";
		case 0x87: return "INC SP";
		case 0x88: return "DEC A";
		case 0x89: return "DEC B";
		case 0x8A: return "DEC L";
		case 0x8B: return "DEC H";
		case 0x8C: return "DEC BR";
		case 0x8D: return "DEC [BR + " + util::to_hex_str(op1) + "]";
		case 0x8E: return "DEC [HL]";
		case 0x8F: return "DEC SP";
		case 0x90: return "INC BA";
		case 0x91: return "INC HL";
		case 0x92: return "INC IX";
		case 0x93: return "INC IY";
		case 0x94: return "BIT A, B";
		case 0x95: return "BIT [HL], " + util::to_hex_str(op1);
		case 0x96: return "BIT A, " + util::to_hex_str(op1);
		case 0x97: return "BIT B, " + util::to_hex_str(op1);
		case 0x98: return "DEC BA";
		case 0x99: return "DEC HL";
		case 0x9A: return "DEC IX";
		case 0x9B: return "DEC IY";
		case 0x9C: return "AND SC, " + util::to_hex_str(op1);
		case 0x9D: return "OR SC, " + util::to_hex_str(op1);
		case 0x9E: return "XOR SC, " + util::to_hex_str(op1);
		case 0x9F: return "LD SC, " + util::to_hex_str(op1);
		case 0xA0: return "PUSH BA";
		case 0xA1: return "PUSH HL";
		case 0xA2: return "PUSH IX";
		case 0xA3: return "PUSH IY";
		case 0xA4: return "PUSH BR";
		case 0xA5: return "PUSH EP";
		case 0xA6: return "PUSH IP";
		case 0xA7: return "PUSH SC";
		case 0xA8: return "POP BA";
		case 0xA9: return "POP HL";
		case 0xAA: return "POP IX";
		case 0xAB: return "POP IY";
		case 0xAC: return "POP BR";
		case 0xAD: return "POP EP";
		case 0xAE: return "POP IP";
		case 0xAF: return "POP SC";
		case 0xB0: return "LD A, " + util::to_hex_str(op1);
		case 0xB1: return "LD B, " + util::to_hex_str(op1);
		case 0xB2: return "LD L, " + util::to_hex_str(op1);
		case 0xB3: return "LD H, " + util::to_hex_str(op1);
		case 0xB4: return "LD BR, " + util::to_hex_str(op1);
		case 0xB5: return "LD [HL], " + util::to_hex_str(op1);
		case 0xB6: return "LD [IX], " + util::to_hex_str(op1);
		case 0xB7: return "LD [IY], " + util::to_hex_str(op1);
		case 0xB8: return "LD BA, [" + util::to_hex_str(op2) + "]";
		case 0xB9: return "LD HL, [" + util::to_hex_str(op2) + "]";
		case 0xBA: return "LD IX, [" + util::to_hex_str(op2) + "]";
		case 0xBB: return "LD IY, [" + util::to_hex_str(op2) + "]";
		case 0xBC: return "LD [" + util::to_hex_str(op2) + "], BA";
		case 0xBD: return "LD [" + util::to_hex_str(op2) + "], HL";
		case 0xBE: return "LD [" + util::to_hex_str(op2) + "], IX";
		case 0xBF: return "LD [" + util::to_hex_str(op2) + "], IY";
		case 0xC0: return "ADD BA, " + util::to_hex_str(op2);
		case 0xC1: return "ADD HL, " + util::to_hex_str(op2);
		case 0xC2: return "ADD IX, " + util::to_hex_str(op2);
		case 0xC3: return "ADD IY, " + util::to_hex_str(op2);
		case 0xC4: return "LD BA, " + util::to_hex_str(op2);
		case 0xC5: return "LD HL, " + util::to_hex_str(op2);
		case 0xC6: return "LD IX, " + util::to_hex_str(op2);
		case 0xC7: return "LD IY, " + util::to_hex_str(op2);
		case 0xC8: return "EX BA, HL" + util::to_hex_str(op2);
		case 0xC9: return "EX BA, IX" + util::to_hex_str(op2);
		case 0xCA: return "EX BA, IY" + util::to_hex_str(op2);
		case 0xCB: return "EX BA, SP" + util::to_hex_str(op2);
		case 0xCC: return "EX A, B" + util::to_hex_str(op2);
		case 0xCD: return "EX A, [HL]" + util::to_hex_str(op2);

		case 0xCE:
			core_cpu.debug_opcode = (core_cpu.debug_opcode << 8) | op1;

			switch(op1)
			{
				case 0x00: return "ADD A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x01: return "ADD A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x02: return "ADD A, [IX + L]";
				case 0x03: return "ADD A, [IY + L]";
				case 0x04: return "ADD [HL], A";
				case 0x05: return "ADD [HL], " + util::to_hex_str(op3);
				case 0x06: return "ADD [HL], [IX]";
				case 0x07: return "ADD [HL], [IY]";
				case 0x08: return "ADC A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x09: return "ADC A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x0A: return "ADC A, [IX + L]";
				case 0x0B: return "ADC A, [IY + L]";
				case 0x0C: return "ADC [HL], A";
				case 0x0D: return "ADC [HL], " + util::to_hex_str(op3);
				case 0x0E: return "ADC [HL], [IX]";
				case 0x0F: return "ADC [HL], [IY]";
				case 0x10: return "SUB A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x11: return "SUB A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x12: return "SUB A, [IX + L]";
				case 0x13: return "SUB A, [IY + L]";
				case 0x14: return "SUB [HL], A";
				case 0x15: return "SUB [HL], " + util::to_hex_str(op3);
				case 0x16: return "SUB [HL], [IX]";
				case 0x17: return "SUB [HL], [IY]";
				case 0x18: return "SBC A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x19: return "SBC A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x1A: return "SBC A, [IX + L]";
				case 0x1B: return "SBC A, [IY + L]";
				case 0x1C: return "SBC [HL], A";
				case 0x1D: return "SBC [HL], " + util::to_hex_str(op3);
				case 0x1E: return "SBC [HL], [IX]";
				case 0x1F: return "SBC [HL], [IY]";
				case 0x20: return "AND A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x21: return "AND A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x22: return "AND A, [IX + L]";
				case 0x23: return "AND A, [IY + L]";
				case 0x24: return "AND [HL], A";
				case 0x25: return "AND [HL], " + util::to_hex_str(op3);
				case 0x26: return "AND [HL], [IX]";
				case 0x27: return "AND [HL], [IY]";
				case 0x28: return "OR A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x29: return "OR A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x2A: return "OR A, [IX + L]";
				case 0x2B: return "OR A, [IY + L]";
				case 0x2C: return "OR [HL], A";
				case 0x2D: return "OR [HL], " + util::to_hex_str(op3);
				case 0x2E: return "OR [HL], [IX]";
				case 0x2F: return "OR [HL], [IY]";
				case 0x30: return "CP A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x31: return "CP A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x32: return "CP A, [IX + L]";
				case 0x33: return "CP A, [IY + L]";
				case 0x34: return "CP [HL], A";
				case 0x35: return "CP [HL], " + util::to_hex_str(op3);
				case 0x36: return "CP [HL], [IX]";
				case 0x37: return "CP [HL], [IY]";
				case 0x38: return "XOR A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x39: return "XOR A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x3A: return "XOR A, [IX + L]";
				case 0x3B: return "XOR A, [IY + L]";
				case 0x3C: return "XOR [HL], A";
				case 0x3D: return "XOR [HL], " + util::to_hex_str(op3);
				case 0x3E: return "XOR [HL], [IX]";
				case 0x3F: return "XOR [HL], [IY]";
				case 0x40: return "LD A, [IX + " + util::to_sstr(sop3) + "]";
				case 0x41: return "LD A, [IY + " + util::to_sstr(sop3) + "]";
				case 0x42: return "LD A, [IX + L]";
				case 0x43: return "LD A, [IY + L]";
				case 0x44: return "LD [IX + " + util::to_sstr(sop3) + "], A";
				case 0x45: return "LD [IY + " + util::to_sstr(sop3) + "], A";
				case 0x46: return "LD [IX + L], A";
				case 0x47: return "LD [IY + L], A";
				case 0x48: return "LD B, [IX + " + util::to_sstr(sop3) + "]";
				case 0x49: return "LD B, [IY + " + util::to_sstr(sop3) + "]";
				case 0x4A: return "LD B, [IX + L]";
				case 0x4B: return "LD B, [IY + L]";
				case 0x4C: return "LD [IX + " + util::to_sstr(sop3) + "], B";
				case 0x4D: return "LD [IY + " + util::to_sstr(sop3) + "], B";
				case 0x4E: return "LD [IX + L], B";
				case 0x4F: return "LD [IY + L], B";
				case 0x50: return "LD L, [IX + " + util::to_sstr(sop3) + "]";
				case 0x51: return "LD L, [IY + " + util::to_sstr(sop3) + "]";
				case 0x52: return "LD L, [IX + L]";
				case 0x53: return "LD L, [IY + L]";
				case 0x54: return "LD [IX + " + util::to_sstr(sop3) + "], L";
				case 0x55: return "LD [IY + " + util::to_sstr(sop3) + "], L";
				case 0x56: return "LD [IX + L], L";
				case 0x57: return "LD [IY + L], L";
				case 0x58: return "LD H, [IX + " + util::to_sstr(sop3) + "]";
				case 0x59: return "LD H, [IY + " + util::to_sstr(sop3) + "]";
				case 0x5A: return "LD H, [IX + L]";
				case 0x5B: return "LD H, [IY + L]";
				case 0x5C: return "LD [IX + " + util::to_sstr(sop3) + "], H";
				case 0x5D: return "LD [IY + " + util::to_sstr(sop3) + "], H";
				case 0x5E: return "LD [IX + L], H";
				case 0x5F: return "LD [IY + L], H";
				case 0x60: return "LD [HL], [IX + " + util::to_sstr(sop3) + "]";
				case 0x61: return "LD [HL], [IY + " + util::to_sstr(sop3) + "]";
				case 0x62: return "LD [HL], [IX + L]";
				case 0x63: return "LD [HL], [IY + L]";
				case 0x68: return "LD [IX], [IX + " + util::to_sstr(sop3) + "]";
				case 0x69: return "LD [IX], [IY + " + util::to_sstr(sop3) + "]";
				case 0x6A: return "LD [IX], [IX + L]";
				case 0x6B: return "LD [IX], [IY + L]";
				case 0x78: return "LD [IY], [IX + " + util::to_sstr(sop3) + "]";
				case 0x79: return "LD [IY], [IY + " + util::to_sstr(sop3) + "]";
				case 0x7A: return "LD [IY], [IX + L]";
				case 0x7B: return "LD [IY], [IY + L]";
				case 0x80: return "SLA A";
				case 0x81: return "SLA B";
				case 0x82: return "SLA [BR + " + util::to_hex_str(op3) + "]";
				case 0x83: return "SLA [HL]";
				case 0x84: return "SLL A";
				case 0x85: return "SLL B";
				case 0x86: return "SLL [BR + " + util::to_hex_str(op3) + "]";
				case 0x87: return "SLL [HL]";
				case 0x88: return "SRA A";
				case 0x89: return "SRA B";
				case 0x8A: return "SRA [BR + " + util::to_hex_str(op3) + "]";
				case 0x8B: return "SRA [HL]";
				case 0x8C: return "SRL A";
				case 0x8D: return "SRL B";
				case 0x8E: return "SRL [BR + " + util::to_hex_str(op3) + "]";
				case 0x8F: return "SRL [HL]";
				case 0x90: return "RL A";
				case 0x91: return "RL B";
				case 0x92: return "RL [BR + " + util::to_hex_str(op3) + "]";
				case 0x93: return "RL [HL]";
				case 0x94: return "RLC A";
				case 0x95: return "RLC B";
				case 0x96: return "RLC [BR + " + util::to_hex_str(op3) + "]";
				case 0x97: return "RLC [HL]";
				case 0x98: return "RR A";
				case 0x99: return "RR B";
				case 0x9A: return "RR [BR + " + util::to_hex_str(op3) + "]";
				case 0x9B: return "RR [HL]";
				case 0x9C: return "RRC A";
				case 0x9D: return "RRC B";
				case 0x9E: return "RRC [BR + " + util::to_hex_str(op3) + "]";
				case 0x9F: return "RRC [HL]";
				case 0xA0: return "CPL A";
				case 0xA1: return "CPL B";
				case 0xA2: return "CPL [BR + " + util::to_hex_str(op3) + "]";
				case 0xA3: return "CPL [HL]";
				case 0xA4: return "NEG A";
				case 0xA5: return "NEG B";
				case 0xA6: return "NEG [BR + " + util::to_hex_str(op3) + "]";
				case 0xA7: return "NEG [HL]";
				case 0xA8: return "SEP";
				case 0xB0: return "AND B " + util::to_hex_str(op3);
				case 0xB1: return "AND L " + util::to_hex_str(op3);
				case 0xB2: return "AND H " + util::to_hex_str(op3);
				case 0xB4: return "OR B " + util::to_hex_str(op3);
				case 0xB5: return "OR L " + util::to_hex_str(op3);
				case 0xB6: return "OR H " + util::to_hex_str(op3);
				case 0xB8: return "XOR B " + util::to_hex_str(op3);
				case 0xB9: return "XOR L " + util::to_hex_str(op3);
				case 0xBA: return "XOR H " + util::to_hex_str(op3);
				case 0xBC: return "CP B " + util::to_hex_str(op3);
				case 0xBD: return "CP L " + util::to_hex_str(op3);
				case 0xBE: return "CP H " + util::to_hex_str(op3);
				case 0xBF: return "CP BR " + util::to_hex_str(op3);
				case 0xC0: return "LD A, BR";
				case 0xC1: return "LD A, SC";
				case 0xC2: return "LD BR, A";
				case 0xC3: return "LD SC, A";
				case 0xC4: return "LD NB, " + util::to_hex_str(op3);
				case 0xC5: return "LD EP, " + util::to_hex_str(op3);
				case 0xC6: return "LD XP, " + util::to_hex_str(op3);
				case 0xC7: return "LD YP, " + util::to_hex_str(op3);
				case 0xC8: return "LD A, NB";
				case 0xC9: return "LD A, EP";
				case 0xCA: return "LD A, XP";
				case 0xCB: return "LD A, YP";
				case 0xCC: return "LD NB, A";
				case 0xCD: return "LD EP, A";
				case 0xCE: return "LD XP, A";
				case 0xCF: return "LD YP, A";
				case 0xD0: return "LD A, [" + util::to_hex_str(op4) + "]";
				case 0xD1: return "LD B, [" + util::to_hex_str(op4) + "]";
				case 0xD2: return "LD L, [" + util::to_hex_str(op4) + "]";
				case 0xD3: return "LD H, [" + util::to_hex_str(op4) + "]";
				case 0xD4: return "LD [" + util::to_hex_str(op4) + "], A";
				case 0xD5: return "LD [" + util::to_hex_str(op4) + "], B";
				case 0xD6: return "LD [" + util::to_hex_str(op4) + "], L";
				case 0xD7: return "LD [" + util::to_hex_str(op4) + "], H";
				case 0xD8: return "MLT L, A";
				case 0xD9: return "DIV HL, A";
				case 0xE0: return "JRS LT, " + util::to_sstr(sop3);
				case 0xE1: return "JRS LE, " + util::to_sstr(sop3);
				case 0xE2: return "JRS GT, " + util::to_sstr(sop3);
				case 0xE3: return "JRS GE, " + util::to_sstr(sop3);
				case 0xE4: return "JRS V, " + util::to_sstr(sop3);
				case 0xE5: return "JRS NV, " + util::to_sstr(sop3);
				case 0xE6: return "JRS P, " + util::to_sstr(sop3);
				case 0xE7: return "JRS M, " + util::to_sstr(sop3);
				case 0xF0: return "CARS LT, " + util::to_sstr(sop3);
				case 0xF1: return "CARS LE, " + util::to_sstr(sop3);
				case 0xF2: return "CARS GT, " + util::to_sstr(sop3);
				case 0xF3: return "CARS GE, " + util::to_sstr(sop3);
				case 0xF4: return "CARS V, " + util::to_sstr(sop3);
				case 0xF5: return "CARS NV, " + util::to_sstr(sop3);
				case 0xF6: return "CARS P, " + util::to_sstr(sop3);
				case 0xF7: return "CARS M, " + util::to_sstr(sop3);
				default: return "";
			}

		case 0xCF:
			core_cpu.debug_opcode = (core_cpu.debug_opcode << 8) | op1;

			switch(op1)
			{
				case 0x00: return "ADD BA, BA";
				case 0x01: return "ADD BA, HL";
				case 0x02: return "ADD BA, IX";
				case 0x03: return "ADD BA, IY";
				case 0x04: return "ADC BA, BA";
				case 0x05: return "ADC BA, HL";
				case 0x06: return "ADC BA, IX";
				case 0x07: return "ADC BA, IY";
				case 0x08: return "SUB BA, BA";
				case 0x09: return "SUB BA, HL";
				case 0x0A: return "SUB BA, IX";
				case 0x0B: return "SUB BA, IY";
				case 0x0C: return "SBC BA, BA";
				case 0x0D: return "SBC BA, HL";
				case 0x0E: return "SBC BA, IX";
				case 0x0F: return "SBC BA, IY";
				case 0x18: return "CP BA, BA";
				case 0x19: return "CP BA, HL";
				case 0x1A: return "CP BA, IX";
				case 0x1B: return "CP BA, IY";
				case 0x20: return "ADD HL, BA";
				case 0x21: return "ADD HL, HL";
				case 0x22: return "ADD HL, IX";
				case 0x23: return "ADD HL, IY";
				case 0x24: return "ADC HL, BA";
				case 0x25: return "ADC HL, HL";
				case 0x26: return "ADC HL, IX";
				case 0x27: return "ADC HL, IY";
				case 0x28: return "SUB HL, BA";
				case 0x29: return "SUB HL, HL";
				case 0x2A: return "SUB HL, IX";
				case 0x2B: return "SUB HL, IY";
				case 0x2C: return "SBC HL, BA";
				case 0x2D: return "SBC HL, HL";
				case 0x2E: return "SBC HL, IX";
				case 0x2F: return "SBC HL, IY";
				case 0x38: return "CP HL, BA";
				case 0x39: return "CP HL, HL";
				case 0x3A: return "CP HL, IX";
				case 0x3B: return "CP HL, IY";
				case 0x40: return "ADD IX, BA";
				case 0x41: return "ADD IX, HL";
				case 0x42: return "ADD IY, BA";
				case 0x43: return "ADD IY, HL";
				case 0x44: return "ADD SP, BA";
				case 0x45: return "ADD SP, HL";
				case 0x48: return "SUB IX, BA";
				case 0x49: return "SUB IX, HL";
				case 0x4A: return "SUB IY, BA";
				case 0x4B: return "SUB IY, HL";
				case 0x4C: return "SUB SP, BA";
				case 0x4D: return "SUB SP, HL";
				case 0x5C: return "CP SP, BA";
				case 0x5D: return "CP SP, HL";
				case 0x60: return "ADC BA, " + util::to_hex_str(op4);
				case 0x61: return "ADC HL, " + util::to_hex_str(op4);
				case 0x62: return "SBC BA, " + util::to_hex_str(op4);
				case 0x63: return "SBC HL, " + util::to_hex_str(op4);
				case 0x68: return "ADD SP, " + util::to_hex_str(op4);
				case 0x6A: return "SUB SP, " + util::to_hex_str(op4);
				case 0x6C: return "CP SP, " + util::to_hex_str(op4);
				case 0x6E: return "LD SP, " + util::to_hex_str(op4);
				case 0x70: return "LD BA, [SP + " + util::to_sstr(sop3) + "]";
				case 0x71: return "LD HL, [SP + " + util::to_sstr(sop3) + "]";
				case 0x72: return "LD IX, [SP + " + util::to_sstr(sop3) + "]";
				case 0x73: return "LD IY, [SP + " + util::to_sstr(sop3) + "]";
				case 0x74: return "LD [SP + " + util::to_sstr(sop3) + "], BA";
				case 0x75: return "LD [SP + " + util::to_sstr(sop3) + "], HL";
				case 0x76: return "LD [SP + " + util::to_sstr(sop3) + "], IX";
				case 0x77: return "LD [SP + " + util::to_sstr(sop3) + "], IY";
				case 0x78: return "LD SP [" + util::to_hex_str(op4) + "]";
				case 0xB0: return "PUSH A";
				case 0xB1: return "PUSH B";
				case 0xB2: return "PUSH L";
				case 0xB3: return "PUSH H";
				case 0xB4: return "POP A";
				case 0xB5: return "POP B";
				case 0xB6: return "POP L";
				case 0xB7: return "POP H";
				case 0xB8: return "PUSH ALL";
				case 0xB9: return "PUSH ALE";
				case 0xBC: return "POP ALL";
				case 0xBD: return "POP ALE";
				case 0xC0: return "LD BA, [HL]";
				case 0xC1: return "LD HL, [HL]";
				case 0xC2: return "LD IX, [HL]";
				case 0xC3: return "LD IY, [HL]";
				case 0xC4: return "LD [HL], BA";
				case 0xC5: return "LD [HL], HL";
				case 0xC6: return "LD [HL], IX";
				case 0xC7: return "LD [HL], IY";
				case 0xD0: return "LD BA, [IX]";
				case 0xD1: return "LD HL, [IX]";
				case 0xD2: return "LD IX, [IX]";
				case 0xD3: return "LD IY, [IX]";
				case 0xD4: return "LD [IX], BA";
				case 0xD5: return "LD [IX], HL";
				case 0xD6: return "LD [IX], IX";
				case 0xD7: return "LD [IX], IY";
				case 0xD8: return "LD BA, [IY]";
				case 0xD9: return "LD HL, [IY]";
				case 0xDA: return "LD IX, [IY]";
				case 0xDB: return "LD IY, [IY]";
				case 0xDC: return "LD [IY], BA";
				case 0xDD: return "LD [IY], HL";
				case 0xDE: return "LD [IY], IX";
				case 0xDF: return "LD [IY], IY";
				case 0xE0: return "LD BA, BA";
				case 0xE1: return "LD BA, HL";
				case 0xE2: return "LD BA, IX";
				case 0xE3: return "LD BA, IY";
				case 0xE4: return "LD HL, BA";
				case 0xE5: return "LD HL, HL";
				case 0xE6: return "LD HL, IX";
				case 0xE7: return "LD HL, IY";
				case 0xE8: return "LD IX, BA";
				case 0xE9: return "LD IX, HL";
				case 0xEA: return "LD IX, IX";
				case 0xEB: return "LD IX, IY";
				case 0xEC: return "LD IY, BA";
				case 0xED: return "LD IY, HL";
				case 0xEE: return "LD IY, IX";
				case 0xEF: return "LD IY, IY";
				case 0xF0: return "LD SP, BA";
				case 0xF1: return "LD SP, HL";
				case 0xF2: return "LD SP, IX";
				case 0xF3: return "LD SP, IY";
				case 0xF4: return "LD HL, SP";
				case 0xF5: return "LD HL, PC";
				case 0xF8: return "LD BA, SP";
				case 0xF9: return "LD BA, PC";
				case 0xFA: return "LD IX, SP";
				case 0xFE: return "LD IY, SP";
				default: return "";
			}

			case 0xD0: return "SUB BA " + util::to_hex_str(op2);
			case 0xD1: return "SUB HL " + util::to_hex_str(op2);
			case 0xD2: return "SUB IX " + util::to_hex_str(op2);
			case 0xD3: return "SUB IY " + util::to_hex_str(op2);
			case 0xD4: return "CP BA " + util::to_hex_str(op2);
			case 0xD5: return "CP HL " + util::to_hex_str(op2);
			case 0xD6: return "CP IX " + util::to_hex_str(op2);
			case 0xD7: return "CP IY " + util::to_hex_str(op2);
			case 0xD8: return "AND [BR + " + util::to_hex_str(op1) + "], " + util::to_hex_str(op3);
			case 0xD9: return "OR [BR  + " + util::to_hex_str(op1) + "], " + util::to_hex_str(op3);
			case 0xDA: return "XOR [BR + " + util::to_hex_str(op1) + "], " + util::to_hex_str(op3);
			case 0xDB: return "CP [BR + " + util::to_hex_str(op1) + "], " + util::to_hex_str(op3);
			case 0xDC: return "BIT [BR + " + util::to_hex_str(op1) + "], " + util::to_hex_str(op3);
			case 0xDD: return "LD [BR + " + util::to_hex_str(op1) + "], " + util::to_hex_str(op3);
			case 0xDE: return "PCK";
			case 0xDF: return "UPCK";
			case 0xE0: return "CARS C, " + util::to_sstr(sop1);
			case 0xE1: return "CARS NC, " + util::to_sstr(sop1);
			case 0xE2: return "CARS Z, " + util::to_sstr(sop1);
			case 0xE3: return "CARS NZ, " + util::to_sstr(sop1);
			case 0xE4: return "JRS C, " + util::to_sstr(sop1);
			case 0xE5: return "JRS NC, " + util::to_sstr(sop1);
			case 0xE6: return "JRS Z, " + util::to_sstr(sop1);
			case 0xE7: return "JRS NZ, " + util::to_sstr(sop1);
			case 0xE8: return "CARL C, " + util::to_sstr(sop2);
			case 0xE9: return "CARL NC, " + util::to_sstr(sop2);
			case 0xEA: return "CARL Z, " + util::to_sstr(sop2);
			case 0xEB: return "CARL NZ, " + util::to_sstr(sop2);
			case 0xEC: return "JRL C, " + util::to_sstr(sop2);
			case 0xED: return "JRL NC, " + util::to_sstr(sop2);
			case 0xEE: return "JRL Z, " + util::to_sstr(sop2);
			case 0xEF: return "JRL NZ, " + util::to_sstr(sop2);
			case 0xF0: return "CARS, " + util::to_sstr(sop1);
			case 0xF1: return "JRS, " + util::to_sstr(sop1);
			case 0xF2: return "CARL, " + util::to_sstr(sop2);
			case 0xF3: return "JRL, " + util::to_sstr(sop2);
			case 0xF4: return "JP HL";
			case 0xF5: return "DJR NZ " + util::to_sstr(sop1);
			case 0xF6: return "SWAP A";
			case 0xF7: return "SWAP [HL]";
			case 0xF8: return "RET";
			case 0xF9: return "RETE";
			case 0xFA: return "RETS";
			case 0xFB: return "CALL [" + util::to_hex_str(op2) + "]";
			case 0xFC: return "INT";
			case 0xFD: return "JP INT";
			case 0xFF: return "NOP";
	}

	return "";
}

/****** Debugger - Display relevant info to the screen ******/
void MIN_core::debug_display() const
{	
	std::cout << std::hex << "CPU::Executing Opcode : 0x" << core_cpu.debug_opcode << " --> " << db_unit.last_mnemonic << "\n\n";

	//Display CPU registers
	std::cout<< std::hex << "A  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.a << 
		" -- B  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.b << "\n";

	std::cout<< std::hex << "H  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.h << 
		" -- L  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.l << "\n";

	std::cout<< std::hex << "IX  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.ix << 
		" -- IY  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.iy << "\n";

	std::cout<< std::hex << "BR  : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.br << "\n";

	std::cout<< std::hex <<"PC : 0x" << std::setw(4) << std::setfill('0') << core_cpu.reg.pc_ex <<
		" -- SP  : 0x" << std::setw(4) << std::setfill('0') << (u32)core_cpu.reg.sp << "\n";

	std::cout<< std::hex <<"CB : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.cb <<
		" -- EP  : 0x" << std::setw(4) << std::setfill('0') << (u32)core_cpu.reg.ep << "\n";

	std::cout<< std::hex <<"XP : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.xp <<
		" -- YP  : 0x" << std::setw(4) << std::setfill('0') << (u32)core_cpu.reg.yp << "\n";

	std::string flag_stats = "(";

	if(core_cpu.reg.sc & 0x20) { flag_stats += "U"; }
	else { flag_stats += "."; }

	if(core_cpu.reg.sc & 0x10) { flag_stats += "D"; }
	else { flag_stats += "."; }

	if(core_cpu.reg.sc & 0x08) { flag_stats += "N"; }
	else { flag_stats += "."; }

	if(core_cpu.reg.sc & 0x04) { flag_stats += "O"; }
	else { flag_stats += "."; }

	if(core_cpu.reg.sc & 0x02) { flag_stats += "C"; }
	else { flag_stats += "."; }

	if(core_cpu.reg.sc & 0x01) { flag_stats += "Z"; }
	else { flag_stats += "."; }

	flag_stats += ")";

	std::cout<< std::hex <<"FLAGS : 0x" << std::setw(2) << std::setfill('0') << (u32)core_cpu.reg.sc << "\t" << flag_stats << "\n";
}

/****** Debugger - Wait for user input, process it to decide what next to do ******/
void MIN_core::debug_process_command()
{
	core_cpu.update_regs();

	std::string command = "";
	std::cout<< ": ";
	std::getline(std::cin, command);

	bool valid_command = false;

	while(!valid_command)
	{
		//Proceed with next instruction
		//Nothing really needs to be done really, so make sure to print a new-line
		if(command == "n") { valid_command = true; std::cout<<"\n"; db_unit.last_command = "n"; }

		//Continue until breaking condition occurs
		else if(command == "c") { valid_command = true; std::cout<<"\n"; db_unit.last_command = "c"; }

		//Quit the emulator
		else if(command == "q") { valid_command = true; stop(); std::cout<<"\n"; }

		//Quit the debugger
		else if(command == "dq") { valid_command = true; db_unit.debug_mode = false; std::cout<<"\n"; }

		//Add breakpoint
		else if((command.substr(0, 2) == "bp") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 bp = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, bp);
		
			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else 
			{
				db_unit.breakpoints.push_back(bp);
				db_unit.last_command = "bp";
				std::cout<<"\nBreakpoint added at 0x" << std::hex << bp << "\n";
				debug_process_command();
			}
		}

		//Delete all breakpoints
		else if(command.substr(0, 3) == "del")
		{
			valid_command = true;
			
			db_unit.breakpoints.clear();
			db_unit.watchpoint_addr.clear();
			db_unit.watchpoint_val.clear();
			db_unit.watchpoint_old_val.clear();

			//Advanced debugging
			#ifdef GBE_DEBUG
			db_unit.write_addr.clear();
			db_unit.read_addr.clear();
			#endif
			
			std::cout<<"\nBreakpoints deleted\n";
			debug_process_command();
		}

		//Show memory - 1 byte
		else if((command.substr(0, 2) == "u8") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				db_unit.last_command = "u8";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u8(mem_location) << "\n";
				debug_process_command();
			}
		}

		//Show memory - 16 bytes
		else if((command.substr(0, 3) == "u8s") && (command.substr(4, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(6);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				db_unit.last_command = "u8s";
				
				for(u32 x = 0; x < 16; x++)
				{
					std::cout<<"Memory @ " << util::to_hex_str(mem_location + x) << " : 0x" << std::hex << (int)core_mmu.read_u8(mem_location + x) << "\n";
				}
				
				debug_process_command();
			}
		}

		//Show memory - 2 bytes
		else if((command.substr(0, 3) == "u16") && (command.substr(4, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(6);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				db_unit.last_command = "u16";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u16(mem_location) << "\n";
				debug_process_command();
			}
		}

		//Write memory - 1 byte
		else if((command.substr(0, 2) == "w8") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 mem_location = 0;
			u32 mem_value = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				//Request value
				while(!valid_value)
				{
					std::cout<<"\nInput value: ";
					std::getline(std::cin, command);
				
					valid_value = util::from_hex_str(command.substr(2), mem_value);
				
					if(!valid_value)
					{
						std::cout<<"\nInvalid value : " << command << "\n";
					}

					else if((valid_value) && (mem_value > 0xFF))
					{
						std::cout<<"\nValue is too large (greater than 0xFF) : 0x" << std::hex << mem_value;
						valid_value = false;
					}
				}

				std::cout<<"\n";

				db_unit.last_command = "w8";
				core_mmu.write_u8(mem_location, mem_value);
				debug_process_command();
			}
		}

		//Write memory - 2 bytes
		else if((command.substr(0, 3) == "w16") && (command.substr(4, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 mem_location = 0;
			u32 mem_value = 0;
			std::string hex_string = command.substr(6);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				//Request value
				while(!valid_value)
				{
					std::cout<<"\nInput value: ";
					std::getline(std::cin, command);
				
					valid_value = util::from_hex_str(command.substr(2), mem_value);
				
					if(!valid_value)
					{
						std::cout<<"\nInvalid value : " << command << "\n";
					}

					else if((valid_value) && (mem_value > 0xFFFF))
					{
						std::cout<<"\nValue is too large (greater than 0xFFFF) : 0x" << std::hex << mem_value;
						valid_value = false;
					}
				}

				std::cout<<"\n";

				db_unit.last_command = "w16";
				core_mmu.write_u16(mem_location, mem_value);
				debug_process_command();
			}
		}

		//Load save state
		else if(command.substr(0, 2) == "ls")
		{
			bool valid_value = false;
			u32 slot = 0;
			std::string slot_string = command.substr(3);

			//Convert string into a usable u32
			valid_value = util::from_str(slot_string, slot);

			if(!valid_value)
			{
				std::cout<<"\nInvalid save state slot : " << slot_string << "\n";
			}

			else
			{
				if(slot >= 10) { std::cout<<"Save state slot too high\n"; }

				else
				{
					std::cout<<"Loading Save State " << slot_string << "\n";
					load_state(slot);
				}
			}

			valid_command = true;
			db_unit.last_command = "ls";
			debug_process_command();
		}

		//Make save state
		else if(command.substr(0, 2) == "ss")
		{
			bool valid_value = false;
			u32 slot = 0;
			std::string slot_string = command.substr(3);

			//Convert string into a usable u32
			valid_value = util::from_str(slot_string, slot);

			if(!valid_value)
			{
				std::cout<<"\nInvalid save state slot : " << slot_string << "\n";
			}

			else
			{
				if(slot >= 10) { std::cout<<"Save state slot too high\n"; }

				else
				{
					std::cout<<"Saving State " << slot_string << "\n";
					save_state(slot);
				}
			}

			valid_command = true;
			db_unit.last_command = "ss";
			debug_process_command();
		}

		//Break on memory change
		else if((command.substr(0, 2) == "bc") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 mem_location = 0;
			u32 mem_value = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				//Request value
				while(!valid_value)
				{
					std::cout<<"\nWatch value: ";
					std::getline(std::cin, command);
				
					valid_value = util::from_hex_str(command.substr(2), mem_value);
				
					if(!valid_value)
					{
						std::cout<<"\nInvalid value : " << command << "\n";
					}

					else if((valid_value) && (mem_value > 0xFF))
					{
						std::cout<<"\nValue is too large (greater than 0xFF) : 0x" << std::hex << mem_value;
						valid_value = false;
					}
				}

				std::cout<<"\n";

				db_unit.last_command = "bc";
				db_unit.watchpoint_addr.push_back(mem_location);
				db_unit.watchpoint_val.push_back(mem_value);
				db_unit.watchpoint_old_val.push_back(core_mmu.read_u8(mem_location));
				debug_process_command();
			}
		}

		//Advanced debugging
		#ifdef GBE_DEBUG

		//Set write breakpoint
		else if((command.substr(0, 2) == "bw") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				db_unit.last_command = "bw";
				db_unit.write_addr.push_back(mem_location);
				std::cout<<"\nWrite Breakpoint added at 0x" << std::hex << mem_location << "\n";
				debug_process_command();
			}
		}

		//Set write breakpoint
		else if((command.substr(0, 2) == "br") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			u32 mem_location = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, mem_location);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				db_unit.last_command = "br";
				db_unit.read_addr.push_back(mem_location);
				std::cout<<"\nRead Breakpoint added at 0x" << std::hex << mem_location << "\n";
				debug_process_command();
			}
		}

		#endif

		//Toggle display of CPU cycles
		else if(command == "dc")
		{
			if(db_unit.display_cycles) 
			{
				std::cout<<"\nDisplay CPU cycles disabled\n";
				db_unit.display_cycles = false;
			}
		
			else
			{
				std::cout<<"\nDisplay CPU cycles enabled\n";
				db_unit.display_cycles = true;
			}

			valid_command = true;
			db_unit.last_command = "dc";
			debug_process_command();
		}

		//Reset CPU cycles counter
		else if(command == "cr")
		{
			std::cout<<"\nCPU cycle counter reset to 0\n";

			valid_command = true;
			core_cpu.debug_cycles = 0;
			debug_process_command();
		}

		//Reset the emulator
		else if(command == "rs")
		{
			std::cout<<"\nManual reset\n";
			reset();
			debug_display();

			valid_command = true;
			db_unit.last_command = "rs";
			debug_process_command();
		}

		//Run emulation for a given amount of instructions before halting
		else if((command.substr(0, 2) == "ri") && (command.substr(3, 2) == "0x"))
		{
			valid_command = true;
			bool valid_value = false;
			u32 instruction_count = 0;
			std::string hex_string = command.substr(5);

			//Convert hex string into usable u32
			valid_command = util::from_hex_str(hex_string, instruction_count);

			//Request valid input again
			if(!valid_command)
			{
				std::cout<<"\nInvalid memory address : " << command << "\n";
				std::cout<<": ";
				std::getline(std::cin, command);
			}

			else
			{
				std::cout<<"\n";
				db_unit.run_count = instruction_count;
				valid_command = true;
				db_unit.last_command = "ri";
			}
		}

		//Print all instructions to the screen
		else if(command == "pa")
		{
			if(db_unit.print_all)
			{
				std::cout<<"\nPrint-All turned off\n";
				db_unit.print_all = false;
			}

			else
			{
				std::cout<<"\nPrint-All turned on\n";
				db_unit.print_all = true;
			}

			valid_command = true;
			db_unit.last_command = "pa";
			debug_process_command();
		}

		//Print every PC to the screen
		else if(command == "pc")
		{
			if(db_unit.print_pc)
			{
				std::cout<<"\nPrint-PC turned off\n";
				db_unit.print_pc = false;
			}

			else
			{
				std::cout<<"\nPrint-PC turned on\n";
				db_unit.print_pc = true;
			}

			valid_command = true;
			db_unit.last_command = "pc";
			debug_process_command();
		}

		//Print help information
		else if(command == "h")
		{
			std::cout<<"n \t\t Run next Fetch-Decode-Execute stage\n";
			std::cout<<"c \t\t Continue until next breakpoint\n";
			std::cout<<"bp \t\t Set breakpoint, format 0x1234\n";
			std::cout<<"bc \t\t Set breakpoint on memory change, format 0x1234 for addr, 0x12 for value\n";

			//Advanced debugging
			#ifdef GBE_DEBUG
			std::cout<<"bw \t\t Set breakpoint on memory write, format 0x1234 for addr\n";
			std::cout<<"br \t\t Set breakpoint on memory read, format 0x1234 for addr\n";
			#endif

			std::cout<<"del \t\t Deletes ALL current breakpoints\n";
			std::cout<<"u8 \t\t Show BYTE @ memory, format 0x1234\n";
			std::cout<<"u8s \t\t Show 16 BYTES @ memory, format 0x1234\n";
			std::cout<<"u16 \t\t Show WORD @ memory, format 0x1234\n";
			std::cout<<"w8 \t\t Write BYTE @ memory, format 0x1234 for addr, 0x12 for value\n";
			std::cout<<"w16 \t\t Write WORD @ memory, format 0x1234 for addr, 0x1234 for value\n";
			std::cout<<"dq \t\t Quit the debugger\n";
			std::cout<<"rs \t\t Reset emulation\n";
			std::cout<<"pa \t\t Toggles printing all instructions to screen\n";
			std::cout<<"pc \t\t Toggles printing all Program Counter values to screen\n"; 
			std::cout<<"q \t\t Quit GBE+\n\n";

			valid_command = true;
			db_unit.last_command = "h";
			debug_process_command();
		}	

		//Request valid input again
		else if(!valid_command)
		{
			std::cout<<"\nInvalid input : " << command << " - Type h for help\n";
			std::cout<<": ";
			std::getline(std::cin, command);
		}
	}
}
