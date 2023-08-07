// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : dma.cpp
// Date : May 29, 2016
// Description : ARM946E-S/ARM7TDMI emulator
//
// Emulates the NDS9 and NDS7's 4 DMA channels
// Transfers memory to different locations

#include "arm9.h"
#include "arm7.h" 

/****** Performs DMA0 through DMA3 transfers - NDS9 ******/
void NTR_ARM9::nds9_dma(u8 index)
{
	index &= 0x3;

	//Check DMA control register to start transfer
	if((mem->dma[index].control & 0x80000000) == 0) { mem->dma[index].enable = false; return; }

	u32 temp_value = 0;
	u32 original_dest_addr = mem->dma[index].destination_address;
	u8 dma_mode = ((mem->dma[index].control >> 27) & 0x7);

	u32 fill_addr = NDS_DMA0FILL + (index * 4);
	u32 cnt_addr = NDS_DMA0CNT + (index * 12);
	u32 irq_mask = (1 << 8) << index;

	//Verify special DMA timings
	switch(dma_mode)
	{
		//VBlank
		//HBlank
		//Display Sync
		//GXFIFO
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x5:
		case 0x7:
			if(!mem->dma[index].started) { return; }
	}

	//std::cout<<"NDS9 DMA" << std::dec << (u16)index << "\n";
	//std::cout<<"START ADDR -> 0x" << std::hex << mem->dma[index].start_address << "\n";
	//std::cout<<"DEST  ADDR -> 0x" << std::hex << mem->dma[index].destination_address << "\n";
	//std::cout<<"WORD COUNT -> 0x" << std::hex << mem->dma[index].word_count << "\n";

	if(dma_mode <= 3)
	{
		//DMA fill operation
		if(mem->dma[index].start_address == fill_addr) { mem->dma[index].src_addr_ctrl = 4; }

		//16-bit transfer
		if(mem->dma[index].word_type == 0)
		{
			//Align addresses to half-word
			mem->dma[index].start_address &= ~0x1;
			mem->dma[index].destination_address &= ~0x1;

			while(mem->dma[index].word_count != 0)
			{
				temp_value = mem->read_u16(mem->dma[index].start_address);
				mem->write_u16(mem->dma[index].destination_address, temp_value);

				//Update DMA Start Address
				if(mem->dma[index].src_addr_ctrl == 0) { mem->dma[index].start_address += 2; }
				else if(mem->dma[index].src_addr_ctrl == 1) { mem->dma[index].start_address -= 2; }
				else if(mem->dma[index].src_addr_ctrl == 3) { mem->dma[index].start_address += 2; }

				//Update DMA Destination Address
				if(mem->dma[index].dest_addr_ctrl == 0) { mem->dma[index].destination_address += 2; }
				else if(mem->dma[index].dest_addr_ctrl == 1) { mem->dma[index].destination_address -= 2; }
				else if(mem->dma[index].dest_addr_ctrl == 3) { mem->dma[index].destination_address += 2; }

				mem->dma[index].word_count--;
			}
		}

		//32-bit transfer
		else
		{
			//Align addresses to word
			mem->dma[index].start_address &= ~0x3;
			mem->dma[index].destination_address &= ~0x3;

			while(mem->dma[index].word_count != 0)
			{
				temp_value = mem->read_u32(mem->dma[index].start_address);
				mem->write_u32(mem->dma[index].destination_address, temp_value);

				//Update DMA Start Address
				if(mem->dma[index].src_addr_ctrl == 0) { mem->dma[index].start_address += 4; }
				else if(mem->dma[index].src_addr_ctrl == 1) { mem->dma[index].start_address -= 4; }
				else if(mem->dma[index].src_addr_ctrl == 3) { mem->dma[index].start_address += 4; }

				//Update DMA Destination Address
				if(mem->dma[index].dest_addr_ctrl == 0) { mem->dma[index].destination_address += 4; }
				else if(mem->dma[index].dest_addr_ctrl == 1) { mem->dma[index].destination_address -= 4; }
				else if(mem->dma[index].dest_addr_ctrl == 3) { mem->dma[index].destination_address += 4; }

				mem->dma[index].word_count--;

				//Force LCD to process GX commands if necessary
				//Some games manually use an NDS9 DMA to GXFIFO (fixed destination, 32-bit, inc/dec source)
				if(mem->gx_command)
				{
					controllers.video.process_gx_command();
					mem->gx_command = false;
				}
			}
		}
	}

	else if(dma_mode == 5)
	{
		std::cout<<"CART DMA\n";

		mem->dma[index].destination_address &= ~0x3;
		mem->dma[index].word_count = 0x200;

		mem->nds_card.transfer_src = (mem->nds_card.cmd_lo << 8);
		mem->nds_card.transfer_src |= (mem->nds_card.cmd_hi >> 24);
		mem->nds_card.transfer_src &= (mem->cart_data.size() - 1);

		while(mem->dma[index].word_count != 0)
		{
			mem->memory_map[mem->dma[index].destination_address++] = mem->cart_data[mem->nds_card.transfer_src++];
			mem->dma[index].word_count--;
		}

		mem->nds_card.active_transfer = false;
		mem->nds_card.cnt &= ~0x800000;
		mem->nds_card.cnt &= ~0x80000000;

		if((mem->nds9_exmem & 0x800) == 0) { mem->nds9_if |= 0x80000; }
	}

	else if(dma_mode == 7)
	{
		std::cout<<"GX FIFO DMA\n";

		mem->gx_command = false;

		//Align addresses to word
		mem->dma[index].start_address &= ~0x3;

		while(mem->dma[index].word_count != 0)
		{
			temp_value = mem->read_u32(mem->dma[index].start_address);
			mem->write_u32(NDS_GXFIFO, temp_value);

			//Update DMA Start Address
			if(mem->dma[index].src_addr_ctrl == 0) { mem->dma[index].start_address += 4; }
			else if(mem->dma[index].src_addr_ctrl == 1) { mem->dma[index].start_address -= 4; }
			else if(mem->dma[index].src_addr_ctrl == 3) { mem->dma[index].start_address += 4; }

			mem->dma[index].word_count--;

			//Force LCD to process GX commands
			if(mem->gx_command)
			{
				controllers.video.process_gx_command();
				mem->gx_command = false;
			}
		}
	}

	mem->dma[index].control &= ~0x1FFFFF;

	//Trigger IRQ
	if(mem->dma[index].control & 0x40000000) { mem->nds9_if |= irq_mask; }

	//Repeat DMAs
	if(mem->dma[index].control & 0x200)
	{
		mem->dma[index].control |= 0x80000000;
		mem->dma[index].enable = true;
		mem->dma[index].started = false;
	}

	//End DMAs
	else
	{
		mem->dma[index].control &= ~0x80000000;
		mem->dma[index].enable = false;
		mem->dma[index].started = false;
	}

	mem->write_u32(cnt_addr, mem->dma[index].control);

	switch(dma_mode)
	{
		case 0x4: std::cout<<"NDS9 DMA" << std::dec << (u16)index << " - Main Mem Display\n"; running = false; break;
		case 0x6: std::cout<<"NDS9 DMA" << std::dec << (u16)index << " - GBA Cart\n"; running = false; break;
	}
}

