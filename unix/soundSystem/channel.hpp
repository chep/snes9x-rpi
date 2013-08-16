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

#ifndef _CHANNEL_HPP_
#define _CHANNEL_HPP_

#include <boost/cstdint.hpp>

#include "soundConstants.hpp"

class Channel
{

public:
	SoundState getState() const {return state;}
	void decode(bool mod);
	void mixStereo(boost::int16_t *mixBuffer, unsigned bufferSize,
	               bool mod,
	               int pitchMod, int *noiseGen, int playbackRate);
	void setChannelNumber(unsigned number) {channelNumber = number;}
	void playSample(struct SAPU *apu, int playbackRate);

	void reset();
	void setEchoBufPtr(boost::int16_t *buf) {echoBufPtr = buf;}

private:
	void decodeBlock();
	void altDecodeBlock();
	void setEndOfSample ();
	void setEnvRate (unsigned long rate, int direction, int target,
	                 int playbackRate);
	void fixEnvelope (boost::uint8_t gain, boost::uint8_t adsr1, boost::uint8_t adsr2,
	                  int playbackRate);
	void setEnvelopeHeight (int level);
	void setSoundADSR (int attack_rate, int decay_rate,
	                   int sustain_rate, int sustain_level, int release_rate,
	                   int playbackRate);
	bool setSoundMode (SoundMode newMode);
	void setSoundFrequency (int hertz, int playbackRate);

private:
	unsigned channelNumber;

    SoundState state;
    SoundType type;
    short volumeLeft;
    short volumeRight;
    boost::uint32_t hertz;
    boost::uint32_t frequency;
    boost::uint32_t count;
    bool loop;
    int envx;
    short leftVolLevel;
    short rightVolLevel;
    short envxTarget;
    unsigned long int envError;
    unsigned long erate;
    int direction;
    unsigned long attackRate;
    unsigned long decayRate;
    unsigned long sustainRate;
    unsigned long releaseRate;
    unsigned long sustainLevel;
    signed short sample;
    signed short decoded [16];
    signed short previous16 [2];
    signed short *block;
    boost::uint16_t sampleNumber;
    bool lastBlock;
    bool needsDecode;
    boost::uint32_t blockPointer;
    boost::uint32_t samplePointer;
    boost::int16_t *echoBufPtr;
    SoundMode mode;
    boost::int32_t envxx;
    signed short nextSample;
    boost::int32_t interpolate;
    boost::int32_t previous [2];
    // Just incase they are needed in the future, for snapshot compatibility.
    boost::uint32_t dummy [8];
};

#endif
