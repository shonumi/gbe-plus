// GB Enhanced+ Copyright Daniel Baxter 2017
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : debug.cpp
// Date : November 01, 2017
// Description : Debugging interface
//
// DMG debugging via the CLI
// Allows emulator to pause on breakpoints, view and edit memory

#include <iomanip>

#include "common/util.h"

#include "core.h" 

/****** Debugger - Allow core to run until a breaking condition occurs ******/
void NTR_core::debug_step()
{
	bool printed = false;

	//Select NDS9 or NDS7 PC when looking for a break condition
	u32 pc = nds9_debug ? core_cpu_nds9.reg.r15 : core_cpu_nds7.reg.r15;

	arm_debug = false;

	if((nds9_debug) && (core_cpu_nds9.arm_mode == NTR_ARM9::ARM)) { arm_debug = true; }
	else if((!nds9_debug) && (core_cpu_nds7.arm_mode == NTR_ARM7::ARM)) { arm_debug = true; }

	u32 op_addr = nds9_debug ? core_cpu_nds9.debug_addr : core_cpu_nds7.debug_addr;

	//In continue mode, if breakpoints exist, try to stop on one
	if((db_unit.breakpoints.size() > 0) && (db_unit.last_command == "c"))
	{
		for(int x = 0; x < db_unit.breakpoints.size(); x++)
		{
			//When a BP is matched, display info, wait for next input command
			if(pc == db_unit.breakpoints[x])
			{
				db_unit.last_mnemonic = debug_get_mnemonic(op_addr);

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
				std::cout<<"Watchpoint Triggered: 0x" << std::hex << db_unit.watchpoint_addr[x] << " -- Value: 0x" << (u16)db_unit.watchpoint_val[x] << "\n";

				db_unit.last_mnemonic = debug_get_mnemonic(op_addr);

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
		db_unit.last_mnemonic = debug_get_mnemonic(op_addr);

		debug_display();
		debug_process_command();
		printed = true;
	}

	//Display every instruction when print all is enabled
	if((!printed) && (db_unit.print_all))
	{
		db_unit.last_mnemonic = debug_get_mnemonic(op_addr);
		debug_display();
	}
}

/****** Debugger - Display relevant info to the screen ******/
void NTR_core::debug_display() const
{
	//Select NDS9 or NDS7 debugging info
	u32 debug_code = nds9_debug ? core_cpu_nds9.debug_code : core_cpu_nds7.debug_code;
	u8 debug_message = nds9_debug ? core_cpu_nds9.debug_message : core_cpu_nds7.debug_message;
	u32 cpu_regs[16];
	u32 cpsr = nds9_debug ? core_cpu_nds9.reg.cpsr : core_cpu_nds7.reg.cpsr;

	for(u32 x = 0; x < 16; x++)
	{
		cpu_regs[x] = nds9_debug ? core_cpu_nds9.get_reg(x) : core_cpu_nds7.get_reg(x);
	}

	//Display current CPU action
	switch(debug_message)
	{
		case 0xFF:
			std::cout << "Filling pipeline\n"; break;
		case 0x0: 
			std::cout << std::hex << "CPU::Executing THUMB_1 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1:
			std::cout << std::hex << "CPU::Executing THUMB_2 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x2:
			std::cout << std::hex << "CPU::Executing THUMB_3 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x3:
			std::cout << std::hex << "CPU::Executing THUMB_4 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x4:
			std::cout << std::hex << "CPU::Executing THUMB_5 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x5:
			std::cout << std::hex << "CPU::Executing THUMB_6 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x6:
			std::cout << std::hex << "CPU::Executing THUMB_7 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x7:
			std::cout << std::hex << "CPU::Executing THUMB_8 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x8:
			std::cout << std::hex << "CPU::Executing THUMB_9 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x9:
			std::cout << std::hex << "CPU::Executing THUMB_10 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xA:
			std::cout << std::hex << "CPU::Executing THUMB_11 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xB:
			std::cout << std::hex << "CPU::Executing THUMB_12 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xC:
			std::cout << std::hex << "CPU::Executing THUMB_13 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xD:
			std::cout << std::hex << "CPU::Executing THUMB_14 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xE:
			std::cout << std::hex << "CPU::Executing THUMB_15 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0xF:
			std::cout << std::hex << "CPU::Executing THUMB_16 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x10:
			std::cout << std::hex << "CPU::Executing THUMB_17 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x11:
			std::cout << std::hex << "CPU::Executing THUMB_18 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x12:
			std::cout << std::hex << "CPU::Executing THUMB_19 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x13:
			std::cout << std::hex << "Unknown THUMB Instruction : 0x" << debug_code << "\n\n"; break;
		case 0x14:
			std::cout << std::hex << "CPU::Executing ARM_3 : 0x" << debug_code << "\n\n"; break;
		case 0x15:
			std::cout << std::hex << "CPU::Executing ARM_4 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x16:
			std::cout << std::hex << "CPU::Executing ARM_5 : 0x" << debug_code << "\n\n"; break;
		case 0x17:
			std::cout << std::hex << "CPU::Executing ARM_6 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x18:
			std::cout << std::hex << "CPU::Executing ARM_7 : 0x" << debug_code << "\n\n"; break;
		case 0x19:
			std::cout << std::hex << "CPU::Executing ARM_9 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1A:
			std::cout << std::hex << "CPU::Executing ARM_10 : 0x" << debug_code << "\n\n"; break;
		case 0x1B:
			std::cout << std::hex << "CPU::Executing ARM_11 : 0x" << debug_code << "\n\n"; break;
		case 0x1C:
			std::cout << std::hex << "CPU::Executing ARM_12 : 0x" << debug_code << "\n\n"; break;
		case 0x1D:
			std::cout << std::hex << "CPU::Executing ARM_13 : 0x" << debug_code << " -- " << db_unit.last_mnemonic << "\n\n"; break;
		case 0x1E:
			std::cout << std::hex << "CPU::Executing ARM Coprocessor Register Transfer : 0x" << debug_code << "\n\n"; break;
		case 0x1F:
			std::cout << std::hex << "CPU::Executing ARM Coprocessor Data Transfer : 0x" << debug_code << "\n\n"; break;
		case 0x20:
			std::cout << std::hex << "CPU::Executing ARM Coprocessor Data Operation : 0x" << debug_code << "\n\n"; break;
		case 0x21:
			std::cout << std::hex << "CPU::Executing ARM CLZ : 0x" << debug_code << "\n\n"; break;
		case 0x22:
			std::cout << std::hex << "CPU::Executing ARM QADD-QSUB : 0x" << debug_code << "\n\n"; break;
		case 0x23:
			std::cout << std::hex << "Unknown ARM Instruction : 0x" << debug_code << "\n\n"; break;
		case 0x24:
			std::cout << std::hex << "CPU::Skipping ARM Instruction : 0x" << debug_code << "\n\n"; break;
	}

	//Display CPU registers
	std::cout<< std::hex <<"R0 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[0] << 
		" -- R4  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[4] << 
		" -- R8  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[8] << 
		" -- R12 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[12] << "\n";

	std::cout<< std::hex <<"R1 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[1] << 
		" -- R5  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[5] << 
		" -- R9  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[9] << 
		" -- R13 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[13] << "\n";

	std::cout<< std::hex <<"R2 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[2] << 
		" -- R6  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[6] << 
		" -- R10 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[10] << 
		" -- R14 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[14] << "\n";

	std::cout<< std::hex <<"R3 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[3] << 
		" -- R7  : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[7] << 
		" -- R11 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[11] << 
		" -- R15 : 0x" << std::setw(8) << std::setfill('0') << cpu_regs[15] << "\n";

	//Grab CPSR Flags and status
	std::string cpsr_stats = "(";

	if(cpsr & CPSR_N_FLAG) { cpsr_stats += "N"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_Z_FLAG) { cpsr_stats += "Z"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_C_FLAG) { cpsr_stats += "C"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_V_FLAG) { cpsr_stats += "V"; }
	else { cpsr_stats += "."; }

	cpsr_stats += "  ";

	if(cpsr & CPSR_IRQ) { cpsr_stats += "I"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_FIQ) { cpsr_stats += "F"; }
	else { cpsr_stats += "."; }

	if(cpsr & CPSR_STATE) { cpsr_stats += "T)"; }
	else { cpsr_stats += ".)"; }

	std::cout<< std::hex <<"CPSR : 0x" << std::setw(8) << std::setfill('0') << cpsr << "\t" << cpsr_stats << "\n";

	/*
	//Display current CPU cycles
	if(db_unit.display_cycles) { std::cout<<"Current CPU cycles : " << std::dec << core_cpu_nds9.debug_cycles << "\n"; }
	*/
}

/****** Returns a string with the mnemonic assembly instruction ******/
std::string NTR_core::debug_get_mnemonic(u32 addr)
{
	u32 opcode = (arm_debug) ? core_mmu.read_u32(addr) : core_mmu.read_u16(addr);
	std::string instr = "";

	//Get ARM mnemonic
	if(arm_debug)
	{
		std::string cond_code = "";
		u8 cond_bytes = ((opcode >> 28) & 0xF);

		switch(cond_bytes)
		{
			case 0x0: cond_code = "EQ"; break;
			case 0x1: cond_code = "NE"; break;
			case 0x2: cond_code = "CS"; break;
			case 0x3: cond_code = "CC"; break;
			case 0x4: cond_code = "MI"; break;
			case 0x5: cond_code = "PL"; break;
			case 0x6: cond_code = "VS"; break;
			case 0x7: cond_code = "VC"; break;
			case 0x8: cond_code = "HI"; break;
			case 0x9: cond_code = "LS"; break;
			case 0xA: cond_code = "GE"; break;
			case 0xB: cond_code = "LT"; break;
			case 0xC: cond_code = "GT"; break;
			case 0xD: cond_code = "LE"; break;
			case 0xE: cond_code = ""; break;
			case 0xF: cond_code = ""; break;
		}

		//ARM.13 SWI opcodes
		if((opcode & 0xF000000) == 0xF000000)
		{
			instr = "SWI" + cond_code + " " + util::to_hex_str((opcode >> 16) & 0xFF);
		}

		//ARM.4 B, BL, BLX opcodes
		else if((opcode & 0xE000000) == 0xA000000)
		{
			u8 op = (opcode >> 24) & 0x1;
			if((opcode >> 28) == 0xF) { op = 2; }

			u32 offset = (opcode & 0xFFFFFF);
			offset <<= 2;
			if(offset & 0x2000000) { offset |= 0xFC000000; }
			offset += (addr + 8);

			if((op == 2) && (opcode & 0x1000000)) { offset += 2; }

			switch(op)
			{
				case 0x0: instr = "B" + cond_code + " " + util::to_hex_str(offset); break;
				case 0x1: instr = "BL" + cond_code + " " + util::to_hex_str(offset); break;
				case 0x2: instr = "BLX" + cond_code + " " + util::to_hex_str(offset); break;
			}
		}

		//ARM.3 B and BX opcodes
		else if((opcode & 0xFFFFFD0) == 0x12FFF10)
		{
			u8 op = (opcode >> 4) & 0xF;
			u8 rn = (opcode & 0xF);

			switch(op)
			{
				case 0x0: instr = "B" + cond_code + " R" + util::to_str(rn); break;
				case 0x3: instr = "BX" + cond_code + " R" + util::to_str(rn); break;
			}
		}

		//ARM.9 Single Data Transfer opcodes
		else if((opcode & 0xC000000) == 0x4000000)
		{
			u8 op = ((opcode >> 20) & 0x1);
			u8 rd = ((opcode >> 12) & 0xF);
			u8 rn = ((opcode >> 16) & 0xF);
			u8 is_imm = ((opcode >> 25) & 0x1);
			u8 is_up = ((opcode >> 23) & 0x1);
			u8 is_byte = ((opcode >> 22) & 0x1);
			std::string immediate = "";

			if(is_imm)
			{
				u8 shift = ((opcode >> 7) & 0x1F);
				u8 shift_type = ((opcode >> 5) & 0x3);
				u8 rm = (opcode & 0xF);

				immediate = "R" + util::to_str(rm) + " ";

				switch(shift_type)
				{
					case 0x0: immediate += "LSL #" + util::to_str(shift); break;
					case 0x1: immediate += "LSR #" + util::to_str(shift); break;
					case 0x2: immediate += "ASR #" + util::to_str(shift); break;
					case 0x3: immediate += "ROR #" + util::to_str(shift); break;
				}
			}

			else { immediate = util::to_hex_str(opcode & 0xFFF); }

			if(!is_up) { immediate = "- " + immediate; }
			else { immediate = "+ " + immediate; }
				
			switch(op)
			{
				case 0x0:
					instr = "STR" + cond_code;
					if(is_byte) { instr += "B"; }
					instr += " R" + util::to_str(rd) + ",";
					instr += " [R" + util::to_str(rn) + " " + immediate + "]";
					break;

				case 0x1:
					instr = "LDR" + cond_code;
					if(is_byte) { instr += "B"; }
					instr += " R" + util::to_str(rd) + ",";
					instr += " [R" + util::to_str(rn) + " " + immediate + "]";
					break;
			}
		}

		//ARM.12 Swap opcodes
		else if((opcode & 0xFB00FF0) == 0x1000090)
		{
			u8 is_byte = ((opcode >> 22) & 0x1);
			u8 rm = (opcode & 0xF);
			u8 rd = ((opcode >> 12) & 0xF);
			u8 rn = ((opcode >> 16) & 0xF);

			instr = "SWP" + cond_code;
			if(is_byte) { instr += "B"; }
			instr += " R" + util::to_str(rd) + ", R" + util::to_str(rm) + ", [R" + util::to_str(rn) + "]";
		}

		//ARM.6 PSR opcodes
		else if(((opcode & 0xFB00000) == 0x3200000)
		|| ((opcode & 0xF900FF0) == 0x1000000))
		{
			u8 op = ((opcode >> 21) & 0x1);
			u8 is_imm = ((opcode >> 25) & 0x1);
			std::string psr = ((opcode >> 22) & 0x1) ? "SPSR" : "CPSR";

			//MRS
			if(opcode == 0)
			{
				instr = "MRS" + cond_code + ", R" + util::to_str(((opcode >> 12) & 0xF)) + ", " + psr;
			}

			//MSR
			else
			{
				std::string immediate = "";
				std::string flags = "_";
				
				if((opcode >> 19) & 0x1) { flags += "f"; }
				if((opcode >> 18) & 0x1) { flags += "s"; }
				if((opcode >> 17) & 0x1) { flags += "x"; }
				if((opcode >> 16) & 0x1) { flags += "c"; }
				
				if(is_imm)
				{
					u8 ror = (opcode >> 8) & 0xF;
					ror *= 2;

					immediate = util::to_hex_str(opcode & 0xFF) + " ROR #" + util::to_str(ror);
				}

				else { immediate = "R" + util::to_str(opcode & 0xF); }

				instr = "MSR " + psr + ", " + immediate;
			}
		}
	}

	//Get THUMB mnemonic
	else
	{
		//THUMB.17 SWI opcodes
		if((opcode & 0xFF00) == 0xDF00)
		{
			instr = "SWI " + util::to_hex_str(opcode & 0xFF);
		}

		//THUMB.13 ADD SP opcodes
		else if((opcode & 0xFF00) == 0xB000)
		{
			instr = "ADD SP, ";
			u16 offset = ((opcode & 0x7F) << 2);
			
			if(opcode & 0x80) { instr += util::to_hex_str(offset); }
			else { instr += "-" + util::to_hex_str(offset); }
		}

		//THUMB.14 PUSH-POP opcodes
		else if((opcode & 0xF600) == 0xB400)
		{
			instr = (opcode & 0x800) ? "POP " : "PUSH ";
			u8 r_list = (opcode & 0xFF);
			u8 last_reg = 0;

			for(u32 x = 0; x < 8; x++)
			{
				if((r_list >> x) & 0x1) { last_reg = x; }
			}

			for(u32 x = 0; x < 8; x++)
			{
				if((r_list >> x) & 0x1)
				{
					instr += "R" + util::to_str(x);
					if(x != last_reg) { instr += ", "; }
				}
			}

			instr += "}";

			if((opcode & 0x100) && (opcode & 0x800)) { instr += "{R15}"; }
			else if(opcode & 0x800) { instr += "{R14}"; }
		}

		//THUMB.7 Load-Store Reg Offsets opcodes
		else if((opcode & 0xF200) == 0x5000)
		{
			u8 op = ((opcode >> 10) & 0x3);

			instr = (op & 0x2) ? "LDR" : "STR";
			if(op & 0x1) { instr += "B"; }

			instr += " R" + util::to_str(opcode & 0x7) + ",";
			instr += " [R" + util::to_str((opcode >> 3) & 0x7) + "],";
			instr += " R" + util::to_str((opcode >> 6) & 0x7);
		}

		//THUMB.8 Load-Store Sign Extended opcodes
		else if((opcode & 0xF200) == 5200)
		{
			u8 op = ((opcode >> 10) & 0x3);

			switch(op)
			{
				case 0x0: instr = "STRH"; break;
				case 0x1: instr = "LDSB"; break;
				case 0x2: instr = "LDRH"; break;
				case 0x3: instr = "LDSH"; break;
			}

			instr += " R" + util::to_str(opcode & 0x7) + ",";
			instr += " [R" + util::to_str((opcode >> 3) & 0x7) + "],";
			instr += " R" + util::to_str((opcode >> 6) & 0x7);
		}

		//THUMB.10 Load-Store Halfword opcodes
		else if((opcode & 0xF000) == 0x8000)
		{
			u8 op = ((opcode >> 11) & 0x1);
			u8 rd = (opcode & 0x7);
			u8 rb = ((opcode >> 3) & 0x7);
			u8 offset = ((opcode >> 6) & 0x1F);
			offset *= 2;

			switch(op)
			{
				case 0x0: instr = "STRH R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset) + "]"; break;
				case 0x1: instr = "LDRH R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset) + "]"; break;
			}
		}

		//THUMB.11 Load-Store SP relative opcodes
		else if((opcode & 0xF000) == 0x9000)
		{
			u8 op = ((opcode >> 11) & 0x1);
			u8 rd = ((opcode >> 8) & 0x7);
			u16 offset = (opcode & 0xFF);
			offset *= 4;

			switch(op)
			{
				case 0x0: instr = "STR R" + util::to_str(rd) + ", [SP, " + util::to_hex_str(offset) + "]"; break;
				case 0x1: instr = "LDR R" + util::to_str(rd) + ", [SP, " + util::to_hex_str(offset) + "]"; break;
			}
		}

		//THUMB.12 Get relative address opcodes
		else if((opcode & 0xF000) == 0xA000)
		{
			u8 op = ((opcode >> 11) & 0x1);
			u8 rd = ((opcode >> 8) & 0x7);
			u16 offset = (opcode & 0xFF);
			offset *= 4;

			switch(op)
			{
				case 0x0: instr = "ADD R" + util::to_str(rd) + ", PC, " + util::to_hex_str(offset); break;
				case 0x1: instr = "ADD R" + util::to_str(rd) + ", SP, " + util::to_hex_str(offset); break;
			}
		}

		//THUMB.15 LDM-STM opcodes
		else if((opcode & 0xF000) == 0xC000)
		{
			instr = (opcode & 0x800) ? "LDMIA " : "STMIA ";
			u8 r_list = (opcode & 0xFF);
			u8 rb = ((opcode >> 8) & 0x7);
			u8 last_reg = 0;

			for(u32 x = 0; x < 8; x++)
			{
				if((r_list >> x) & 0x1) { last_reg = x; }
			}
				
			instr += util::to_str(rb) + " {";

			for(u32 x = 0; x < 8; x++)
			{
				if((r_list >> x) & 0x1)
				{
					instr += "R" + util::to_str(x);
					if(x != last_reg) { instr += ", "; }
				}
			}

			instr += "}";
		}

		//THUMB.16 Conditional Branch opcodes
		else if((opcode & 0xF000) == 0xD000)
		{
			u8 op = ((opcode >> 8) & 0xF);
			
			u32 offset = (opcode & 0xFF);
			offset <<= 1;			
			if(offset & 0x100) { offset |= 0xFFFFFE00; }
			offset += (addr + 4);

			switch(op)
			{
				case 0x0: instr = "BEQ "; break;
				case 0x1: instr = "BNE "; break;
				case 0x2: instr = "BCS "; break;
				case 0x3: instr = "BCC "; break;
				case 0x4: instr = "BMI "; break;
				case 0x5: instr = "BPL "; break;
				case 0x6: instr = "BVS "; break;
				case 0x7: instr = "BVC "; break;
				case 0x8: instr = "BHI "; break;
				case 0x9: instr = "BLS "; break;
				case 0xA: instr = "BGE "; break;
				case 0xB: instr = "BLT "; break;
				case 0xC: instr = "BGT "; break;
				case 0xD: instr = "BLE "; break;
				default: instr = "UNDEF Branch"; break;
			}

			instr += util::to_hex_str(offset);
		}
	
		//THUMB.4 ALU opcodes
		else if((opcode & 0xFC00) == 0x4000)
		{
			u8 op = ((opcode >> 6) & 0xF);

			switch(op)
			{
				case 0x0: instr = "AND"; break;
				case 0x1: instr = "EOR"; break;
				case 0x2: instr = "LSL"; break;
				case 0x3: instr = "LSR"; break;
				case 0x4: instr = "ASR"; break;
				case 0x5: instr = "ADC"; break;
				case 0x6: instr = "SBC"; break;
				case 0x7: instr = "ROR"; break;
				case 0x8: instr = "TST"; break;
				case 0x9: instr = "NEG"; break;
				case 0xA: instr = "CMP"; break;
				case 0xB: instr = "CMN"; break;
				case 0xC: instr = "ORR"; break;
				case 0xD: instr = "MUL"; break;
				case 0xE: instr = "BIC"; break;
				case 0xF: instr = "MVN"; break;
			}

			instr += "R" + util::to_str(opcode & 0x7) + ", ";
			instr += "R" + util::to_str((opcode >> 3) & 0x7);
		}

		//THUMB.5 High Reg opcodes
		else if((opcode & 0xFC00) == 0x4400)
		{
			u8 op = ((opcode >> 8) & 0x3);
			u8 sr_msb = (opcode & 0x40) ? 1 : 0;
			u8 dr_msb = (opcode & 0x80) ? 1 : 0;

			u8 rs = sr_msb ? 0x8 : 0x0;
			u8 rd = dr_msb ? 0x8 : 0x0;

			rs |= ((opcode >> 3) & 0x7);
			rd |= (opcode & 0x7);

			switch(op)
			{
				case 0x0: instr = "ADD"; break;
				case 0x1: instr = "CMP"; break;
				case 0x2: instr = "MOV"; break;
				case 0x3: instr = dr_msb ? "BLX" : "BX"; break;
			}

			if(op < 3)
			{
				instr += " R" + util::to_str(rd) + ",";
				instr += " R" + util::to_str(rs);
			}

			else { instr += " R" + util::to_str(rs); }
		}

		//THUMB.18 Unconditional Branch opcodes
		else if((opcode & 0xF800) == 0xE000)
		{
			u32 offset = (opcode & 0x7FF);
			offset <<= 1;
			if(offset & 0x800) { offset |= 0xFFFFF000; }
			offset += (addr + 4);

			instr = "B " + util::to_hex_str(offset);
		}

		//THUMB.6 Load PC Relative opcodes
		else if((opcode & 0xF800) == 0x4800)
		{
			u32 offset = (opcode & 0xFF);
			offset <<= 2;
			offset += (addr + 4);

			instr = "LDR R" + util::to_str((opcode >> 8) & 0x7) + ", [" + util::to_hex_str(offset) + "]";
		}

		//THUMB.2 ADD-SUB opcodes
		else if((opcode & 0xF800) == 0x1800)
		{
			u8 op = ((opcode >> 9) & 0x3);
			u8 rd = (opcode & 0x7);
			u8 rs = ((opcode >> 3) & 0x7);
			u8 rn = ((opcode >> 6) & 0x7);

			switch(op)
			{
				case 0x0: instr = "ADD R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", R" + util::to_str(rn); break;
				case 0x1: instr = "SUB R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", R" + util::to_str(rn); break;
				case 0x2: instr = "ADD R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_hex_str(rn); break;
				case 0x3: instr = "SUB R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_hex_str(rn); break;
			}
		}

		//THUMB.1 Move Shifted Register
		else if((opcode & 0xE000) == 0)
		{
			u8 op = ((opcode >> 11) & 0x3);
			u8 rd = (opcode & 0x7);
			u8 rs = ((opcode >> 3) & 0x7);
			u8 offset = ((opcode >> 6) & 0x1F);

			switch(op)
			{
				case 0x0: instr = "LSL R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_str(offset); break;
				case 0x1: instr = "LSR R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_str(offset); break;
				case 0x2: instr = "ASR R" + util::to_str(rd) + ", R" + util::to_str(rs) + ", " + util::to_str(offset); break;
				case 0x3: instr = "Reserved"; break;
			}
		}

		//THUMB.3 MCAS opcodes
		else if((opcode & 0xE000) == 0x2000)
		{
			u8 op = ((opcode >> 11) & 0x3);
			u8 rd = ((opcode >> 8) & 0x7);
			u8 imm = (opcode & 0xFF);

			switch(op)
			{
				case 0x0: instr = "MOV R" + util::to_str(rd) + ", " + util::to_hex_str(imm); break;
				case 0x1: instr = "CMP R" + util::to_str(rd) + ", " + util::to_hex_str(imm); break;
				case 0x2: instr = "ADD R" + util::to_str(rd) + ", " + util::to_hex_str(imm); break;
				case 0x3: instr = "SUB R" + util::to_str(rd) + ", " + util::to_hex_str(imm); break;
			}
		}

		//THUMB.9 Load-Store with offset
		else if((opcode & 0xE000) == 0x6000)
		{
			u8 op = ((opcode >> 11) & 0x3);
			u8 rd = (opcode & 0x7);
			u8 rb = ((opcode >> 3) & 0x7);
			u8 offset = ((opcode >> 6) & 0x1F);

			switch(op)
			{
				case 0x0: instr = "STR R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset * 4) + "]"; break;
				case 0x1: instr = "LDR R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset * 4) + "]"; break;
				case 0x2: instr = "STRB R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset) + "]"; break;
				case 0x3: instr = "LDRB R" + util::to_str(rd) + ", [R" + util::to_str(rb) + ", " + util::to_hex_str(offset) + "]"; break;
			}
		}

		
	}

	return instr;
}

