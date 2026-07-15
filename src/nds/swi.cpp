// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : swi.cpp
// Date : November 05, 2015
// Description : NDS ARM7-ARM9 Software Interrupts
//
// Emulates the NDS's Software Interrupts via High Level Emulation

#include <cmath>

#include "arm9.h"
#include "arm7.h"

u8 vol_lut[724] =
{
	0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
	0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
	0x09, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B, 0x0B,
	0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0E,
	0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0F, 0x0F,	0x0F, 0x0F, 0x0F, 0x10, 0x10, 0x10, 0x10, 0x10,
	0x10, 0x11, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x14,
	0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x18,
	0x18, 0x18, 0x18, 0x19, 0x19, 0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C,
	0x1D, 0x1D, 0x1D, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x20, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22,
	0x22, 0x23, 0x23, 0x24, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 0x27, 0x28, 0x28, 0x29,
	0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D, 0x2D, 0x2E, 0x2E, 0x2F, 0x2F, 0x30, 0x31, 0x31,
	0x32, 0x32, 0x33, 0x33, 0x34, 0x35, 0x35, 0x36, 0x36, 0x37, 0x38, 0x38, 0x39, 0x3A, 0x3A, 0x3B,
	0x3C, 0x3C, 0x3D, 0x3E, 0x3F, 0x3F, 0x40, 0x41, 0x42, 0x42, 0x43, 0x44, 0x45, 0x45, 0x46, 0x47,
	0x48, 0x49, 0x4A, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x52, 0x53, 0x54, 0x55,
	0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67,
	0x68, 0x69, 0x6A, 0x6B, 0x6D, 0x6E, 0x6F, 0x71, 0x72, 0x73, 0x75, 0x76, 0x77, 0x79, 0x7A, 0x7B,
	0x7D, 0x7E, 0x7F, 0x20, 0x21, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25,
	0x26, 0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 0x29, 0x29, 0x2A, 0x2A, 0x2B, 0x2B, 0x2C, 0x2C, 0x2D,
	0x2D, 0x2E, 0x2E, 0x2F, 0x2F, 0x30, 0x30, 0x31, 0x31, 0x32, 0x33, 0x33, 0x34, 0x34, 0x35, 0x36,
	0x36, 0x37, 0x37, 0x38, 0x39, 0x39, 0x3A, 0x3B, 0x3B, 0x3C, 0x3D, 0x3E, 0x3E, 0x3F, 0x40, 0x40,
	0x41, 0x42, 0x43, 0x43, 0x44, 0x45, 0x46, 0x47, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4D,
	0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,
	0x5E, 0x5F, 0x60, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6F, 0x70,
	0x71, 0x73, 0x74, 0x75, 0x77, 0x78, 0x79, 0x7B, 0x7C, 0x7E, 0x7E, 0x40, 0x41, 0x42, 0x43, 0x43,
	0x44, 0x45, 0x46, 0x47, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51,
	0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61,
	0x62, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6B, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x72, 0x74, 0x75,
	0x76, 0x78, 0x79, 0x7B, 0x7C, 0x7D, 0x7E, 0x40, 0x41, 0x42, 0x42, 0x43, 0x44, 0x45, 0x46, 0x46,
	0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
	0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x65, 0x66,
	0x67, 0x68, 0x69, 0x6A, 0x6C, 0x6D, 0x6E, 0x6F, 0x71, 0x72, 0x73, 0x75, 0x76, 0x77, 0x79, 0x7A,
	0x7C, 0x7D, 0x7E, 0x7F
};

