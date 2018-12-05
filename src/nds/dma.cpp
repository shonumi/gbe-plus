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
	u8 dma_mode = ((mem->dma[index].control >> 28) & 0x7);

	u32 fill_addr = NDS_DMA0FILL + (index * 4);
	u32 cnt_addr = NDS_DMA0CNT + (index * 12);
	u32 irq_mask = (1 << 8) << index;

	//Verify HBlank timing
	if((dma_mode == 2) && (!mem->dma[index].started)) { return; }

	std::cout<<"NDS9 DMA" << std::dec << (u16)index << "\n";
	std::cout<<"START ADDR -> 0x" << std::hex << mem->dma[index].start_address << "\n";
	std::cout<<"DEST  ADDR -> 0x" << std::hex << mem->dma[index].destination_address << "\n";
	std::cout<<"WORD COUNT -> 0x" << std::hex << mem->dma[index].word_count << "\n";

	//DMA fill operation
	if(mem->dma[index].start_address == fill_addr)
	{
		mem->dma[index].start_address = mem->read_u32(fill_addr);
		mem->dma[index].src_addr_ctrl = 4;
	}

	//16-bit transfer
	if(mem->dma[index].word_type == 0)
	{
		//Align addresses to half-word
		mem->dma[index].start_address &= ~0x1;
		mem->dma[index].destination_address	&= ~0x1;

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

	mem->dma[index].control &= ~0x80000000;
	mem->write_u32(cnt_addr, mem->dma[index].control);

	//Trigger IRQ
	if(mem->dma[index].control & 0x40000000) { mem->nds9_if |= irq_mask; }

	mem->dma[index].enable = false;
	mem->dma[index].started = true;

	switch(dma_mode)
	{
		case 0x1: std::cout<<"NDS9 DMA" << std::dec << (u16)index << " - VBlank\n"; running = false; break;
		case 0x3: std::cout<<"NDS9 DMA" << std::dec << (u16)index << " - Display Sync\n"; running = false; break;
		case 0x4: std::cout<<"NDS9 DMA" << std::dec << (u16)index << " - Main Mem Display\n"; running = false; break;
		case 0x5: std::cout<<"NDS9 DMA" << std::dec << (u16)index << " - DS Cart\n"; running = false; break;
		case 0x6: std::cout<<"NDS9 DMA" << std::dec << (u16)index << " - GBA Cart\n"; running = false; break;
		case 0x7: std::cout<<"NDS9 DMA" << std::dec << (u16)index << " - Geometry Command FIFO\n"; running = false; break;
	}
}

/****** Performs DMA0 transfers - NDS7 ******/
void NTR_ARM7::dma0()
{
	//Wait 2 cycles after DMA is triggered before actual transfer
	if(mem->dma[4].delay != 0) { mem->dma[4].delay--; }

	//See if DMA Start Timing conditions dictate a transfer
	else
	{
		u32 temp_value = 0;
		u32 original_dest_addr = mem->dma[4].destination_address;

		if((mem->dma[4].control & 0x80000000) == 0) { mem->dma[4].enable = false; return; }

		//Check DMA Start Timings
		switch(((mem->dma[4].control >> 28) & 0x3))
		{
			case 0x0:
			{
				std::cout<<"NDS7 DMA0 - Immediate\n";
				std::cout<<"START ADDR -> 0x" << std::hex << mem->dma[4].start_address << "\n";
				std::cout<<"DEST  ADDR -> 0x" << std::hex << mem->dma[4].destination_address << "\n";
				std::cout<<"WORD COUNT -> 0x" << std::hex << mem->dma[4].word_count << "\n";

				//16-bit transfer
				if(mem->dma[4].word_type == 0)
				{
					//Align addresses to half-word
					mem->dma[4].start_address &= ~0x1;
					mem->dma[4].destination_address	&= ~0x1;

					while(mem->dma[4].word_count != 0)
					{
						temp_value = mem->read_u16(mem->dma[4].start_address);
						mem->write_u16(mem->dma[4].destination_address, temp_value);

						//Update DMA0 Start Address
						if(mem->dma[4].src_addr_ctrl == 0) { mem->dma[4].start_address += 2; }
						else if(mem->dma[4].src_addr_ctrl == 1) { mem->dma[4].start_address -= 2; }
						else if(mem->dma[4].src_addr_ctrl == 3) { mem->dma[4].start_address += 2; }

						//Update DMA0 Destination Address
						if(mem->dma[4].dest_addr_ctrl == 0) { mem->dma[4].destination_address += 2; }
						else if(mem->dma[4].dest_addr_ctrl == 1) { mem->dma[4].destination_address -= 2; }
						else if(mem->dma[4].dest_addr_ctrl == 3) { mem->dma[4].destination_address += 2; }

						mem->dma[4].word_count--;
					}
				}

				//32-bit transfer
				else
				{
					//Align addresses to word
					mem->dma[4].start_address &= ~0x3;
					mem->dma[4].destination_address	&= ~0x3;

					while(mem->dma[4].word_count != 0)
					{
						temp_value = mem->read_u32(mem->dma[4].start_address);
						mem->write_u32(mem->dma[4].destination_address, temp_value);

						//Update DMA0 Start Address
						if(mem->dma[4].src_addr_ctrl == 0) { mem->dma[4].start_address += 4; }
						else if(mem->dma[4].src_addr_ctrl == 1) { mem->dma[4].start_address -= 4; }
						else if(mem->dma[4].src_addr_ctrl == 3) { mem->dma[4].start_address += 4; }

						//Update DMA0 Destination Address
						if(mem->dma[4].dest_addr_ctrl == 0) { mem->dma[4].destination_address += 4; }
						else if(mem->dma[4].dest_addr_ctrl == 1) { mem->dma[4].destination_address -= 4; }
						else if(mem->dma[4].dest_addr_ctrl == 3) { mem->dma[4].destination_address += 4; }

						mem->dma[4].word_count--;
					}
				}
			}

			mem->dma[4].control &= ~0x80000000;
			mem->write_u32(NDS_DMA0CNT, mem->dma[4].control);

			//Trigger IRQ
			if(mem->dma[4].control & 0x40000000) { mem->nds7_if |= 0x100; }

			break;

			case 0x1: std::cout<<"NDS7 DMA0 - VBlank\n"; break;
			case 0x2: std::cout<<"NDS7 DMA0 - DS Cart\n"; break;
			case 0x3: std::cout<<"NDS7 DMA0 - Wifi/GBA Cart\n"; break;

		}

		mem->dma[4].enable = false;

		if((mem->dma[4].control >> 28) & 0x7) { running = false; }
	}
}