/****** Debugger - Wait for user input, process it to decide what next to do ******/
void NTR_core::debug_process_command()
{
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

		//Switch between NDS9 and NDS7 debugging
		else if(command == "sc")
		{
			valid_command = true;
			db_unit.last_command = "sc";

			if(nds9_debug)
			{
				nds9_debug = false;
				std::cout<<"Switching to NDS7...\n";
			}

			else
			{
				nds9_debug = true;
				std::cout<<"Switching to NDS9...\n";
			}

			debug_display();
			debug_process_command();
		}

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

		//Show memory - byte
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
				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "u8";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u8(mem_location) << "\n";
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Show memory - halfword
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
				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "u16";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u16(mem_location) << "\n";
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Show memory - word
		else if((command.substr(0, 3) == "u32") && (command.substr(4, 2) == "0x"))
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
				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "u32";
				std::cout<<"Memory @ " << hex_string << " : 0x" << std::hex << (int)core_mmu.read_u32(mem_location) << "\n";
				debug_process_command();

				core_mmu.access_mode = last_access;
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

				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "w8";
				core_mmu.write_u8(mem_location, mem_value);
				debug_process_command();

				core_mmu.access_mode = last_access;
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

				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "w16";
				core_mmu.write_u16(mem_location, mem_value);
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
		}

		//Write memory - 4 bytes
		else if((command.substr(0, 3) == "w32") && (command.substr(4, 2) == "0x"))
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
				}

				std::cout<<"\n";

				u8 last_access = core_mmu.access_mode;
				core_mmu.access_mode = nds9_debug;

				db_unit.last_command = "w32";
				core_mmu.write_u32(mem_location, mem_value);
				debug_process_command();

				core_mmu.access_mode = last_access;
			}
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

		/*
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
		*/

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

		//Print help information
		else if(command == "h")
		{
			std::cout<<"n \t\t Run next Fetch-Decode-Execute stage\n";
			std::cout<<"c \t\t Continue until next breakpoint\n";
			std::cout<<"sc \t\t Switch CPU (NDS9 or NDS7)\n";
			std::cout<<"bp \t\t Set breakpoint, format 0x1234ABCD\n";
			std::cout<<"bc \t\t Set breakpoint on memory change, format 0x1234ABCD for addr, 0x12 for value\n";
			std::cout<<"u8 \t\t Show BYTE @ memory, format 0x1234ABCD\n";
			std::cout<<"u16 \t\t Show HALFWORD @ memory, format 0x1234ABCD\n";
			std::cout<<"u32 \t\t Show WORD @ memory, format 0x1234ABCD\n";
			std::cout<<"dq \t\t Quit the debugger\n";
			std::cout<<"dc \t\t Toggle CPU cycle display\n";
			std::cout<<"cr \t\t Reset CPU cycle counter\n";
			std::cout<<"rs \t\t Reset emulation\n";
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