u16 pitch_lut[768]
{
	0x0000, 0x003B, 0x0076, 0x00B2, 0x00ED, 0x0128, 0x0164, 0x019F,
	0x01DB, 0x0217, 0x0252, 0x028E, 0x02CA, 0x0305, 0x0341, 0x037D,
	0x03B9, 0x03F5, 0x0431, 0x046E, 0x04AA, 0x04E6, 0x0522, 0x055F,
	0x059B, 0x05D8, 0x0614, 0x0651, 0x068D, 0x06CA, 0x0707, 0x0743,
	0x0780, 0x07BD, 0x07FA, 0x0837, 0x0874, 0x08B1, 0x08EF, 0x092C,
	0x0969, 0x09A7, 0x09E4, 0x0A21, 0x0A5F, 0x0A9C, 0x0ADA, 0x0B18,
	0x0B56, 0x0B93, 0x0BD1, 0x0C0F, 0x0C4D, 0x0C8B, 0x0CC9, 0x0D07,
	0x0D45, 0x0D84, 0x0DC2, 0x0E00, 0x0E3F, 0x0E7D, 0x0EBC, 0x0EFA,
	0x0F39, 0x0F78, 0x0FB6, 0x0FF5, 0x1034, 0x1073, 0x10B2, 0x10F1,
	0x1130, 0x116F, 0x11AE, 0x11EE, 0x122D, 0x126C, 0x12AC, 0x12EB,
	0x132B, 0x136B, 0x13AA, 0x13EA, 0x142A, 0x146A, 0x14A9, 0x14E9,
	0x1529, 0x1569, 0x15AA, 0x15EA, 0x162A, 0x166A, 0x16AB, 0x16EB,
	0x172C, 0x176C, 0x17AD, 0x17ED, 0x182E, 0x186F, 0x18B0, 0x18F0,
	0x1931, 0x1972, 0x19B3, 0x19F5, 0x1A36, 0x1A77, 0x1AB8, 0x1AFA,
	0x1B3B, 0x1B7D, 0x1BBE, 0x1C00, 0x1C41, 0x1C83, 0x1CC5, 0x1D07,
	0x1D48, 0x1D8A, 0x1DCC, 0x1E0E, 0x1E51, 0x1E93, 0x1ED5, 0x1F17,
	0x1F5A, 0x1F9C, 0x1FDF, 0x2021, 0x2064, 0x20A6, 0x20E9, 0x212C,
	0x216F, 0x21B2, 0x21F5, 0x2238, 0x227B, 0x22BE, 0x2301, 0x2344,
	0x2388, 0x23CB, 0x240E, 0x2452, 0x2496, 0x24D9, 0x251D, 0x2561,
	0x25A4, 0x25E8, 0x262C, 0x2670, 0x26B4, 0x26F8, 0x273D, 0x2781,
	0x27C5, 0x280A, 0x284E, 0x2892, 0x28D7, 0x291C, 0x2960, 0x29A5,
	0x29EA, 0x2A2F, 0x2A74, 0x2AB9, 0x2AFE, 0x2B43, 0x2B88, 0x2BCD,
	0x2C13, 0x2C58, 0x2C9D, 0x2CE3, 0x2D28, 0x2D6E, 0x2DB4, 0x2DF9,
	0x2E3F, 0x2E85, 0x2ECB, 0x2F11, 0x2F57, 0x2F9D, 0x2FE3, 0x302A,
	0x3070, 0x30B6, 0x30FD, 0x3143, 0x318A, 0x31D0, 0x3217, 0x325E,
	0x32A5, 0x32EC, 0x3332, 0x3379, 0x33C1, 0x3408, 0x344F, 0x3496,
	0x34DD, 0x3525, 0x356C, 0x35B4, 0x35FB, 0x3643, 0x368B, 0x36D3,
	0x371A, 0x3762, 0x37AA, 0x37F2, 0x383A, 0x3883, 0x38CB, 0x3913,
	0x395C, 0x39A4, 0x39ED, 0x3A35, 0x3A7E, 0x3AC6, 0x3B0F, 0x3B58,
	0x3BA1, 0x3BEA, 0x3C33, 0x3C7C, 0x3CC5, 0x3D0E, 0x3D58, 0x3DA1,
	0x3DEA, 0x3E34, 0x3E7D, 0x3EC7, 0x3F11, 0x3F5A, 0x3FA4, 0x3FEE,
	0x4038, 0x4082, 0x40CC, 0x4116, 0x4161, 0x41AB, 0x41F5, 0x4240,
	0x428A, 0x42D5, 0x431F, 0x436A, 0x43B5, 0x4400, 0x444B, 0x4495,
	0x44E1, 0x452C, 0x4577, 0x45C2, 0x460D, 0x4659, 0x46A4, 0x46F0,
	0x473B, 0x4787, 0x47D3, 0x481E, 0x486A, 0x48B6, 0x4902, 0x494E,
	0x499A, 0x49E6, 0x4A33, 0x4A7F, 0x4ACB, 0x4B18, 0x4B64, 0x4BB1,
	0x4BFE, 0x4C4A, 0x4C97, 0x4CE4, 0x4D31, 0x4D7E, 0x4DCB, 0x4E18,
	0x4E66, 0x4EB3, 0x4F00, 0x4F4E, 0x4F9B, 0x4FE9, 0x5036, 0x5084,
	0x50D2, 0x5120, 0x516E, 0x51BC, 0x520A, 0x5258, 0x52A6, 0x52F4,
	0x5343, 0x5391, 0x53E0, 0x542E, 0x547D, 0x54CC, 0x551A, 0x5569,
	0x55B8, 0x5607, 0x5656, 0x56A5, 0x56F4, 0x5744, 0x5793, 0x57E2,
	0x5832, 0x5882, 0x58D1, 0x5921, 0x5971, 0x59C1, 0x5A10, 0x5A60,
	0x5AB0, 0x5B01, 0x5B51, 0x5BA1, 0x5BF1, 0x5C42, 0x5C92, 0x5CE3,
	0x5D34, 0x5D84, 0x5DD5, 0x5E26, 0x5E77, 0x5EC8, 0x5F19, 0x5F6A,
	0x5FBB, 0x600D, 0x605E, 0x60B0, 0x6101, 0x6153, 0x61A4, 0x61F6,
	0x6248, 0x629A, 0x62EC, 0x633E, 0x6390, 0x63E2, 0x6434, 0x6487,
	0x64D9, 0x652C, 0x657E, 0x65D1, 0x6624, 0x6676, 0x66C9, 0x671C,
	0x676F, 0x67C2, 0x6815, 0x6869, 0x68BC, 0x690F, 0x6963, 0x69B6,
	0x6A0A, 0x6A5E, 0x6AB1, 0x6B05, 0x6B59, 0x6BAD, 0x6C01, 0x6C55,
	0x6CAA, 0x6CFE, 0x6D52, 0x6DA7, 0x6DFB, 0x6E50, 0x6EA4, 0x6EF9,
	0x6F4E, 0x6FA3, 0x6FF8, 0x704D, 0x70A2, 0x70F7, 0x714D, 0x71A2,
	0x71F7, 0x724D, 0x72A2, 0x72F8, 0x734E, 0x73A4, 0x73FA, 0x7450,
	0x74A6, 0x74FC, 0x7552, 0x75A8, 0x75FF, 0x7655, 0x76AC, 0x7702,
	0x7759, 0x77B0, 0x7807, 0x785E, 0x78B4, 0x790C, 0x7963, 0x79BA,
	0x7A11, 0x7A69, 0x7AC0, 0x7B18, 0x7B6F, 0x7BC7, 0x7C1F, 0x7C77,
	0x7CCF, 0x7D27, 0x7D7F, 0x7DD7, 0x7E2F, 0x7E88, 0x7EE0, 0x7F38,
	0x7F91, 0x7FEA, 0x8042, 0x809B, 0x80F4, 0x814D, 0x81A6, 0x81FF,
	0x8259, 0x82B2, 0x830B, 0x8365, 0x83BE, 0x8418, 0x8472, 0x84CB,
	0x8525, 0x857F, 0x85D9, 0x8633, 0x868E, 0x86E8, 0x8742, 0x879D,
	0x87F7, 0x8852, 0x88AC, 0x8907, 0x8962, 0x89BD, 0x8A18, 0x8A73,
	0x8ACE, 0x8B2A, 0x8B85, 0x8BE0, 0x8C3C, 0x8C97, 0x8CF3, 0x8D4F,
	0x8DAB, 0x8E07, 0x8E63, 0x8EBF, 0x8F1B, 0x8F77, 0x8FD4, 0x9030,
	0x908C, 0x90E9, 0x9146, 0x91A2, 0x91FF, 0x925C, 0x92B9, 0x9316,
	0x9373, 0x93D1, 0x942E, 0x948C, 0x94E9, 0x9547, 0x95A4, 0x9602,
	0x9660, 0x96BE, 0x971C, 0x977A, 0x97D8, 0x9836, 0x9895, 0x98F3,
	0x9952, 0x99B0, 0x9A0F, 0x9A6E, 0x9ACD, 0x9B2C, 0x9B8B, 0x9BEA,
	0x9C49, 0x9CA8, 0x9D08, 0x9D67, 0x9DC7, 0x9E26, 0x9E86, 0x9EE6,
	0x9F46, 0x9FA6, 0xA006, 0xA066, 0xA0C6, 0xA127, 0xA187, 0xA1E8,
	0xA248, 0xA2A9, 0xA30A, 0xA36B, 0xA3CC, 0xA42D, 0xA48E, 0xA4EF,
	0xA550, 0xA5B2, 0xA613, 0xA675, 0xA6D6, 0xA738, 0xA79A, 0xA7FC,
	0xA85E, 0xA8C0, 0xA922, 0xA984, 0xA9E7, 0xAA49, 0xAAAC, 0xAB0E,
	0xAB71, 0xABD4, 0xAC37, 0xAC9A, 0xACFD, 0xAD60, 0xADC3, 0xAE27,
	0xAE8A, 0xAEED, 0xAF51, 0xAFB5, 0xB019, 0xB07C, 0xB0E0, 0xB145,
	0xB1A9, 0xB20D, 0xB271, 0xB2D6, 0xB33A, 0xB39F, 0xB403, 0xB468,
	0xB4CD, 0xB532, 0xB597, 0xB5FC, 0xB662, 0xB6C7, 0xB72C, 0xB792,
	0xB7F7, 0xB85D, 0xB8C3, 0xB929, 0xB98F, 0xB9F5, 0xBA5B, 0xBAC1,
	0xBB28, 0xBB8E, 0xBBF5, 0xBC5B, 0xBCC2, 0xBD29, 0xBD90, 0xBDF7,
	0xBE5E, 0xBEC5, 0xBF2C, 0xBF94, 0xBFFB, 0xC063, 0xC0CA, 0xC132,
	0xC19A, 0xC202, 0xC26A, 0xC2D2, 0xC33A, 0xC3A2, 0xC40B, 0xC473,
	0xC4DC, 0xC544, 0xC5AD, 0xC616, 0xC67F, 0xC6E8, 0xC751, 0xC7BB,
	0xC824, 0xC88D, 0xC8F7, 0xC960, 0xC9CA, 0xCA34, 0xCA9E, 0xCB08,
	0xCB72, 0xCBDC, 0xCC47, 0xCCB1, 0xCD1B, 0xCD86, 0xCDF1, 0xCE5B,
	0xCEC6, 0xCF31, 0xCF9C, 0xD008, 0xD073, 0xD0DE, 0xD14A, 0xD1B5,
	0xD221, 0xD28D, 0xD2F8, 0xD364, 0xD3D0, 0xD43D, 0xD4A9, 0xD515,
	0xD582, 0xD5EE, 0xD65B, 0xD6C7, 0xD734, 0xD7A1, 0xD80E, 0xD87B,
	0xD8E9, 0xD956, 0xD9C3, 0xDA31, 0xDA9E, 0xDB0C, 0xDB7A, 0xDBE8,
	0xDC56, 0xDCC4, 0xDD32, 0xDDA0, 0xDE0F, 0xDE7D, 0xDEEC, 0xDF5B,
	0xDFC9, 0xE038, 0xE0A7, 0xE116, 0xE186, 0xE1F5, 0xE264, 0xE2D4,
	0xE343, 0xE3B3, 0xE423, 0xE493, 0xE503, 0xE573, 0xE5E3, 0xE654,
	0xE6C4, 0xE735, 0xE7A5, 0xE816, 0xE887, 0xE8F8, 0xE969, 0xE9DA,
	0xEA4B, 0xEABC, 0xEB2E, 0xEB9F, 0xEC11, 0xEC83, 0xECF5, 0xED66,
	0xEDD9, 0xEE4B, 0xEEBD, 0xEF2F, 0xEFA2, 0xF014, 0xF087, 0xF0FA,
	0xF16D, 0xF1E0, 0xF253, 0xF2C6, 0xF339, 0xF3AD, 0xF420, 0xF494,
	0xF507, 0xF57B, 0xF5EF, 0xF663, 0xF6D7, 0xF74C, 0xF7C0, 0xF834,
	0xF8A9, 0xF91E, 0xF992, 0xFA07, 0xFA7C, 0xFAF1, 0xFB66, 0xFBDC,
	0xFC51, 0xFCC7, 0xFD3C, 0xFDB2, 0xFE28, 0xFE9E, 0xFF14, 0xFF8A
};

