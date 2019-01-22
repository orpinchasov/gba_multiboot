// XBOO adaptation to GBA
#include <gba_console.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xboo.h"
#include "transfer.h"

#include "mb_image_bin.h"

#define printf iprintf

unsigned char filesize[2],no_header=0,test_flag=0;			//filesize transfer(1st and 2nd byte), no header flag(to know whether to send dummy header),beta_test flag
unsigned char rec[4]={0xFF,0xFF,0xFF,0xFF};		            //Received Data from GBA (used by Sendata32 func.)
unsigned long filesize_32;									//file transfer function length
// NOTE: These values are adjusted to mulitboot from the original normal mode.
unsigned long enc,CRC,rr,hh,cc,var_30,var_C = 0xFFF8;		//Encryption and crc variables(all of them important)

//Dummy Header
unsigned char header[0xCF]={0x2E,0x0,0x0,0xEA,0x24,0xFF,0xAE,0x51,0x69,0x9A,0xA2,0x21,0x3D,0x84,0x82,0xA,0x84,0xE4,0x9,0xAD,0x11,0x24,0x8B,0x98,0xC0,0x81,0x7F,0x21,0xA3,0x52,0xBE,0x19,0x93,0x9,0xCE,0x20,0x10,0x46,0x4A,0x4A,0xF8,0x27,0x31,0xEC,0x58,0xC7,0xE8,0x33,0x82,0xE3,0xCE,0xBF,0x85,0xF4,0xDF,0x94,0xCE,0x4B,0x9,0xC1,0x94,0x56,0x8A,0xC0,0x13,0x72,0xA7,0xFC,0x9F,0x84,0x4D,0x73,0xA3,0xCA,0x9A,0x61,0x58,0x97,0xA3,0x27,0xFC,0x3,0x98,0x76,0x23,0x1D,0xC7,0x61,0x3,0x4,0xAE,0x56,0xBF,0x38,0x84,0x0,0x40,0xA7,0xE,0xFD,0xFF,0x52,0xFE,0x3,0x6F,0x95,0x30,0xF1,0x97,0xFB,0xC0,0x85,0x60,0xD6,0x80,0x25,0xA9,0x63,0xBE,0x3,0x1,0x4E,0x38,0xE2,0xF9,0xA2,0x34,0xFF,0xBB,0x3E,0x3,0x44,0x78,0x0,0x90,0xCB,0x88,0x11,0x3A,0x94,0x65,0xC0,0x7C,0x63,0x87,0xF0,0x3C,0xAF,0xD6,0x25,0xE4,0x8B,0x38,0xA,0xAC,0x72,0x21,0xD4,0xF8,0x7,0x5B,0x5A,0x65,0x72,0x6F,0x5D,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x96,0x0,0x80,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x79,0x0,0x0};

//Main program file
// NOTE: This seems to be like too big of an array to keep in the iwram.
// Instead, I'll use mg_image_bin directly
//unsigned char program[0x4000F];
#define program mb_image_bin

int xboo_main()
{
	//printf ("Header=%d, bit delay=%.3f, unit delay=%.3f, Port=%X, File=%s\n\n",no_header,bit_delay,unit_delay,port_address,arg[f]);
	FileCalc();
	Initialize();
	SendMainData();

    return 0;
}

volatile int delay = 0;

void DelayLoop(float a, int b)
{
    delay = 0;
    for (int i = 0; i < 10000; ++i)
        delay++;
}

void FileCalc()
{
	unsigned int filesize_long;
	//char fchar[0x4000F];

	filesize_long=mb_image_bin_size;			        //Get file length (from the generated.h file)
    // TODO: I can't hold all of this in iwram. Instead I'll work
    // directly on the whole image.
    //memcpy(fchar, mb_image_bin, mb_image_bin_size);
	filesize_32=(filesize_long + 0x0F) & 0xFFFFFFF0;	//File size realignment to xxxxx0h

	if (filesize_32<0x01C0)								//Adjust if too small
		filesize_32 = 0x01C0;

	if (filesize_32>0x40000)							//Quit if too big
	{
		printf("File to be transfered is too big....\n\n");
		exit(1);
	}

/*
	for (loop=0; loop<filesize_32; loop++)
	{
		if (loop<filesize_long)
			program[loop]=fchar[loop];
		else
			program[loop]=0;
	}
    */

	filesize_long=((filesize_32-0xC0)>>2)-0x34;			//transfer length in 32 bits encoded (for length transfer protocol)
	filesize_32=(filesize_32-0xC0)/4;					//transfer length in 32 bits, after realignment (see SendMainData Function)
	filesize[0]=(filesize_long>>8)&(0x000000FF);		//put upper byte into universal variable filesize[0]
	filesize[1]=(filesize_long)&(0x000000FF);			//put lower byte into universal variable filesize[1]
}

void SendData32(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
    transfer(c << 8 | d);
}