/****** Performs DMA1 transfers - NDS7 ******/
void NTR_ARM7::dma1()
{
	//Wait 2 cycles after DMA is triggered before actual transfer
	if(mem->dma[5].delay != 0) { mem->dma[5].delay--; }

	//See if DMA Start Timing conditions dictate a transfer
	else
	{
		u32 temp_value = 0;
		u32 original_dest_addr = mem->dma[5].destination_address;

		if((mem->dma[5].control & 0x80000000) == 0) { mem->dma[5].enable = false; return; }

		//Check DMA Start Timings
		switch(((mem->dma[5].control >> 28) & 0x3))
		{
			case 0x0:
			{
				std::cout<<"NDS7 DMA1 - Immediate\n";
				std::cout<<"START ADDR -> 0x" << std::hex << mem->dma[5].start_address << "\n";
				std::cout<<"DEST  ADDR -> 0x" << std::hex << mem->dma[5].destination_address << "\n";
				std::cout<<"WORD COUNT -> 0x" << std::hex << mem->dma[5].word_count << "\n";

				//16-bit transfer
				if(mem->dma[5].word_type == 0)
				{
					//Align addresses to half-word
					mem->dma[5].start_address &= ~0x1;
					mem->dma[5].destination_address	&= ~0x1;

					while(mem->dma[5].word_count != 0)
					{
						temp_value = mem->read_u16(mem->dma[5].start_address);
						mem->write_u16(mem->dma[5].destination_address, temp_value);

						//Update DMA1 Start Address
						if(mem->dma[5].src_addr_ctrl == 0) { mem->dma[5].start_address += 2; }
						else if(mem->dma[5].src_addr_ctrl == 1) { mem->dma[5].start_address -= 2; }
						else if(mem->dma[5].src_addr_ctrl == 3) { mem->dma[5].start_address += 2; }

						//Update DMA1 Destination Address
						if(mem->dma[5].dest_addr_ctrl == 0) { mem->dma[5].destination_address += 2; }
						else if(mem->dma[5].dest_addr_ctrl == 1) { mem->dma[5].destination_address -= 2; }
						else if(mem->dma[5].dest_addr_ctrl == 3) { mem->dma[5].destination_address += 2; }

						mem->dma[5].word_count--;
					}
				}

				//32-bit transfer
				else
				{
					//Align addresses to word
					mem->dma[5].start_address &= ~0x3;
					mem->dma[5].destination_address	&= ~0x3;

					while(mem->dma[5].word_count != 0)
					{
						temp_value = mem->read_u32(mem->dma[5].start_address);
						mem->write_u32(mem->dma[5].destination_address, temp_value);

						//Update DMA1 Start Address
						if(mem->dma[5].src_addr_ctrl == 0) { mem->dma[5].start_address += 4; }
						else if(mem->dma[5].src_addr_ctrl == 1) { mem->dma[5].start_address -= 4; }
						else if(mem->dma[5].src_addr_ctrl == 3) { mem->dma[5].start_address += 4; }

						//Update DMA1 Destination Address
						if(mem->dma[5].dest_addr_ctrl == 0) { mem->dma[5].destination_address += 4; }
						else if(mem->dma[5].dest_addr_ctrl == 1) { mem->dma[5].destination_address -= 4; }
						else if(mem->dma[5].dest_addr_ctrl == 3) { mem->dma[5].destination_address += 4; }

						mem->dma[5].word_count--;
					}
				}
			}

			mem->dma[5].control &= ~0x80000000;
			mem->write_u32(NDS_DMA1CNT, mem->dma[5].control);

			//Trigger IRQ
			if(mem->dma[5].control & 0x40000000) { mem->nds7_if |= 0x200; }

			break;

			case 0x1: std::cout<<"NDS7 DMA1 - VBlank\n"; break;
			case 0x2: std::cout<<"NDS7 DMA1 - DS Cart\n"; break;
			case 0x3: std::cout<<"NDS7 DMA1 - Wifi/GBA Cart\n"; break;

		}

		mem->dma[5].enable = false;

		if((mem->dma[5].control >> 28) & 0x7) { running = false; }
	}
}