/****** Process Software Interrupts - NDS9 ******/
void NTR_ARM9::process_swi(u32 comment)
{
	switch(comment)
	{
		//SoftReset
		case 0x0:
			std::cout<<"ARM9::SWI::SoftReset \n";
			swi_softreset();
			break;

		//WaitByLoop
		case 0x3:
			//std::cout<<"ARM9::SWI::WaitByLoop \n";
			swi_waitbyloop();
			break;

		//IntrWait
		case 0x4:
			std::cout<<"ARM9::SWI::IntrWait \n";
			swi_intrwait();
			break;

		//VBlankIntrWait
		case 0x5:
			//std::cout<<"ARM9::SWI::VBlankIntrWait \n";
			swi_vblankintrwait();
			break;

		//Halt
		case 0x6:
			std::cout<<"ARM9::SWI::Halt \n";
			swi_halt();
			break;

		//Div
		case 0x9:
			std::cout<<"ARM9::SWI::Div \n";
			swi_div();
			break;

		//CPUSet
		case 0xB:
			//std::cout<<"ARM9::SWI::CPUSet \n";
			swi_cpuset();
			break;

		//CPUFastSet
		case 0xC:
			//std::cout<<"ARM9::SWI::CPU Fast Set \n";
			swi_cpufastset();
			break;

		//Sqrt
		case 0xD:
			std::cout<<"ARM9::SWI::Sqrt \n";
			swi_sqrt();
			break;

		//GetCRC16
		case 0xE:
			//std::cout<<"ARM9::SWI::GetCRC16 \n";
			swi_getcrc16();
			break;

		//IsDebugger
		case 0xF:
			//std::cout<<"ARM9::SWI::IsDebugger \n";
			swi_isdebugger();
			break;

		//BitUnPack
		case 0x10:
			std::cout<<"ARM9::SWI::BitUnpack \n";
			swi_bitunpack();
			break;

		//LZ77UnCompWram
		case 0x11:
			std::cout<<"ARM9::SWI::LZ77UnCompWram \n";
			swi_lz77uncompvram();
			break;

		//LZ77UnCompReadByCallback
		case 0x12:
			std::cout<<"ARM9::SWI::LZ77UnCompReadByCallback \n";
			swi_lz77uncompvram();
			break;

		//RLUnCompReadNormalWrite8Bit
		case 0x14:
			std::cout<<"ARM9::SWI::LUnCompReadNormalWrite8Bit \n";
			swi_rluncompvram();
			break;

		//RLUnCompReadByCallback
		case 0x15:
			std::cout<<"ARM9::SWI::RLUnCompReadByCallback \n";
			swi_rluncompvram();
			break;

		//CustomPost
		case 0x1F:
			std::cout<<"ARM9::SWI::CustomPost \n";
			swi_custompost();
			break;

		default:
			std::cout<<"SWI::Error - Unknown NDS9 BIOS function 0x" << std::hex << comment << "\n";
			running = false;
			break;
	}
}

