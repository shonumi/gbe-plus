// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : swi.cpp
// Date : October 2, 2014
// Description : GBA ARM7 Software Interrupts
//
// Emulates the GBA's Software Interrupts via High Level Emulation

#include <cmath>

#include "arm7.h"

/****** Process Software Interrupts ******/
void ARM7::process_swi(u8 comment)
{
	switch(comment)
	{
		//SoftReset
		case 0x0:
			std::cout<<"SWI::Soft Reset (not implemented yet) \n";
			break;

		//RegisterRAMReset
		case 0x1:
			std::cout<<"SWI::Register RAM Reset (not implemented yet) \n";
			break;

		//Halt
		case 0x2:
			std::cout<<"SWI::Halt (not implemented yet) \n";
			break;

		//Stop-Sleep
		case 0x3:
			std::cout<<"SWI::Stop-Sleep (not implemented yet) \n";
			break;

		//IntrWait
		case 0x4:
			std::cout<<"SWI::Interrupt Wait (not implemented yet) \n";
			break;

		//VBlankIntrWait
		case 0x5:
			//std::cout<<"SWI::VBlank Interrupt Wait \n";
			swi_vblankintrwait();
			break;

		//Div
		case 0x6:
			std::cout<<"SWI::Divide \n";
			swi_div();
			break;

		//DivARM
		case 0x7:
			std::cout<<"SWI::Divide ARM (not implemented yet) \n";
			break;

		//Sqrt
		case 0x8:
			std::cout<<"SWI::Square Root (not implemented yet) \n";
			break;

		//ArcTan
		case 0x9:
			std::cout<<"SWI::ArcTan (not implemented yet) \n";
			break;

		//ArcTan2
		case 0xA:
			std::cout<<"SWI::ArcTan2 (not implemented yet) \n";
			break;

		//CPUSet
		case 0xB:
			std::cout<<"SWI::CPU Set \n";
			swi_cpuset();
			break;

		//CPUFastSet
		case 0xC:
			std::cout<<"SWI::CPU Fast Set \n";
			swi_cpufastset();
			break;

		//GetBIOSChecksum
		case 0xD:
			std::cout<<"SWI::Get BIOS Checksum (not implemented yet) \n";
			break;

		//BGAffineSet
		case 0xE:
			std::cout<<"SWI::BG Affine Set \n";
			swi_bgaffineset();
			break;

		//OBJAffineSet
		case 0xF:
			std::cout<<"SWI::OBJ Affine Set (not implemented yet) \n";
			break;

		//BitUnPack
		case 0x10:
			std::cout<<"SWI::Bit Unpack (not implemented yet) \n";
			break;

		//LZ77UnCompWram
		case 0x11:
			std::cout<<"SWI::LZ77 Uncompress Work RAM \n";
			swi_lz77uncompvram();
			break;

		//LZ77UnCompVram
		case 0x12:
			std::cout<<"SWI::LZ77 Uncompress Video RAM \n";
			swi_lz77uncompvram();
			break;

		//HuffUnComp
		case 0x13:
			std::cout<<"SWI::Huffman Uncompress \n";
			swi_huffuncomp();
			break;

		//RLUnCompWram
		case 0x14:
			std::cout<<"SWI::Run Length Uncompress Work RAM (not implemented yet) \n";
			break;

		//RLUnCompVram
		case 0x15:
			std::cout<<"SWI::Run Length Uncompress Video RAM (not implemented yet) \n";
			break;

		//Diff8bitUnFilterWram
		case 0x16:
			std::cout<<"SWI::Diff8bitUnFilterWram (not implemented yet) \n";
			break;

		//Diff8bitUnFilterVram
		case 0x17:
			std::cout<<"SWI::Diff8bitUnFilterVram (not implemented yet) \n";
			break;

		//Diff16bitUnFilter
		case 0x18:
			std::cout<<"SWI::Diff16bitUnFilter (not implemented yet) \n";
			break;

		//SoundBias
		case 0x19:
			std::cout<<"SWI::Sound Bias (not implemented yet) \n";
			break;

		//SoundDriverInit
		case 0x1A:
			std::cout<<"SWI::Sound Driver Init (not implemented yet) \n";
			break;

		//SoundDriverMode
		case 0x1B:
			std::cout<<"SWI::Sound Driver Mode (not implemented yet) \n";
			break;

		//SoundDriverMain
		case 0x1C:
			std::cout<<"SWI::Sound Driver Main (not implemented yet) \n";
			break;

		//SoundDriverVSync
		case 0x1D:
			std::cout<<"SWI::Sound Driver VSync (not implemented yet) \n";
			break;

		//SoundChannelClear
		case 0x1E:
			std::cout<<"SWI::Sound Channel Clear (not implemented yet) \n";
			break;

		//MidiKey2Freq
		case 0x1F:
			std::cout<<"SWI::Midi Key to Frequency (not implemented yet) \n";
			break;

		//Undocumented Sound Function 0
		case 0x20:
			std::cout<<"SWI::Undocumented Sound Function 0 \n";
			break;

		//Undocumented Sound Function 1
		case 0x21:
			std::cout<<"SWI::Undocumented Sound Function 1 \n";
			break;

		//Undocumented Sound Function 2
		case 0x22:
			std::cout<<"SWI::Undocumented Sound Function 2 \n";
			break;

		//Undocumented Sound Function 3
		case 0x23:
			std::cout<<"SWI::Undocumented Sound Function 3 \n";
			break;

		//Undocumented Sound Function 4
		case 0x24:
			std::cout<<"SWI::Undocumented Sound Function 4 \n";
			break;

		//Multiboot
		case 0x25:
			std::cout<<"SWI::Multiboot (not implemented yet) \n";
			break;

		//HardReset
		case 0x26:
			std::cout<<"SWI::Hard Reset (not implemented yet) \n";
			break;

		//CustomHalt
		case 0x27:
			std::cout<<"SWI::Custom Halt (not implemented yet) \n";
			break;

		//SoundDriverVSyncOff
		case 0x28:
			std::cout<<"SWI::Sound Driver VSync Off (not implemented yet) \n";
			break;

		//SoundDriverVSyncOn
		case 0x29:
			std::cout<<"SWI::Sound Driver VSync On (not implemented yet) \n";
			break;

		//SoundGetJumpList
		case 0x2A:
			std::cout<<"SWI::Sound Get Jump List (not implemented yet) \n";
			break;

		default:
			std::cout<<"SWI::Error - Unknown BIOS function 0x" << std::hex << (int)comment << "\n";
			break;
	}
}

