// GB Enhanced+ Copyright Daniel Baxter 2014
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : dma.cpp
// Date : November 02, 2014
// Description : ARM7TDMI emulator
//
// Emulates the GBA's 4 DMA channels
// Transfers memory to different locations

#include "arm7.h" 

/****** Performs DMA0 transfers ******/
void ARM7::dma0()
{
	//Wait 2 cycles after DMA is triggered before actual transfer
	if(mem->dma[0].delay != 0) { mem->dma[0].delay--; }

	//See if DMA Start Timing conditions dictate a transfer
	else
	{
		mem->dma[0].start_address = (mem->read_u32(DMA0SAD) & 0x7FFFFFF);
		mem->dma[0].destination_address = (mem->read_u32(DMA0DAD) & 0x7FFFFFF);
		mem->dma[0].word_count = mem->read_u16(DMA0CNT_L);
		mem->dma[0].word_type = (mem->read_u16(DMA0CNT_H) & 0x400) ? 1 : 0;

		mem->dma[0].control = mem->read_u16(DMA0CNT_H);
		mem->dma[0].dest_addr_ctrl = (mem->read_u16(DMA0CNT_H) >> 5) & 0x3;
		mem->dma[0].src_addr_ctrl = (mem->read_u16(DMA0CNT_H) >> 7) & 0x3;

		u32 temp_value = 0;
		u32 original_src_addr = mem->dma[0].start_address;
		u32 original_dest_addr = mem->dma[0].destination_address;

		//Check DMA Start Timings
		switch(((mem->dma[0].control >> 12) & 0x3))
		{
			//Immediate
			case 0x0:
				//Set word count of transfer to max (0x10000) if specified as zero
				if(mem->dma[0].word_count == 0) { mem->dma[0].word_count = 0x10000; }

				//16-bit transfer
				if(mem->dma[0].word_type == 0)
				{
					while(mem->dma[0].word_count != 0)
					{
						temp_value = mem->read_u16(mem->dma[0].start_address);
						mem->write_u16(mem->dma[0].destination_address, temp_value);

						//Update DMA0 Start Address
						if(mem->dma[0].src_addr_ctrl == 0) { mem->dma[0].start_address += 2; }
						else if(mem->dma[0].src_addr_ctrl == 1) { mem->dma[0].start_address -= 2; }
						else if(mem->dma[0].src_addr_ctrl == 3) { mem->dma[0].start_address += 2; }

						//Update DMA0 Destination Address
						if(mem->dma[0].dest_addr_ctrl == 0) { mem->dma[0].destination_address += 2; }
						else if(mem->dma[0].dest_addr_ctrl == 1) { mem->dma[0].destination_address -= 2; }
						else if(mem->dma[0].dest_addr_ctrl == 3) { mem->dma[0].destination_address += 2; }

						mem->dma[0].word_count--;
					}
				}

				//32-bit transfer
				else
				{
					while(mem->dma[0].word_count != 0)
					{
						temp_value = mem->read_u32(mem->dma[0].start_address);
						mem->write_u32(mem->dma[0].destination_address, temp_value);

						//Update DMA0 Start Address
						if(mem->dma[0].src_addr_ctrl == 0) { mem->dma[0].start_address += 4; }
						else if(mem->dma[0].src_addr_ctrl == 1) { mem->dma[0].start_address -= 4; }
						else if(mem->dma[0].src_addr_ctrl == 3) { mem->dma[0].start_address += 4; }

						//Update DMA0 Destination Address
						if(mem->dma[0].dest_addr_ctrl == 0) { mem->dma[0].destination_address += 4; }
						else if(mem->dma[0].dest_addr_ctrl == 1) { mem->dma[0].destination_address -= 4; }
						else if(mem->dma[0].dest_addr_ctrl == 3) { mem->dma[0].destination_address += 4; }

						mem->dma[0].word_count--;
					}

					//Reload if control flags are set to 0x3
					if(mem->dma[0].src_addr_ctrl == 3) { mem->dma[0].src_addr_ctrl = original_src_addr; }
					if(mem->dma[0].dest_addr_ctrl == 3) { mem->dma[0].dest_addr_ctrl = original_dest_addr; }
				}

				mem->dma[0].enable = false;
				break;

			//VBlank
			case 0x1:
				std::cout<<"VBlank DMA0!\n";
				mem->dma[0].enable = false;
				break;

			//HBlank
			case 0x2:
				std::cout<<"HBlank DMA0!\n";
				mem->dma[0].enable = false;
				break;

			//Special
			case 0x3:
				std::cout<<"Special DMA0!\n";
				mem->dma[0].enable = false;
				break;
		}
	}
}