/****** HLE implementation of SoftReset - NDS9 ******/
void NTR_ARM9::swi_softreset()
{
	//Reset IRQ, SVC, and SYS stack pointers
	reg.r13_svc = 0x0803FC0;
	reg.r13_irq = 0x0803FA0;
	reg.r13 = 0x0803EC0;

	//Set PC to return address at 0x27FFE24
	reg.r15 = mem->read_u32(0x27FFE24);

	//Switch to ARM or THUMB mode as necessary
	if(reg.r15 & 0x1) { arm_mode = THUMB; }
	else { arm_mode = ARM; }	

	needs_flush = true;
	in_interrupt = false;

	//Set registers R0-R12 to zero
	for(int x = 0; x <= 12; x++) { set_reg(x, 0); }

	//Set R14_svc, R14_irq to zero, R14 to the return address
	reg.r14_svc = 0;
	reg.r14_irq = 0;

	//Set SPSR_svc and SPSR_irq to zero
	reg.spsr_svc = 0;
	reg.spsr_irq = 0;

	//Set mode to SYS
	current_cpu_mode = SYS;
	reg.cpsr &= ~0x1F;
	reg.cpsr |= 0x1F;

	//Clear top 0x200 bytes of the DTCM
	for(u16 x = 0; x < 0x200; x++) { mem->memory_map[mem->nds9_irq_handler + 0x3E00 + x] = 0; }

	//Clear internal input
	mem->g_pad->clear_input();

	//TODO - Flush caches + write buffer, setup CP15
}

/****** HLE implementation of WaitByLoop - NDS9 ******/
void NTR_ARM9::swi_waitbyloop()
{
	//Setup the initial value for swi_waitbyloop_count - R0
	swi_waitbyloop_count = get_reg(0) & 0x7FFFFFFF;
	swi_waitbyloop_count >>= 2;

	//Set CPU idle state to 2
	idle_state = 2;
}

/****** HLE implementation of Halt - NDS9 ******/
void NTR_ARM9::swi_halt()
{
	//Set CPU idle state to 1
	idle_state = 1;
	last_idle_state = 1;
	mem->nds9_temp_if = mem->nds9_if;
	mem->nds9_if = 0;

	//Destroy R0
	set_reg(0, 0);
}

/****** HLE implementation of IntrWait - NDS9 ******/
void NTR_ARM9::swi_intrwait()
{
	//NDS9 version is slightly bugged. When R0 == 0, it simply waits for any new interrupt, then leaves
	//Normally, it should return immediately if the flags in R1 are already set in IF
	//R0 == 1 will wait for the specified flags in R1

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Create temporary IF for flags in R1
	mem->nds9_temp_if = reg.r1;

	if(reg.r0) { mem->nds9_if &= ~mem->nds9_temp_if; }

	//Set CPU idle state to 3
	idle_state = 3;
	last_idle_state = 3;
}

/****** HLE implementation of VBlankIntrWait - NDS9 ******/
void NTR_ARM9::swi_vblankintrwait()
{
	//This is basically the IntrWait SWI, but R0 and R1 are both set to 1
	reg.r0 = 1;
	reg.r1 = 1;

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Create temporary IF for flags in R1
	mem->nds9_temp_if = reg.r1;
	mem->nds9_if &= ~mem->nds9_temp_if;

	//Set CPU idle state to 3
	idle_state = 3;
	last_idle_state = 3;
}

