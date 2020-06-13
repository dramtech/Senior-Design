/* Copyright (c) 2020, Texas Instruments Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

*  Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

*  Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

*  Neither the name of Texas Instruments Incorporated nor the names of
   its contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/


#include "grlib/grlib.h"

static const unsigned char pixel_leftCarIndicator1BPP_COMP_RLE4[] =
{
0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xb1, 0x70, 0x01, 0x10, 0x11, 0x50, 0x11, 0x90, 0x11, 0x60, 0x11, 0x80, 0x11, 0x60, 0x11, 
0x80, 0x11, 0x70, 0x11, 0x70, 0x21, 0x60, 0x21, 0x30, 0xf1, 0x21, 0x00, 0xf1, 0xf1, 0xf1, 0xf1, 0xf1, 0x01, 0x20, 0x81, 
0x10, 0x51, 0x20, 0x81, 0x10, 0x91, 0x50, 0x51, 0x20, 0x31, 0x70, 0x31, 0xf0, 
};

static const unsigned long palette_leftCarIndicator1BPP_COMP_RLE4[]=
{
	0x000000, 	0xffffff
};

const tImage  leftCarIndicator1BPP_COMP_RLE4=
{
	IMAGE_FMT_1BPP_COMP_RLE4,
	20,
	20,
	2,
	palette_leftCarIndicator1BPP_COMP_RLE4,
	pixel_leftCarIndicator1BPP_COMP_RLE4,
};