/****** Performs DMA2 transfers - NDS7 ******/
void NTR_ARM7::dma2()
{
	//Wait 2 cycles after DMA is triggered before actual transfer
	if(mem->dma[6].delay != 0) { mem->dma[6].delay--; }

	//See if DMA Start Timing conditions dictate a transfer
	else
	{
		u32 temp_value = 0;
		u32 original_dest_addr = mem->dma[6].destination_address;

		if((mem->dma[6].control & 0x80000000) == 0) { mem->dma[6].enable = false; return; }

		//Check DMA Start Timings
		switch(((mem->dma[6].control >> 28) & 0x3))
		{
			case 0x0:
			{
				std::cout<<"NDS7 DMA2 - Immediate\n";
				std::cout<<"START ADDR -> 0x" << std::hex << mem->dma[6].start_address << "\n";
				std::cout<<"DEST  ADDR -> 0x" << std::hex << mem->dma[6].destination_address << "\n";
				std::cout<<"WORD COUNT -> 0x" << std::hex << mem->dma[6].word_count << "\n";

				//16-bit transfer
				if(mem->dma[6].word_type == 0)
				{
					//Align addresses to half-word
					mem->dma[6].start_address &= ~0x1;
					mem->dma[6].destination_address	&= ~0x1;

					while(mem->dma[6].word_count != 0)
					{
						temp_value = mem->read_u16(mem->dma[6].start_address);
						mem->write_u16(mem->dma[6].destination_address, temp_value);

						//Update DMA2 Start Address
						if(mem->dma[6].src_addr_ctrl == 0) { mem->dma[6].start_address += 2; }
						else if(mem->dma[6].src_addr_ctrl == 1) { mem->dma[6].start_address -= 2; }
						else if(mem->dma[6].src_addr_ctrl == 3) { mem->dma[6].start_address += 2; }

						//Update DMA2 Destination Address
						if(mem->dma[6].dest_addr_ctrl == 0) { mem->dma[6].destination_address += 2; }
						else if(mem->dma[6].dest_addr_ctrl == 1) { mem->dma[6].destination_address -= 2; }
						else if(mem->dma[6].dest_addr_ctrl == 3) { mem->dma[6].destination_address += 2; }

						mem->dma[6].word_count--;
					}
				}

				//32-bit transfer
				else
				{
					//Align addresses to word
					mem->dma[6].start_address &= ~0x3;
					mem->dma[6].destination_address	&= ~0x3;

					while(mem->dma[6].word_count != 0)
					{
						temp_value = mem->read_u32(mem->dma[6].start_address);
						mem->write_u32(mem->dma[6].destination_address, temp_value);

						//Update DMA2 Start Address
						if(mem->dma[6].src_addr_ctrl == 0) { mem->dma[6].start_address += 4; }
						else if(mem->dma[6].src_addr_ctrl == 1) { mem->dma[6].start_address -= 4; }
						else if(mem->dma[6].src_addr_ctrl == 3) { mem->dma[6].start_address += 4; }

						//Update DMA2 Destination Address
						if(mem->dma[6].dest_addr_ctrl == 0) { mem->dma[6].destination_address += 4; }
						else if(mem->dma[6].dest_addr_ctrl == 1) { mem->dma[6].destination_address -= 4; }
						else if(mem->dma[6].dest_addr_ctrl == 3) { mem->dma[6].destination_address += 4; }

						mem->dma[6].word_count--;
					}
				}
			}

			mem->dma[6].control &= ~0x80000000;
			mem->write_u32(NDS_DMA2CNT, mem->dma[6].control);

			//Trigger IRQ
			if(mem->dma[6].control & 0x40000000) { mem->nds7_if |= 0x400; }

			break;

			case 0x1: std::cout<<"NDS7 DMA2 - VBlank\n"; break;
			case 0x2: std::cout<<"NDS7 DMA2 - DS Cart\n"; break;
			case 0x3: std::cout<<"NDS7 DMA2 - Wifi/GBA Cart\n"; break;

		}

		mem->dma[6].enable = false;

		if((mem->dma[6].control >> 28) & 0x7) { running = false; }
	}
}