/****** HLE implementation of Div - NDS9 ******/
void NTR_ARM9::swi_div()
{
	//Grab the numerator - R0
	s32 num = get_reg(0);
	
	//Grab the denominator - R1
	s32 den = get_reg(1);

	s32 result = 0;
	s32 modulo = 0;

	//Do NOT divide by 0
	if(den == 0)
	{
		std::cout<<"ARM9::SWI::Warning - Div tried to divide by zero (ignoring operation) \n";
		return;
	}

	//Special case of -MAX / -1. Same results as NDS math coprocessor
	//Result simply equals -MAX, so here denominator is just set to 1
	if((num == 0x80000000) && (den == 0xFFFFFFFF))
	{
		std::cout<<"SWI::Warning - Div used -MAX/-1 \n";
		den = 1;
	}

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

/****** HLE implementation of CPUSet - NDS9 ******/
void NTR_ARM9::swi_cpuset()
{
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

	src_addr &= (transfer_type == 0) ? ~0x1 : ~0x3;
	dest_addr &= (transfer_type == 0) ? ~0x1 : ~0x3; 

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

	//Write-back R0, R1
	set_reg(0, src_addr);
	set_reg(1, dest_addr);
}

/****** HLE implementation of CPUFastSet - NDS9 ******/
void NTR_ARM9::swi_cpufastset()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	src_addr &= ~0x3;
	dest_addr &= ~0x3;

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

	//Write-back R0, R1
	set_reg(0, src_addr);
	set_reg(1, dest_addr);
}

/****** HLE implementation of Sqrt - NDS9 ******/
void NTR_ARM9::swi_sqrt()
{
	//Grab input
	u32 input = get_reg(0);

	//Set result of operation
	u16 result = sqrt(input);
	set_reg(0, result);
}

/****** HLE implementation of GetCRC16 - NDS9 ******/
void NTR_ARM9::swi_getcrc16()
{
	//R0 = Initial CRC value
	//R1 = Start address of data to look at
	//R2 = Length of data to look at in bytes
	u16 crc = get_reg(0);
	u32 data_addr = get_reg(1) & ~0x1;
	u32 length = get_reg(2) >> 1;

	//LUT for CRC
	u16 table[] = { 0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400 };
	u16 crc_val;

	//Cycle through all the data to get the CRC16
	for(u32 x = 0; x < length; x++)
	{
		crc_val = mem->read_u16(data_addr);

		for(u32 y = 0; y < 4; y++)
		{
			u16 lut_val = table[crc & 0xF];
			crc >>= 4;
			crc ^= lut_val;

			u16 temp = crc_val >> (4 * y);
			crc ^= table[temp & 0xF];
		}

		data_addr += 2;
	}

	set_reg(0, crc);
}

/****** HLE implementation of IsDebugger - NDS9 ******/
void NTR_ARM9::swi_isdebugger()
{
	//Always act as if a retail NDS, set RO to zero
	set_reg(0, 0);

	//Destroy value at 0x27FFFF8 (halfword)
	mem->write_u16(0x27FFFF8, 0x0);
}

/****** HLE implementation of BitUnPack - NDS9******/
void NTR_ARM9::swi_bitunpack()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1;
	u32 dest_addr = get_reg(1);

	//Grab pointer to unpack info - R2;
	u32 unpack_info_addr = get_reg(2);

	//Grab the length
	u16 length = mem->read_u16(unpack_info_addr);
	unpack_info_addr += 2;

	//Grab the source width
	u8 src_width = mem->read_u8(unpack_info_addr);
	unpack_info_addr++;

	//Grab the destination width
	u8 dest_width = mem->read_u8(unpack_info_addr);
	unpack_info_addr++;

	if(src_width > dest_width)
	{
		std::cout<<"ARM9::SWI::ERROR - BitUnPack source width is greater than destination width\n";
		return;
	}

	u8 bit_mask = 0;

	switch(src_width)
	{
		case 1: bit_mask = 0x1; break;
		case 2: bit_mask = 0x3; break;
		case 4: bit_mask = 0xF; break;
		case 8: bit_mask = 0xFF; break;
		default: std::cout<<"ARM9::SWI::ERROR - Invalid source width\n"; return;
	}

	//Grab the data offset and zero flag
	u32 data_offset = mem->read_u32(unpack_info_addr);
	u8 zero_flag = (data_offset & 0x80000000) ? 1 : 0;
	data_offset &= ~0x80000000;

	u8 src_byte = 0;
	u8 src_count = 0;
	u32 result = 0;

	//Decompress bytes from source addr
	while(length > 0)
	{
		result = 0;

		//Cycle through the byte and expand to destination width
		for(u8 x = 0; x < 32; x += dest_width)
		{
			//Grab new source byte
			if((src_count % 8) == 0) 
			{
				src_byte = mem->read_u8(src_addr++);
				length--;
			}

			//Grab the slice
			u32 slice = (src_byte & bit_mask);
			src_byte >>= src_width;
			src_count += src_width;

			if(slice != 0) { slice += data_offset; }
			else if ((slice == 0) && (zero_flag == 1)) { slice += data_offset; }

			//OR the slice to the final result
			result |= (slice << x);
		}

		//Write result to the destination address
		mem->write_u32(dest_addr, result);
		dest_addr += 4;
	}
}	

/****** HLE implementation of LZ77UnCompReadByCallback - NDS9 ******/
void NTR_ARM9::swi_lz77uncompvram()
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