/****** Performs DMA3 transfers ******/
void ARM7::dma3()
{
	//Wait 2 cycles after DMA is triggered before actual transfer
	if(mem->dma[3].delay != 0) { mem->dma[3].delay--; }

	//See if DMA Start Timing conditions dictate a transfer
	else
	{
		mem->dma[3].start_address = (mem->read_u32(DMA3SAD) & 0xFFFFFFF);
		mem->dma[3].destination_address = (mem->read_u32(DMA3DAD) & 0xFFFFFFF);
		mem->dma[3].word_count = mem->read_u16(DMA3CNT_L);
		mem->dma[3].word_type = (mem->read_u16(DMA3CNT_H) & 0x400) ? 1 : 0;

		mem->dma[3].control = mem->read_u16(DMA3CNT_H);
		mem->dma[3].dest_addr_ctrl = (mem->read_u16(DMA3CNT_H) >> 5) & 0x3;
		mem->dma[3].src_addr_ctrl = (mem->read_u16(DMA3CNT_H) >> 7) & 0x3;

		u32 temp_value = 0;
		u32 original_src_addr = mem->dma[3].start_address;
		u32 original_dest_addr = mem->dma[3].destination_address;

		//Check DMA Start Timings
		switch(((mem->dma[3].control >> 12) & 0x3))
		{
			//Immediate
			case 0x0:
				//Set word count of transfer to max (0x10000) if specified as zero
				if(mem->dma[3].word_count == 0) { mem->dma[3].word_count = 0x10000; }

				//16-bit transfer
				if(mem->dma[3].word_type == 0)
				{
					while(mem->dma[3].word_count != 0)
					{
						temp_value = mem->read_u16(mem->dma[3].start_address);
						mem->write_u16(mem->dma[3].destination_address, temp_value);

						//Update DMA3 Start Address
						if(mem->dma[3].src_addr_ctrl == 0) { mem->dma[3].start_address += 2; }
						else if(mem->dma[3].src_addr_ctrl == 1) { mem->dma[3].start_address -= 2; }
						else if(mem->dma[3].src_addr_ctrl == 3) { mem->dma[3].start_address += 2; }

						//Update DMA3 Destination Address
						if(mem->dma[3].dest_addr_ctrl == 0) { mem->dma[3].destination_address += 2; }
						else if(mem->dma[3].dest_addr_ctrl == 1) { mem->dma[3].destination_address -= 2; }
						else if(mem->dma[3].dest_addr_ctrl == 3) { mem->dma[3].destination_address += 2; }

						mem->dma[3].word_count--;
					}
				}

				//32-bit transfer
				else
				{
					while(mem->dma[3].word_count != 0)
					{
						temp_value = mem->read_u32(mem->dma[3].start_address);
						mem->write_u32(mem->dma[3].destination_address, temp_value);

						//Update DMA3 Start Address
						if(mem->dma[3].src_addr_ctrl == 0) { mem->dma[3].start_address += 4; }
						else if(mem->dma[3].src_addr_ctrl == 1) { mem->dma[3].start_address -= 4; }
						else if(mem->dma[3].src_addr_ctrl == 3) { mem->dma[3].start_address += 4; }

						//Update DMA3 Destination Address
						if(mem->dma[3].dest_addr_ctrl == 0) { mem->dma[3].destination_address += 4; }
						else if(mem->dma[3].dest_addr_ctrl == 1) { mem->dma[3].destination_address -= 4; }
						else if(mem->dma[3].dest_addr_ctrl == 3) { mem->dma[3].destination_address += 4; }

						mem->dma[3].word_count--;
					}

					//Reload if control flags are set to 0x3
					if(mem->dma[3].src_addr_ctrl == 3) { mem->dma[3].src_addr_ctrl = original_src_addr; }
					if(mem->dma[3].dest_addr_ctrl == 3) { mem->dma[3].dest_addr_ctrl = original_dest_addr; }
				}

				mem->dma[3].enable = false;
				break;

			//VBlank
			case 0x1:
				std::cout<<"VBlank DMA3!\n";
				mem->dma[3].enable = false;
				break;

			//HBlank
			case 0x2:
				std::cout<<"HBlank DMA3!\n";
				mem->dma[3].enable = false;
				break;

			//Special
			case 0x3:
				std::cout<<"Special DMA3!\n";
				mem->dma[3].enable = false;
				break;
		}
	}
}