/****** HLE implementation of Div ******/
void ARM7::swi_div()
{
	//Grab the numerator - R0
	s32 num = get_reg(0);
	
	//Grab the denominator - R1
	s32 den = get_reg(1);

	s32 result = 0;
	s32 modulo = 0;

	//Do NOT divide by 0
	if(den == 0) { std::cout<<"SWI::Warning - Div tried to divide by zero (ignoring operation) \n"; return; }

	//R0 = result of division
	result = num/den;
	set_reg(0, result);

	//R1 = mod of inputs
	modulo = num % den;
	set_reg(1, modulo);

	//R3 = absolute value of division
	if(result < 0) { result *= -1; }
	set_reg(3, result);
}

/****** HLE implementation of CPUFastSet ******/
void ARM7::swi_cpufastset()
{
	//TODO - Timings
	//TODO - Memory alignments

	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab transfer control options - R2
	u32 transfer_control = get_reg(2);

	//Transfer size - Bits 0-20 of R2
	u32 transfer_size = (transfer_control & 0x1FFFFF);

	//Determine if the transfer operation is copy or fill - Bit 24 of R2
	u8 copy_fill = (transfer_control & 0x1000000) ? 1 : 0;

	u32 temp = 0;

	while(transfer_size != 0)
	{
		//Copy from source to destination
		if(copy_fill == 0)
		{
			temp = mem->read_u32(src_addr);
			mem->write_u32(dest_addr, temp);
			
			src_addr += 4;
			dest_addr += 4;

			transfer_size--;
		}

		//Fill first entry from source with destination
		else
		{
			temp = mem->read_u32(src_addr);
			mem->write_u32(dest_addr, temp);
			
			dest_addr += 4;
			
			transfer_size--;
		}
	}
}