/****** HLE implementation of RLUnCompVram - NDS9 ******/
void NTR_ARM9::swi_rluncompvram()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab data header
	u32 data_header = mem->read_u32(src_addr);

	u32 data_size = (data_header >> 8);

	//Data pointer to compressed data. Points to first flag.
	u32 data_ptr = (src_addr + 4);

	//Uncompress data
	while(data_size > 0)
	{
		u8 flag = mem->read_u8(data_ptr++);

		u8 data_length = (flag & 0x7F);

		//Adjust data length, +1 for uncompressed data, +3 for compressed data
		if(flag & 0x80) { data_length += 3; }
		else { data_length += 1; }

		//Output the specified byte the amount of times in data_length
		for(int x = 0; x < data_length; x++)
		{
			u8 data_byte = 0;

			//Compressed
			if(flag & 0x80) { data_byte = mem->read_u8(data_ptr); }

			//Uncompressed
			else { data_byte = mem->read_u8(data_ptr++); }
				
			mem->write_u8(dest_addr++, data_byte);
			data_size--;

			if(data_size == 0) { return; }
		}

		//Manually adjust data pointer for compressed data to point to next flag
		if(flag & 0x80) { data_ptr++; }
	}
}

/****** HLE implementation of CustomPost - NDS9 ******/
void NTR_ARM9::swi_custompost()
{
	mem->write_u32(NDS_POSTFLG, reg.r0);
}

/****** Process Software Interrupts - NDS7 ******/
void NTR_ARM7::process_swi(u32 comment)
{
	switch(comment)
	{
		//SoftReset
		case 0x0:
			std::cout<<"ARM7::SWI::SoftReset \n";
			swi_softreset();
			break;

		//WaitByLoop
		case 0x3:
			//std::cout<<"ARM7::SWI::WaitByLoop \n";
			swi_waitbyloop();
			break;

		//IntrWait
		case 0x4:
			//std::cout<<"ARM9::SWI::IntrWait \n";
			swi_intrwait();
			break;

		//VBlankIntrWait
		case 0x5:
			//std::cout<<"ARM7::SWI::VBlankIntrWait \n";
			swi_vblankintrwait();
			break;

		//Halt
		case 0x6:
			//std::cout<<"ARM7::SWI::Halt \n";
			swi_halt();
			break;

		//SoundBias
		case 0x8:
			std::cout<<"ARM7::SWI::SoundBias \n";
			swi_soundbias();
			break;

		//Div
		case 0x9:
			std::cout<<"ARM7::SWI::Div \n";
			swi_div();
			break;

		//CPUSet
		case 0xB:
			std::cout<<"ARM7::SWI::CPUSet \n";
			swi_cpuset();
			break;

		//CPUFastSet
		case 0xC:
			std::cout<<"ARM7::SWI::CPU Fast Set \n";
			swi_cpufastset();
			break;

		//Sqrt
		case 0xD:
			std::cout<<"ARM7::SWI::Sqrt \n";
			swi_sqrt();
			break;

		//GetCRC16
		case 0xE:
			//std::cout<<"ARM7::SWI::GetCRC16 \n";
			swi_getcrc16();
			break;

		//IsDebugger
		case 0xF:
			std::cout<<"ARM7::SWI::IsDebugger \n";
			swi_isdebugger();
			break;

		//BitUnPack
		case 0x10:
			std::cout<<"ARM7::SWI::BitUnpack \n";
			swi_bitunpack();
			break;

		//LZ77UnCompReadByCallback
		case 0x12:
			std::cout<<"ARM7::SWI::LZ77UnCompReadByCallback \n";
			swi_lz77uncompvram();
			break;

		//RLUnCompReadNormalWrite8Bit
		case 0x14:
			std::cout<<"ARM7::SWI::LUnCompReadNormalWrite8Bit \n";
			swi_rluncompvram();
			break;

		//RLUnCompReadByCallback
		case 0x15:
			std::cout<<"ARM7::SWI::RLUnCompReadByCallback \n";
			swi_rluncompvram();
			break;

		//GetSineTable
		case 0x1A:
			std::cout<<"ARM7::SWI::GetSineTable\n";
			swi_getsinetable();
			break;

		//GetPitchTable
		case 0x1B:
			std::cout<<"ARM7::SWI::GetPitchTable\n";
			swi_getpitchtable();
			break;

		//GetVolumeTable
		case 0x1C:
			std::cout<<"ARM7::SWI::GetVolumeTable \n";
			swi_getvolumetable();
			break;

		//CustomHalt
		case 0x1F:
			std::cout<<"ARM7::SWI::CustomHalt \n";
			swi_customhalt();
			break;
			
		default:
			std::cout<<"SWI::Error - Unknown NDS7 BIOS function 0x" << std::hex << comment << "\n";
			running = false;
			break;
	}
}

/****** HLE implementation of SoftReset - NDS7 ******/
void NTR_ARM7::swi_softreset()
{
	//Reset IRQ, SVC, and SYS stack pointers
	reg.r13_svc = 0x380FFDC;
	reg.r13_irq = 0x380FFB0;
	reg.r13 = 0x380FF00;

	//Set PC to return address at 0x27FFE34
	reg.r15 = mem->read_u32(0x27FFE34);

	//Switch to ARM or THUMB mode as necessary
	if(reg.r15 & 0x1) { arm_mode = THUMB; }
	else { arm_mode = ARM; }	

	needs_flush = true;
	in_interrupt = false;

	//Set registers R0-R12 to zero
	for(int x = 0; x <= 12; x++) { set_reg(x, 0); }

	//Set R14_svc, R14_irq to zero, R14 to the return address
	reg.r14_svc = 0;
	reg.r14_irq = 0;

	//Set SPSR_svc and SPSR_irq to zero
	reg.spsr_svc = 0;
	reg.spsr_irq = 0;

	//Set mode to SYS
	current_cpu_mode = SYS;
	reg.cpsr &= ~0x1F;
	reg.cpsr |= 0x1F;

	//Clear top 0x200 bytes of some RAM
	for(int x = 0x380FE00; x < 3810000; x++) { mem->memory_map[x] = 0; }

	//Clear internal input
	mem->g_pad->clear_input();
}

