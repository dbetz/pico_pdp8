/* nano8.cpp A minimalist PDP8/E simulator

Copyright (c) 2010, Ian Schofield
With additions by David Betz (2022)

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <stdio.h>
#include "pico/stdlib.h"

#include "nano8.h"

short pc;
short acc;
short inst;
short mem[MEMSIZE];

static short ma, mq, intf, ibus;
static short ifl, dfl, ifr, dfr, svr, uflag, usint, intinh;
static unsigned int tmp;
static int kcnt = 0;

void caf()
{
	acc = mq = ibus = 0;
	dfr = ifr = dfl = ifl = uflag = 0;
	tti_clear();
	tto_clear();
	pti_clear();
	pto_clear();
	dsk_clear();
}

void iot()
{

	if (uflag == 3)
	{
		usint = 1;
		return;
	}
	switch (inst & 0770)
	{
	case 0:
		switch (inst & 0777)
		{
		case 000:
			if (intf)
				pc++;
			intf = intinh = 0;
			break;
		case 001:
			intinh |= 1;
			break;
		case 002:
			intf = intinh = 0;
			break;
		case 003:
			if (ibus)
				pc++;
			break;
		case 004:
			acc = (acc & 010000) ? 014000 : 0;
			if (intinh & 1)
				acc |= 0200;
			if (intf)
				acc |= 01000;
			acc |= svr;
			break;
		case 005:
			intinh = 3;
			acc &= 07777;
			if (acc & 04000)
				acc |= 010000;
			svr = acc & 0177;
			dfr = (svr & 07) << 3;
			dfl = dfr << 9;
			ifr = (svr & 070);
			if (svr & 0100)
				uflag = 1;
			break;
		case 007:
			caf();
			break;
		}
		break;
	case 0200:
	case 0210:
	case 0220:
	case 0230:
	case 0240:
	case 0250:
	case 0260:
	case 0270:
		switch (inst & 0777)
		{
		case 0204:
			usint = 0;
			break;
		case 0254:
			if (usint)
				pc++;
			break;
		case 0264:
			uflag = 0;
			break;
		case 0274:
			uflag = 1;
			break;
		case 0214:
			acc |= dfr;
			break;
		case 0224:
			acc |= ifr;
			break;
		case 0234:
			acc |= svr;
			break;
		case 0244:
			dfr = (svr & 07) << 3;
			dfl = dfr << 9;
			ifr = (svr & 070);
			if (svr & 0100)
				uflag = 1;
			break;
		}
		switch (inst & 0707)
		{
		case 0201:
			dfr = inst & 070;
			dfl = dfr << 9;
			break;
		case 0202:
			ifr = inst & 070;
			intinh |= 2;
			break;
		case 0203:
			dfr = inst & 070;
			ifr = inst & 070;
			dfl = dfr << 9;
			intinh |= 2;
			break;
		}
		break;

	case 010:
	    pti_iot();
		break;
	case 020:
	    pto_iot();
		break;
	case 030:
	    tti_iot();
		break;
	case 040:
	    tto_iot();
		break;
	case 0600:
	case 0610:
	case 0620:
	    dsk_iot();
		break;
	case 0740:
	    rk8e_iot();
	}
}

void group3()
{
	int tm;

	if (inst == 07521)
	{
		tm = acc;
		acc = mq | (acc & 010000);
		mq = tm & 07777;
		return;
	}
	if (inst & 0200)
		acc = acc & 010000;
	if (inst & 020)
	{
		mq = acc & 07777;
		acc &= 010000;
	}
	if (inst & 0100)
		acc |= mq;
}

void group2()
{
	int state;

	state = 0;
	if (acc & 04000)
		state |= 0100;
	if ((acc & 07777) == 0)
		state |= 040;
	if (acc & 010000)
		state |= 020;
	if ((inst & 0160) == 0)
		state = 0;
	if (inst & 010)
	{
		if ((~state & inst) == inst)
			pc++;
	}
	else if (state & inst)
		pc++;
	if (inst & 0200)
		acc &= 010000;
	if (inst & 4)
		acc |= 07777; //OSR
}

void group1()
{

	if (inst & 0200)
		acc &= 010000;
	if (inst & 0100)
		acc &= 07777;
	if (inst & 040)
		acc ^= 07777;
	if (inst & 020)
		acc ^= 010000;
	if (inst & 1)
		acc++;
	acc &= 017777;
	switch (inst & 016)
	{
	case 2:  tmp = (acc << 6) | ((acc >> 6) & 077); // BSW .. v untidy!
		tmp &= 07777;
		if (acc & 010000) tmp |= 010000;
		acc = tmp;
		break;
	case 06:
		acc = acc << 1;
		if (acc & 020000)
			acc++;
	case 04:
		acc = acc << 1;
		if (acc & 020000)
			acc++;
		break;
	case 012:
		if (acc & 1)
			acc |= 020000;
		acc = acc >> 1;
	case 010:
		if (acc & 1)
			acc |= 020000;
		acc = acc >> 1;
		break;
	}
	acc &= 017777;
}

void start(short addr)
{
	caf();
	pc = addr;
}

bool cycl(void)
{
	if (intf && ibus)
	{
		mem[0] = pc & 07777;
		pc = 1;
		intf = intinh = 0;
		svr = (ifl >> 9) + (dfl >> 12);
		if (uflag == 3)
			svr |= 0100;
		dfr = ifr = dfl = ifl = uflag = 0;
	}

	ibus = tti_interrupt() || tto_interrupt();
	inst = mem[pc + ifl];
	ma = ((inst & 0177) | ((inst & 0200) ? (pc & 07600) : 0)) + ifl;
	if (inst & 0400 && inst < 06000)
	{
		if ((ma & 07770) == 010)
			mem[ma]++;
		mem[ma] &= 07777;
		if (inst & 04000)
			ma = mem[ma] + ifl;
		else
			ma = mem[ma] + dfl;
	}
	//printf("PC:%04o Inst:%04o MA:%04o Mem:%04o Acc:%05o\r\n", pc, inst, ma, mem[ma], acc);
	pc++;
	if (kcnt++ > 100)
	{
	    if (!tti_idle())
	        return false;
	    pti_idle();
		kcnt = 0;
	}
	tto_idle();
	pto_idle();
	switch (inst & 07000)
	{
	case 0:												//AND
		acc &= mem[ma] | 010000;
		break;
	case 01000:											//TAD
		acc += mem[ma] & 07777;
		break;
	case 02000:                                         //ISZ
		if (!(mem[ma] = (mem[ma] + 1) & 07777))
			pc++;
		break;
	case 03000:											//DCA
		mem[ma] = acc & 07777;
		acc &= 010000;
		break;
	case 04000:											//JMS
		ifl = ifr << 9;
		ma = (ma & 07777) + ifl;
		mem[ma] = pc;
		pc = (ma + 1) & 07777;
		intinh &= 1;
		uflag |= 2;
		break;
	case 05000:											//JMP
		ifl = ifr << 9;
		pc = (ma & 07777);
		intinh &= 1;
		uflag |= 2;
		break;
	case 06000:											//IOT
		iot();
		break;
	case 07000:											//OPR
		if (inst & 0400)
		{
			if (inst & 1)
			{
				group3();
				break;
			}
			if (inst & 2)
				return false;
			group2();
		}
		else
			group1();
		break;
	}
	acc &= 017777;
	if (intinh == 1 && inst != 06001)
		intf = 1;
	return true;
}