/****** HLE implementation of CPUSet ******/
void ARM7::swi_cpuset()
{
	//TODO - Timings
	//TODO - Memory alignments

	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab transfer control options - R2
	u32 transfer_control = get_reg(2);

	//Transfer size - Bits 0-20 of R2
	u32 transfer_size = (transfer_control & 0x1FFFFF);

	//Determine if the transfer operation is copy or fill - Bit 24 of R2
	u8 copy_fill = (transfer_control & 0x1000000) ? 1 : 0;

	//Determine if the transfer operation is 16 or 32-bit - Bit 26 of R2
	u8 transfer_type = (transfer_control & 0x4000000) ? 1 : 0;

	u32 temp_32 = 0;
	u16 temp_16 = 0;

	while(transfer_size != 0)
	{
		//Copy from source to destination
		if(copy_fill == 0)
		{
			//16-bit transfer
			if(transfer_type == 0)
			{
				temp_16 = mem->read_u16(src_addr);
				mem->write_u16(dest_addr, temp_16);
			
				src_addr += 2;
				dest_addr += 2;
			}

			//32-bit transfer
			else
			{
				temp_32 = mem->read_u32(src_addr);
				mem->write_u32(dest_addr, temp_32);
			
				src_addr += 4;
				dest_addr += 4;
			}

			transfer_size--;
		}

		//Fill first entry from source with destination
		else
		{
			//16-bit transfer
			if(transfer_type == 0)
			{
				temp_16 = mem->read_u16(src_addr);
				mem->write_u16(dest_addr, temp_16);
			
				dest_addr += 2;
			}

			//32-bit transfer
			else
			{
				temp_32 = mem->read_u32(src_addr);
				mem->write_u32(dest_addr, temp_32);
			
				dest_addr += 4;
			}
			
			transfer_size--;
		}
	}
}

/****** HLE implementation of VBlankIntrWait ******/
void ARM7::swi_vblankintrwait()
{
	//Set R0 and R1 to 1
	set_reg(0, 1);
	set_reg(1, 1);

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u16(REG_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	u8 previous_mode = controllers.video.lcd_mode;
	bool mode_change = false;

	//Run controllers until VBlank interrupt is generated
	while(!mode_change)
	{
		clock();
		if((controllers.video.lcd_mode == 2) && (previous_mode != 2)) { mode_change = true; mem->memory_map[REG_IF] |= 0x1; reg.r15 += 2; }
		else { previous_mode = controllers.video.lcd_mode; }
	}
}

/****** HLE implementation of LZ77UnCompVram ******/
void ARM7::swi_lz77uncompvram()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab data header
	u32 data_header = mem->read_u32(src_addr);

	//Grab compressed data size in bytes
	u32 data_size = (data_header >> 8);

	//Pointer to current address of compressed data that needs to be processed
	//When uncompression starts, move 5 bytes from source address (header + flag)
	u32 data_ptr = (src_addr + 4);

	u8 temp = 0;

	//Uncompress data
	while(data_size > 0)
	{
		//Grab flag data
		u8 flag_data = mem->read_u8(data_ptr++);

		//Process 8 blocks
		for(int x = 7; x >= 0; x--)
		{
			u8 block_type = (flag_data & (1 << x)) ? 1 : 0;

			//Block Type 0 - Uncompressed
			if(block_type == 0)
			{
				temp = mem->read_u8(data_ptr++);
				mem->write_u8(dest_addr++, temp);
				
				data_size--;
				if(data_size == 0) { return; }
			}


			//Block Type 1 - Compressed
			else
			{
				u16 compressed_block = mem->read_u16(data_ptr);
				data_ptr += 2;

				u16 distance = ((compressed_block & 0xF) << 8);
				distance |= (compressed_block >> 8);

				u8 length = ((compressed_block >> 4) & 0xF) + 3;

				//Copy length+3 Bytes from dest_addr-length-1 to dest_addr
				for(int y = 0; y < length; y++)
				{
					temp = mem->read_u8(dest_addr - distance - 1);
					mem->write_u8(dest_addr, temp);
					
					dest_addr++;
					data_size--;
					if(data_size == 0) { return; }
				}
			}
		}
	}
}