/****** HLE implementation of GetCRC16 - NDS7 ******/
void NTR_ARM7::swi_getcrc16()
{
	//R0 = Initial CRC value
	//R1 = Start address of data to look at
	//R2 = Length of data to look at in bytes
	u16 crc = get_reg(0);
	u32 data_addr = get_reg(1) & ~0x1;
	u32 length = get_reg(2) >> 1;

	//LUT for CRC
	u16 table[] = { 0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401, 0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400 };
	u16 crc_val;

	//Cycle through all the data to get the CRC16
	for(u32 x = 0; x < length; x++)
	{
		crc_val = mem->read_u16(data_addr);

		for(u32 y = 0; y < 4; y++)
		{
			u16 lut_val = table[crc & 0xF];
			crc >>= 4;
			crc ^= lut_val;

			u16 temp = crc_val >> (4 * y);
			crc ^= table[temp & 0xF];
		}

		data_addr += 2;
	}

	set_reg(0, crc);
}

/****** HLE implementation of WaitByLoop - NDS7 ******/
void NTR_ARM7::swi_waitbyloop()
{
	//Setup the initial value for swi_waitbyloop_count - R0
	swi_waitbyloop_count = get_reg(0) & 0x7FFFFFFF;

	//Set CPU idle state to 2
	idle_state = 2;
}

/****** HLE implementation of IntrWait - NDS7 ******/
void NTR_ARM7::swi_intrwait()
{
	//When R0 == 0, SWI will exit if any flags checked in R1 are already set
	//When R0 == 1, SWI will discard current IF flags and wait for the specified flags in R1

	if((reg.r0 == 0) && (reg.r1 & mem->nds7_if)) { return; } 

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Create temporary IF for flags in R1
	mem->nds7_temp_if = reg.r1;
	mem->nds7_if &= ~mem->nds7_temp_if;

	//Set CPU idle state to 3
	idle_state = 3;
	last_idle_state = 3;
}

/****** HLE implementation of VBlankIntrWait - NDS7 ******/
void NTR_ARM7::swi_vblankintrwait()
{
	//This is basically the IntrWait SWI, but R0 and R1 are both set to 1
	reg.r0 = 1;
	reg.r1 = 1;

	//Force IME on, Force IRQ bit in CPSR
	mem->write_u32(NDS_IME, 0x1);
	reg.cpsr &= ~CPSR_IRQ;

	//Create temporary IF for flags in R1
	mem->nds7_temp_if = reg.r1;
	mem->nds7_if &= ~mem->nds7_temp_if;

	//Set CPU idle state to 3
	idle_state = 3;
	last_idle_state = 3;
}

/****** HLE implementation of Halt - NDS7 ******/
void NTR_ARM7::swi_halt()
{
	//Set CPU idle state to 1
	idle_state = 1;
	last_idle_state = 1;
	mem->nds7_temp_if = mem->nds7_if;
	mem->nds7_if = 0;
}

/****** HLE implementation of SoundBias - NDS7 ******/
void NTR_ARM7::swi_soundbias()
{
	//TODO - Emulate delay
	u16 sound_bias = mem->read_u16(NDS_SOUNDBIAS);
	sound_bias &= ~0x3FF;
	
	if(reg.r0) { sound_bias |= 0x200; }
	mem->write_u16(NDS_SOUNDBIAS, sound_bias);
}

/****** HLE implementation of Div - NDS7 ******/
void NTR_ARM7::swi_div()
{
	//Grab the numerator - R0
	s32 num = get_reg(0);
	
	//Grab the denominator - R1
	s32 den = get_reg(1);

	s32 result = 0;
	s32 modulo = 0;

	//Do NOT divide by 0
	if(den == 0)
	{
		std::cout<<"ARM7::SWI::Warning - Div tried to divide by zero (ignoring operation) \n";
		return;
	}

	//Special case of -MAX / -1. Same results as NDS math coprocessor
	//Result simply equals -MAX, so here denominator is just set to 1
	if((num == 0x80000000) && (den == 0xFFFFFFFF))
	{
		std::cout<<"SWI::Warning - DivARM used -MAX/-1 \n";
		den = 1;
	}

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

/****** HLE implementation of CPUSet - NDS7 ******/
void NTR_ARM7::swi_cpuset()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Abort read/writes to the BIOS
	if(src_addr <= 0x3FFF) { return; }
	if(dest_addr <= 0x3FFF) { return; }

	//Grab transfer control options - R2
	u32 transfer_control = get_reg(2);

	//Transfer size - Bits 0-20 of R2
	u32 transfer_size = (transfer_control & 0x1FFFFF);

	//Determine if the transfer operation is copy or fill - Bit 24 of R2
	u8 copy_fill = (transfer_control & 0x1000000) ? 1 : 0;

	//Determine if the transfer operation is 16 or 32-bit - Bit 26 of R2
	u8 transfer_type = (transfer_control & 0x4000000) ? 1 : 0;

	src_addr &= (transfer_type == 0) ? ~0x1 : ~0x3;
	dest_addr &= (transfer_type == 0) ? ~0x1 : ~0x3; 

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

	//Write-back R0, R1
	set_reg(0, src_addr);
	set_reg(1, dest_addr);
}

/****** HLE implementation of CPUFastSet - NDS7 ******/
void NTR_ARM7::swi_cpufastset()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	src_addr &= ~0x3;
	dest_addr &= ~0x3;

	//Abort read/writes to the BIOS
	if(src_addr <= 0x3FFF) { return; }
	if(dest_addr <= 0x3FFF) { return; }

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

	//Write-back R0, R1
	set_reg(0, src_addr);
	set_reg(1, dest_addr);
}

