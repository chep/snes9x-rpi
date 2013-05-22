// snes9x-rpi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth
// Floor, Boston, MA 02110-1301, USA.
//
//
// snes9x-rpi is based on snes9x:
// Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
//
// (c) Copyright 1996 - 2001 Gary Henderson (gary.henderson@ntlworld.com) and
//                           Jerremy Koot (jkoot@snes9x.com)
//
// Super FX C emulator code 
// (c) Copyright 1997 - 1999 Ivar (ivar@snes9x.com) and
//                           Gary Henderson.
// Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
//
// DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
// C4 asm and some C emulation code (c) Copyright 2000 zsKnight and _Demo_.
// C4 C code (c) Copyright 2001 Gary Henderson (gary.henderson@ntlworld.com).
//
// DOS port code contains the works of other authors. See headers in
// individual files.
//
// Snes9x homepage: http://www.snes9x.com
//
// Copyright Cédric Chépied 2013

#ifndef _APU_HPP_
#define _APU_HPP_

#include <vector>

#include "spc700.h"
#include "port.h"


typedef union
{
#ifdef LSB_FIRST
    struct { uint8 A, Y; } B;
#else
    struct { uint8 Y, A; } B;
#endif
    uint16 W;
} YAndA;



class IAPU
{
public:
	IAPU(uint8 RAMInitialValue); /**<Constructor. Allocate memory. */
	virtual ~IAPU(); /**< Destructor.*/

public:
	/** Give or set RAM at a specific index.
	    @param[in] i index in RAM
	    @return a reference to the element of the RAM at index.
	*/
	uint8& operator[](unsigned i) {return RAM[i];}

public:
	void reset(); /**<Reset all attributes. */
	void setCarry(bool c) {carry = c;}
	void setZero(bool z) {zero = z;}
	void setOverflow(bool o) {overflow = o;}
	void setAPUExecuting(bool e) {APUExecuting = e;}
	int32 getOneCycle() const {return oneCycle;}
	void resetROM();
	void fillRAM(unsigned offset, const std::vector<uint8>& src);

private:
    std::vector<uint8> RAM;
	std::vector<uint8>::iterator PC;
    std::vector<uint8>::iterator directPage;
    bool        APUExecuting;
    uint8_32	bit;
    uint32		address;
    uint8		*waitAddress1;
    uint8		*waitAddress2;
    uint32		waitCounter;
    std::vector<uint8> shadowRAM;
    std::vector<uint8> cachedSamples;
    bool  	    carry;
    bool        zero;
    bool        overflow;
    uint32		timerErrorCounter;
    uint32		scanline;
    int32		oneCycle;
    int32		twoCycles;

    uint8 RAMInitialValue;
};


class APU
{
public:
	APU();
	~APU();

public:
	void reset(); /**<Reset all attributes. */
	void setCycles(int32 iapuOneCycle);
	void setDSP (uint8 byte, IAPU& iapu);
	void setControl (uint8 byte, IAPU& iapu);
	void setTimer(uint16 address, uint8 byte, IAPU& iapu);

/* get and set */
	uint8_32 getP() const {return registers.P;}
	uint8 getDSP(unsigned index) const {return DSP[index];}

private:
	struct Registers
	{
		uint16_32	PC;
		uint8_32	P;
		YAndA		YA;
		uint8_32	X;
		uint8_32	S;
	};

private:
    int32		cycles;
    bool        showROM;
    uint8_32	flags;
    uint8		keyedChannels;
    std::vector<uint8> outPorts;
    std::vector<uint8> DSP;
    std::vector<uint8> extraRAM;
    std::vector<uint16_32> timer;
    std::vector<uint16_32> timerTarget;
    std::vector<bool> timerEnabled;
    std::vector<bool> timerValueWritten;

    Registers registers;
	std::vector<int32> vCycles;
};


class APUController
{
public:
	APUController(uint8 RAMInitialValue);

public:
	void reset(); /**<Reset all attributes. */
	void setAPUControl (uint8 byte);
	void setAPUTimer (uint16 address, uint8 byte);
	uint8 getAPUDSP ();
	bool getNextAPUEnabled() const {return nextAPUEnabled;}

private:
	void unpackStatus();

private:
	IAPU iapu;
	APU apu;

	bool APUEnabled;
	bool nextAPUEnabled;
};


/*
STATIC inline void S9xAPUUnpackStatus()
{
    IAPU._Zero = ((APURegisters.P & Zero) == 0) | (APURegisters.P & Negative);
    IAPU._Carry = (APURegisters.P & Carry);
    IAPU._Overflow = (APURegisters.P & Overflow) >> 6;
}

STATIC inline void S9xAPUPackStatus()
{
    APURegisters.P &= ~(Zero | Negative | Carry | Overflow);
    APURegisters.P |= IAPU._Carry | ((IAPU._Zero == 0) << 1) |
		      (IAPU._Zero & 0x80) | (IAPU._Overflow << 6);
}*/

#define S9xAPUPackStatus() \
{ \
    APURegisters.P &= ~(Zero | Negative | Carry | Overflow); \
    APURegisters.P |= IAPU._Carry | ((IAPU._Zero == 0) << 1) | \
		      (IAPU._Zero & 0x80) | (IAPU._Overflow << 6); \
}

#define S9xAPUUnpackStatus_OP() \
{ \
    iapu->_Zero = ((areg->P & Zero) == 0) | (areg->P & Negative); \
    iapu->_Carry = (areg->P & Carry); \
    iapu->_Overflow = (areg->P & Overflow) >> 6; \
}

#define S9xAPUPackStatus_OP() \
{ \
    areg->P &= ~(Zero | Negative | Carry | Overflow); \
    areg->P |= iapu->_Carry | ((iapu->_Zero == 0) << 1) | \
		      (iapu->_Zero & 0x80) | (iapu->_Overflow << 6); \
}

START_EXTERN_C
bool8_32 S9xInitSound (int quality, bool8_32 stereo, int buffer_size);
void S9xOpenCloseSoundTracingFile (bool8_32);
extern void (*S9xApuOpcodes [256]) (struct SAPURegisters *, struct SIAPU *, struct SAPU *);
END_EXTERN_C


#define APU_VOL_LEFT 0x00
#define APU_VOL_RIGHT 0x01
#define APU_P_LOW 0x02
#define APU_P_HIGH 0x03
#define APU_SRCN 0x04
#define APU_ADSR1 0x05
#define APU_ADSR2 0x06
#define APU_GAIN 0x07
#define APU_ENVX 0x08
#define APU_OUTX 0x09

#define APU_MVOL_LEFT 0x0c
#define APU_MVOL_RIGHT 0x1c
#define APU_EVOL_LEFT 0x2c
#define APU_EVOL_RIGHT 0x3c
#define APU_KON 0x4c
#define APU_KOFF 0x5c
#define APU_FLG 0x6c
#define APU_ENDX 0x7c

#define APU_EFB 0x0d
#define APU_PMON 0x2d
#define APU_NON 0x3d
#define APU_EON 0x4d
#define APU_DIR 0x5d
#define APU_ESA 0x6d
#define APU_EDL 0x7d

#define APU_C0 0x0f
#define APU_C1 0x1f
#define APU_C2 0x2f
#define APU_C3 0x3f
#define APU_C4 0x4f
#define APU_C5 0x5f
#define APU_C6 0x6f
#define APU_C7 0x7f

#define APU_SOFT_RESET 0x80
#define APU_MUTE 0x40
#define APU_ECHO_DISABLED 0x20

#define FREQUENCY_MASK 0x3fff
#endif