/****** Performs DMA3 transfers - NDS7 ******/
void NTR_ARM7::dma3()
{
	//Wait 2 cycles after DMA is triggered before actual transfer
	if(mem->dma[7].delay != 0) { mem->dma[7].delay--; }

	//See if DMA Start Timing conditions dictate a transfer
	else
	{
		u32 temp_value = 0;
		u32 original_dest_addr = mem->dma[7].destination_address;

		if((mem->dma[7].control & 0x80000000) == 0) { mem->dma[7].enable = false; return; }

		//Check DMA Start Timings
		switch(((mem->dma[7].control >> 28) & 0x3))
		{
			case 0x0:
			{
				std::cout<<"NDS7 DMA3 - Immediate\n";
				std::cout<<"START ADDR -> 0x" << std::hex << mem->dma[7].start_address << "\n";
				std::cout<<"DEST  ADDR -> 0x" << std::hex << mem->dma[7].destination_address << "\n";
				std::cout<<"WORD COUNT -> 0x" << std::hex << mem->dma[7].word_count << "\n";

				//16-bit transfer
				if(mem->dma[7].word_type == 0)
				{
					//Align addresses to half-word
					mem->dma[7].start_address &= ~0x1;
					mem->dma[7].destination_address	&= ~0x1;

					while(mem->dma[7].word_count != 0)
					{
						temp_value = mem->read_u16(mem->dma[7].start_address);
						mem->write_u16(mem->dma[7].destination_address, temp_value);

						//Update DMA3 Start Address
						if(mem->dma[7].src_addr_ctrl == 0) { mem->dma[7].start_address += 2; }
						else if(mem->dma[7].src_addr_ctrl == 1) { mem->dma[7].start_address -= 2; }
						else if(mem->dma[7].src_addr_ctrl == 3) { mem->dma[7].start_address += 2; }

						//Update DMA3 Destination Address
						if(mem->dma[7].dest_addr_ctrl == 0) { mem->dma[7].destination_address += 2; }
						else if(mem->dma[7].dest_addr_ctrl == 1) { mem->dma[7].destination_address -= 2; }
						else if(mem->dma[7].dest_addr_ctrl == 3) { mem->dma[7].destination_address += 2; }

						mem->dma[7].word_count--;
					}
				}

				//32-bit transfer
				else
				{
					//Align addresses to word
					mem->dma[7].start_address &= ~0x3;
					mem->dma[7].destination_address	&= ~0x3;

					while(mem->dma[7].word_count != 0)
					{
						temp_value = mem->read_u32(mem->dma[7].start_address);
						mem->write_u32(mem->dma[7].destination_address, temp_value);

						//Update DMA3 Start Address
						if(mem->dma[7].src_addr_ctrl == 0) { mem->dma[7].start_address += 4; }
						else if(mem->dma[7].src_addr_ctrl == 1) { mem->dma[7].start_address -= 4; }
						else if(mem->dma[7].src_addr_ctrl == 3) { mem->dma[7].start_address += 4; }

						//Update DMA3 Destination Address
						if(mem->dma[7].dest_addr_ctrl == 0) { mem->dma[7].destination_address += 4; }
						else if(mem->dma[7].dest_addr_ctrl == 1) { mem->dma[7].destination_address -= 4; }
						else if(mem->dma[7].dest_addr_ctrl == 3) { mem->dma[7].destination_address += 4; }

						mem->dma[7].word_count--;
					}
				}
			}

			mem->dma[7].control &= ~0x80000000;
			mem->write_u32(NDS_DMA3CNT, mem->dma[7].control);

			//Trigger IRQ
			if(mem->dma[7].control & 0x40000000) { mem->nds7_if |= 0x800; }

			break;

			case 0x1: std::cout<<"NDS7 DMA3 - VBlank\n"; break;
			case 0x2: std::cout<<"NDS7 DMA3 - DS Cart\n"; break;
			case 0x3: std::cout<<"NDS7 DMA3 - Wifi/GBA Cart\n"; break;
		}

		mem->dma[7].enable = false;

		if((mem->dma[7].control >> 28) & 0x7) { running = false; }
	}
}