/****** HLE implementation of Sqrt - NDS7 ******/
void NTR_ARM7::swi_sqrt()
{
	//Grab input
	u32 input = get_reg(0);

	//Set result of operation
	u16 result = sqrt(input);
	set_reg(0, result);
}

/****** HLE implementation of IsDebugger - NDS7 ******/
void NTR_ARM7::swi_isdebugger()
{
	//Always act as if a retail NDS, set RO to zero
	set_reg(0, 0);

	//Destroy value at 0x27FFFFA (halfword)
	mem->write_u16(0x27FFFFA, 0x0);
}


/****** HLE implementation of BitUnPack - NDS7 ******/
void NTR_ARM7::swi_bitunpack()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1;
	u32 dest_addr = get_reg(1);

	//Grab pointer to unpack info - R2;
	u32 unpack_info_addr = get_reg(2);

	//Grab the length
	u16 length = mem->read_u16(unpack_info_addr);
	unpack_info_addr += 2;

	//Grab the source width
	u8 src_width = mem->read_u8(unpack_info_addr);
	unpack_info_addr++;

	//Grab the destination width
	u8 dest_width = mem->read_u8(unpack_info_addr);
	unpack_info_addr++;

	if(src_width > dest_width)
	{
		std::cout<<"ARM7::SWI::ERROR - BitUnPack source width is greater than destination width\n";
		return;
	}

	u8 bit_mask = 0;

	switch(src_width)
	{
		case 1: bit_mask = 0x1; break;
		case 2: bit_mask = 0x3; break;
		case 4: bit_mask = 0xF; break;
		case 8: bit_mask = 0xFF; break;
		default: std::cout<<"ARM7::SWI::ERROR - Invalid source width\n"; return;
	}

	//Grab the data offset and zero flag
	u32 data_offset = mem->read_u32(unpack_info_addr);
	u8 zero_flag = (data_offset & 0x80000000) ? 1 : 0;
	data_offset &= ~0x80000000;

	u8 src_byte = 0;
	u8 src_count = 0;
	u32 result = 0;

	//Decompress bytes from source addr
	while(length > 0)
	{
		result = 0;

		//Cycle through the byte and expand to destination width
		for(u8 x = 0; x < 32; x += dest_width)
		{
			//Grab new source byte
			if((src_count % 8) == 0) 
			{
				src_byte = mem->read_u8(src_addr++);
				length--;
			}

			//Grab the slice
			u32 slice = (src_byte & bit_mask);
			src_byte >>= src_width;
			src_count += src_width;

			if(slice != 0) { slice += data_offset; }
			else if ((slice == 0) && (zero_flag == 1)) { slice += data_offset; }

			//OR the slice to the final result
			result |= (slice << x);
		}

		//Write result to the destination address
		mem->write_u32(dest_addr, result);
		dest_addr += 4;
	}
}	

/****** HLE implementation of LZ77UnCompReadByCallback - NDS7 ******/
void NTR_ARM7::swi_lz77uncompvram()
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

/****** HLE implementation of RLUnCompVram - NDS7 ******/
void NTR_ARM7::swi_rluncompvram()
{
	//Grab source address - R0
	u32 src_addr = get_reg(0);

	//Grab destination address - R1
	u32 dest_addr = get_reg(1);

	//Grab data header
	u32 data_header = mem->read_u32(src_addr);

	u32 data_size = (data_header >> 8);

	//Data pointer to compressed data. Points to first flag.
	u32 data_ptr = (src_addr + 4);

	//Uncompress data
	while(data_size > 0)
	{
		u8 flag = mem->read_u8(data_ptr++);

		u8 data_length = (flag & 0x7F);

		//Adjust data length, +1 for uncompressed data, +3 for compressed data
		if(flag & 0x80) { data_length += 3; }
		else { data_length += 1; }

		//Output the specified byte the amount of times in data_length
		for(int x = 0; x < data_length; x++)
		{
			u8 data_byte = 0;

			//Compressed
			if(flag & 0x80) { data_byte = mem->read_u8(data_ptr); }

			//Uncompressed
			else { data_byte = mem->read_u8(data_ptr++); }
				
			mem->write_u8(dest_addr++, data_byte);
			data_size--;

			if(data_size == 0) { return; }
		}

		//Manually adjust data pointer for compressed data to point to next flag
		if(flag & 0x80) { data_ptr++; }
	}
}	

/****** HLE implementation of GetSineTable - NDS7 ******/
void NTR_ARM7::swi_getsinetable()
{
	float index = reg.r0;
	float ratio = reg.r0 / 63.0;
	double pi = 3.14159265;

	if((index < 0) || (index > 0x3F))
	{
		std::cout<<"ARM7::SWI::Warning - Invalid GetSineTable index results in garbage data\n";
	}

	ratio *= 88.6;
	reg.r0 = sin((ratio * pi) / 180.0) * 0x8000;
}

/****** HLE implementation of GetPitchTable - NDS7 ******/
void NTR_ARM7::swi_getpitchtable()
{
	u32 index = reg.r0;

	if(index > 768)
	{
		std::cout<<"ARM7::SWI::Warning - Invalid GetPitchTable index results in garbage data\n";
	}

	reg.r0 = pitch_lut[index];
}

/****** HLE implementation of GetVolumeTable - NDS7 ******/
void NTR_ARM7::swi_getvolumetable()
{
	u32 index = reg.r0;

	if(index > 723)
	{
		std::cout<<"ARM7::SWI::Warning - Invalid GetVolumeTable index results in garbage data\n";
	}

	reg.r0 = vol_lut[index];
}
	

/****** HLE implementation of CustomHalt - NDS7 ******/
void NTR_ARM7::swi_customhalt()
{
	u8 param = (reg.r2 & 0xFF);
	mem->write_u8(NDS_HALTCNT, param);
}
