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


#ifndef _ECHODATA_HPP_
#define _ECHODATA_HPP_

#include <boost/cstdint.hpp>

#include "exceptions.hpp"

#define ECHO_LOOP_SIZE 16

class EchoData
{
public:
	EchoData(unsigned bufferSize) throw (SnesException);
	~EchoData();

public:
	bool isEchoEnabled() const {return enable && echoBufferSize > 0;}
	void setEnable(boost::uint8_t byte);

	void mixSamples(boost::int16_t *audioBuffer,
	                boost::int16_t *mixBuffer,
	                unsigned audioBufferSize,
	                int masterVolume[2]);
	void reset();

	boost::int16_t* getBuffer() {return buffer;}
	boost::int16_t* getDummyBuffer() {return dummyBuffer;}


private:
	void resetBuffer() {std::fill(buffer, buffer + bufferSize, 0);}

private:
	boost::int16_t *echo;
	boost::int16_t *buffer;
	boost::int16_t *dummyBuffer;
	int loop [ECHO_LOOP_SIZE];
    short volumeLeft;
    short volumeRight;
    bool enable;
    int feedback;
    unsigned ptr;
    unsigned bufferSize;
    unsigned echoBufferSize;
    bool writeEnabled;
    int channelEnable;
	bool noFilter;
	int volume[2];
	int filterTaps [8];
	unsigned long Z;
};

#endif
