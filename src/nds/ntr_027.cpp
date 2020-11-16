// GB Enhanced+ Copyright Daniel Baxter 2020
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : ntr_027.cpp
// Date : November 11, 2020
// Description : NTR-027 emulation
//
// Emulates the NTR-027 Activity Meter

#include "mmu.h"

/****** Calculates Checksum for IR data sent from NTR-027 ******/
u16 NTR_MMU::get_checksum()
{
	u32 sum = 0;

	for(u32 x = 1, a = 0; x < ntr_027.ir_stream.size(); x++, a++)
	{
		if(a & 0x1) { sum += (ntr_027.ir_stream[x] ^ 0xAA); }
		else { sum += ((ntr_027.ir_stream[x] ^ 0xAA) << 8); }
	}

	u32 adc = (sum & 0xFFFF0000) >> 16;
	sum += adc;
	sum &= 0xFFFF;

	return sum;
}

/****** Handles various commands sent to the NTR-027 ******/
void NTR_MMU::ntr_027_process()
{
	//Do nothing if comms have not started and no connection is active
	if(!ntr_027.connected && !ntr_027.start_comms) { return; }

	//Send IR Ping first
	if((!ntr_027.connected) && (ntr_027.start_comms) && (read_u16_fast(NDS_AUXSPIDATA) == 0xFF) && (nds_aux_spi.data == 0))
	{
		ntr_027.connected = true;
		ntr_027.start_comms = false;
		
		ntr_027.ir_stream.clear();
		ntr_027.ir_stream.push_back(0x01);
		ntr_027.ir_stream.push_back(0xFC ^ 0xAA);

		ntr_027.ir_counter = 0;
		ntr_027.state = 2;
	}

	switch(ntr_027.state)
	{
		//Receive data
		case 0:
			ntr_027.ir_stream.push_back(nds_aux_spi.data);
			break;

		
		//Parse received commands and begin sending data back
		case 1:
			ntr_027.command = ntr_027.ir_stream[0] ^ 0xAA;

			switch(ntr_027.command)
			{
				//EEPROM Read #1
				//EEPROM Read #2
				case 0x0A:
				case 0x22:
					{
						ntr_027.eeprom_addr = (ntr_027.ir_stream[5] ^ 0xAA);
						ntr_027.eeprom_addr |= ((ntr_027.ir_stream[4] ^ 0xAA) << 8);
						u8 read_length = (ntr_027.ir_stream[6] ^ 0xAA);

						//Build IR response
						ntr_027.ir_stream.clear();

						ntr_027.ir_stream.push_back(read_length + 4);
						ntr_027.ir_stream.push_back(ntr_027.command ^ 0xAA);
						ntr_027.ir_stream.push_back(0x00 ^ 0xAA);
						ntr_027.ir_stream.push_back(0x00 ^ 0xAA);
						ntr_027.ir_stream.push_back(0x00 ^ 0xAA);

						for(u32 x = 0; x < read_length; x++)
						{
							ntr_027.ir_stream.push_back(ntr_027.data[ntr_027.eeprom_addr++] ^ 0xAA);
						}

						u16 ir_sum = get_checksum();
						ntr_027.ir_stream[3] = (ir_sum & 0xFF) ^ 0xAA;
						ntr_027.ir_stream[4] = ((ir_sum >> 8) & 0xFF) ^ 0xAA;

						ntr_027.ir_counter = 0;
						ntr_027.state = 2;
					}
					
					break;

				//IR Handshake
				case 0xFA:
					ntr_027.ir_stream.clear();
					ntr_027.ir_stream.push_back(0x04);
					ntr_027.ir_stream.push_back(0xF8 ^ 0xAA);
					ntr_027.ir_stream.push_back(0x00 ^ 0xAA);
					ntr_027.ir_stream.push_back(0x00 ^ 0xAA);
					ntr_027.ir_stream.push_back(0xF8 ^ 0xAA);

					ntr_027.ir_counter = 0;
					ntr_027.state = 2;
					break;

				//Disconnect
				case 0xF4:
					ntr_027.connected = false;
					ntr_027.ir_stream.clear();

					ntr_027.ir_counter = 0;
					ntr_027.state = 0;
					break;

				default:
					std::cout<<"NTR-027::Unknown command 0x" << (u32)ntr_027.command << "\n";
			}

			break;

		//Send data
		case 2:
			nds_aux_spi.data = ntr_027.ir_stream[ntr_027.ir_counter++];
			
			if(ntr_027.ir_counter == ntr_027.ir_stream.size())
			{
				ntr_027.state = 0;
				ntr_027.ir_stream.clear();
			}

			break;
	}
}
	