/****** HLE implementation of HuffUnComp ******/
void ARM7::swi_huffuncomp()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab data header
	u32 data_header = mem->read_u32(src_addr);

	//Grab data bit-size
	u8 bit_size = (data_header & 0xF);

	if((bit_size != 4) && (bit_size != 8)) { std::cout<<"SWI::Warning - HuffUnComp has irregular data size : " << (int)bit_size << "\n"; }

	//Grab compressed data size in bytes - Data comes in units of 32-bits, 4 bytes
	u32 data_size = (data_header >> 8);
	while((data_size & 0x3) != 0) { data_size++; }

	//Pointer to current address that needs to be processed
	//When uncompression start, points to data after the header (the first Tree Size attribute)
	u32 data_ptr = (src_addr + 4);

	u8 data_shift = 0;
	u32 temp = 0;

	//Grab Tree Size
	u16 tree_size = mem->read_u8(data_ptr++);
	tree_size = ((tree_size + 1) * 2) - 1;

	//Grab the root node
	u32 root_node_addr = data_ptr;
	u32 node_position = 0;

	//Grab the 32-bit compressed bitstream
	u32 bitstream = mem->read_u32(data_ptr + tree_size);
	u32 bitstream_addr = (data_ptr + tree_size);
	u32 bitstream_mask = 0x80000000;

	u8 count = 0;
	bool is_data_node = false;

	//Uncompress data
	while(data_size > 0)
	{
		//Begin parsing the nodes, starting with root node
		while(bitstream_mask != 0)
		{
			u8 node = mem->read_u8(data_ptr);
			u8 node_offset = (node & 0x3F);
			u8 node_0_type = (node & 0x80) ? 1 : 0;
			u8 node_1_type = (node & 0x40) ? 1 : 0;

			//If this node is a data node, read data
			if(is_data_node) 
			{
				//Grab data from the node
				u8 data = 0;

				if(data_size == 4) { data = node & 0xF; }
				else { data = node; }

				//Add data to 32-bit value
				temp |= (data << data_shift);
				data_shift += bit_size;

				//Transfer completed 32-bit value to memory
				if(data_shift >= 32) 
				{
					mem->write_u32(dest_addr, temp);
					dest_addr += 4;
					data_size -= 4;
					temp = 0;
					data_shift = 0;

					if(data_size == 0) { return; } 
				}

				//Return to root node
				data_ptr = root_node_addr;
				is_data_node = false;
				node_position = 0;
			}

			//If this node is a child node, continue along the binary trees
			else
			{
				if(node_position == 0) { node_position++; }
				else { node_position += ((node_offset + 1) * 2); }
			
				//Read bitstream bit, decide if child_node.0 or child_node.1 should be looked at
				u8 bitstream_bit = (bitstream & bitstream_mask) ? 1 : 0;
				bitstream_mask >>= 1;

				//Go offset for child_node.1
				if(bitstream_bit == 1) 
				{
					data_ptr = (root_node_addr + node_position + 1);
					if(node_1_type == 1) { is_data_node = true; }
					else { is_data_node = false; }
				}

				//Go offset for child_node.0
				else 
				{
					data_ptr = (root_node_addr + node_position);
					if(node_0_type == 1) { is_data_node = true; }
					else { is_data_node = false; }
				}
				
			}
		}

		//After this 32-bit bitstream is complete, move onto the next bitstream
		bitstream_addr += 4;
		bitstream = mem->read_u32(bitstream_addr);
		bitstream_mask = 0x80000000;
	}
}
	
/****** HLE implementation of BGAffineSet ******/
void ARM7::swi_bgaffineset()
{
	const double PI = 3.1415926535897;

	//Grab source data field address
	u32 src_addr = get_reg(0);

	//Grab destination field address
	u32 dest_addr = get_reg(1);

	//Grab the number of calculations to perform
	u32 calc_count = get_reg(2);

	//Perform the desired number of calculations
	while(calc_count-- > 0)
	{
		//Grab original data's central X-Y coordinates
		s32 center_x = mem->read_u32(src_addr); src_addr += 4;
		s32 center_y = mem->read_u32(src_addr); src_addr += 4;

		//Grab the display's central X-Y coordinates
		s16 display_x = mem->read_u16(src_addr); src_addr += 2;
		s16 display_y = mem->read_u16(src_addr); src_addr += 2;

		//Grab the X-Y scaling ratios
		s16 scale_x = mem->read_u16(src_addr); src_addr += 2;
		s16 scale_y = mem->read_u16(src_addr); src_addr += 2;

		//Grab the angle of rotation, add 4 to keep data structure aligned by word-size
		u16 theta = mem->read_u16(src_addr); src_addr += 4;
		theta = (theta/0x80) * PI;

		double cos_angle = cos(theta);
		double sin_angle = sin(theta);

		//Calculate differences in X-Y coordinates for this line and the next
		s16 diff_x1 = (scale_x * cos_angle);
		s16 diff_x2 = (scale_x * sin_angle);
		s16 diff_y1 = (scale_y * cos_angle);
		s16 diff_y2 = (scale_y * sin_angle);

		//Write to destination data structure
		mem->write_u16(dest_addr, diff_x1); dest_addr += 2;
		mem->write_u16(dest_addr, -diff_x2); dest_addr += 2;
		mem->write_u16(dest_addr, diff_y1); dest_addr += 2;
		mem->write_u16(dest_addr, diff_y2); dest_addr += 2;

		//Calculate start X-Y coordinates
		s32 start_x = (center_x - diff_x1 * display_x + diff_x2 * display_y);
		s32 start_y = (center_y - diff_y1 * display_x + diff_y2 * display_y);

		mem->write_u32(dest_addr, start_x); dest_addr += 4;
		mem->write_u32(dest_addr, start_y); dest_addr += 4;
	}
} 