/****** Performs DMA0 through DMA3 transfers - NDS7 ******/
void NTR_ARM7::nds7_dma(u8 index)
{
	index &= 0x7;

	if((mem->dma[index].control & 0x80000000) == 0) { mem->dma[index].enable = false; return; }

	u32 temp_value = 0;
	u32 original_dest_addr = mem->dma[index].destination_address;

	u8 dma_mode = ((mem->dma[index].control >> 28) & 0x3);
	u32 cnt_addr = NDS_DMA0CNT + (index * 12);
	u32 irq_mask = (1 << 8) << index;

	//Verify special DMA timings
	switch(dma_mode)
	{
		//VBlank
		case 0x1:
			if(!mem->dma[index].started) { return; }
	}

	//std::cout<<"NDS7 DMA" << std::dec << (u16)index << "\n";
	//std::cout<<"START ADDR -> 0x" << std::hex << mem->dma[index].start_address << "\n";
	//std::cout<<"DEST  ADDR -> 0x" << std::hex << mem->dma[index].destination_address << "\n";
	//std::cout<<"WORD COUNT -> 0x" << std::hex << mem->dma[index].word_count << "\n";

	//16-bit transfer
	if(mem->dma[index].word_type == 0)
	{
		//Align addresses to half-word
		mem->dma[index].start_address &= ~0x1;
		mem->dma[index].destination_address &= ~0x1;

		while(mem->dma[index].word_count != 0)
		{
			temp_value = mem->read_u16(mem->dma[index].start_address);
			mem->write_u16(mem->dma[index].destination_address, temp_value);

			//Update DMA Start Address
			if(mem->dma[index].src_addr_ctrl == 0) { mem->dma[index].start_address += 2; }
			else if(mem->dma[index].src_addr_ctrl == 1) { mem->dma[index].start_address -= 2; }
			else if(mem->dma[index].src_addr_ctrl == 3) { mem->dma[index].start_address += 2; }

			//Update DMA Destination Address
			if(mem->dma[index].dest_addr_ctrl == 0) { mem->dma[index].destination_address += 2; }
			else if(mem->dma[index].dest_addr_ctrl == 1) { mem->dma[index].destination_address -= 2; }
			else if(mem->dma[index].dest_addr_ctrl == 3) { mem->dma[index].destination_address += 2; }

			mem->dma[index].word_count--;
		}
	}

	//32-bit transfer
	else
	{
		//Align addresses to word
		mem->dma[index].start_address &= ~0x3;
		mem->dma[index].destination_address &= ~0x3;

		while(mem->dma[index].word_count != 0)
		{
			temp_value = mem->read_u32(mem->dma[index].start_address);
			mem->write_u32(mem->dma[index].destination_address, temp_value);

			//Update DMA Start Address
			if(mem->dma[index].src_addr_ctrl == 0) { mem->dma[index].start_address += 4; }
			else if(mem->dma[index].src_addr_ctrl == 1) { mem->dma[index].start_address -= 4; }
			else if(mem->dma[index].src_addr_ctrl == 3) { mem->dma[index].start_address += 4; }

			//Update DMA Destination Address
			if(mem->dma[index].dest_addr_ctrl == 0) { mem->dma[index].destination_address += 4; }
			else if(mem->dma[index].dest_addr_ctrl == 1) { mem->dma[index].destination_address -= 4; }
			else if(mem->dma[index].dest_addr_ctrl == 3) { mem->dma[index].destination_address += 4; }

			mem->dma[index].word_count--;
		}
	}

	mem->dma[index].control &= ~0xFFFF;

	//Trigger IRQ
	if(mem->dma[index].control & 0x40000000) { mem->nds7_if |= irq_mask; }

	//Repeat DMAs
	if(mem->dma[index].control & 0x200)
	{
		mem->dma[index].control |= 0x80000000;
		mem->dma[index].enable = true;
		mem->dma[index].started = false;
	}

	//End DMAs
	else
	{
		mem->dma[index].control &= ~0x80000000;
		mem->dma[index].enable = false;
		mem->dma[index].started = false;
	}

	mem->write_u32(cnt_addr, mem->dma[index].control);

	switch(dma_mode)
	{
		case 0x2: std::cout<<"NDS7 DMA" << std::dec << (u16)index << " - DS Cart\n"; running = false; break;
		case 0x3: std::cout<<"NDS7 DMA" << std::dec << (u16)index << " - Wifi/GBA Cart\n"; running = false; break;
	}
}