void Initialize()
{
	int loop,timeout=0;

	printf("Requesting Normal Mode...");

START:
	for (loop=0; !(((rec[0]==0x00)&&(rec[1]==0x00))|(rec[0]==0x72)); loop++)		//Wait for slave to become ready
	{
		if (loop>7)
		{
			DelayLoop(10,1);	//If failed wait 65ms

			loop=0;
			timeout++;
		}

		if (timeout>240)
		{
			printf("Timeout : Failed\n\nCheck if your GBA is on. Hold START + SELECT during the boot up process.\nElse, there is a problem with your cable.\n\n");
			exit(2);
		}

		SendData32(0,0,0x62,0x02);
	}

	for (loop=0; loop<47; loop++ )						//Send repeating 6200
	{
		SendData32(0,0,0x62,0x02);
		if (rec[0]!=0x72)								//Try again if failed, wait 62.5ms
		{
			DelayLoop(50,1);
			DelayLoop(12.5,1);

			goto START;
		}
	}

	SendData32(0,0,0x61,0x02);							//about to send header

	printf ("Done\nSending Header Data......");
	for (loop=0; loop<0x60; loop++)						//Send header 0xC0 bytes, 0x60 times for 2bytes
	{
		if (no_header==0)
			SendData32(0,0,program[(loop*2)+1],program[(loop*2)]);

		else if (no_header==1)
		{
			if (loop<2)
				SendData32(0,0,program[(loop*2)+1],program[(loop*2)]);
			else
				SendData32(0,0,header[(loop*2)+1],header[(loop*2)]);
		}
	}
	printf ("Done\n");

	SendData32(0,0,0x62,0x02);
	SendData32(0,0,0x62,0x02);							//xchange master slave info

	while (!(rec[0]==0x73))								//Wait until slave reply 73cc
		SendData32(0,0,0x63,0xF7);

	cc=rec[1];											//get the encryption
	hh = ((cc&0x0FF)+0x20F)&0x0FF;						//
	enc = ((cc&0x0FF)<<8)|(0x0FFFF00F7);				//

	SendData32(0,0,0x64,hh);							//Handshake

	DelayLoop(50,1);									//Pause 1/16 sec
}

void SendMainData()
{
	unsigned long data32, pos=0xC0, loop, timeout=0;
	int bit, percent=0;

	SendData32(0,0,filesize[0],filesize[1]);										//transfer length
	rr = rec[1];
	rr = rr&0x0FF;

	printf ("Sending Main Data........  0%%");
	for (loop=0; loop<filesize_32; loop++)											//Send Program
	{
		data32 = program[pos] + (program[pos+1] << 8)
				+ (program[pos+2]<<0x10) + (program[pos+3] <<0x18);

		CRC = data32;																//CRC calculation
		for (bit = 0; bit<=31; bit++)
		{
			var_30 = var_C ^ CRC;
			var_C = var_C >> 1;
			CRC = CRC >> 1;
			if (var_30&0x01)
                // Adapted to multiplayer transfer
				var_C = var_C^0x0A517;
		}

		enc = (enc * 0x6F646573)+1;																		//Data encoding
        // NOTE: The following has been adapted to multiboot instead of
        // normal mode.
		data32 = (enc ^ data32)^( ((~(pos+0x2000000))+1) ^ 0x6465646F);

		//SendData32(((data32>>0x18)&0x0FF),((data32>>0x10)&0x0FF),((data32>>8)&0x0FF),(data32&0x0FF));	//Send data 32bit at once
        // NOTE: We need to send in two times:
        SendData32(0, 0, ((data32>>0x18)&0x0FF),((data32>>0x10)&0x0FF));
        SendData32(0, 0, ((data32>>8)&0x0FF), (data32&0x0FF));

		pos = pos + 4;																					//realingn pos pointer, only for encoding calculation (see above)

		if (percent!=(((loop+1)*100)/filesize_32))
		{
			percent=((loop+1)*100)/filesize_32;
			//printf ("\b\b\b\b%3d%%",percent);
		}
	}
	printf("\b\b\b\bDone\nCRC Check................");

	SendData32(0,0,0x00,0x65);
	while ((!((rec[0]==0x00)&&(rec[1]==0x75)))&&(timeout<1000))
	{
		SendData32(0,0,0x00,0x65);												//while no response keep sending
		timeout++;
		if (timeout==1000)
			printf ("Timeout : ");
	}

	SendData32(0,0,0x0,0x66);													//transfer CRC follows

	CRC = ((rr<<8)|0x0FFFF0000)+hh;												//additional CRC calculation
	for (bit = 0 ; bit<=31 ; bit++)
	{
		var_30 = var_C ^ CRC;
		var_C  = var_C>>1;
		CRC = CRC>>1;
		if (var_30 & 0x01)
			var_C = var_C ^ 0x0A517;
	}

	SendData32(0,0,(((var_C>>8)&0x0FF)+test_flag),var_C&0x0FF);				//CRC transfer,modified with test flag

	CRC=rec[0];
	CRC=(CRC<<8)+rec[1];

	if (CRC==var_C)
		printf ("Passed\n\n");

	if (CRC!=var_C)
	{
		printf ("CRC check has failed.\n");
		exit(3);
	}
}
