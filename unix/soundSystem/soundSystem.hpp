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

#ifndef _SOUNDSYSTEM_HPP_
#define _SOUNDSYSTEM_HPP_
#include <alsa/asoundlib.h>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>

#include "exceptions.hpp"
#include "echoData.hpp"
#include "channel.hpp"

struct SAPU;

class SoundSystem
{
public:
	SoundSystem(unsigned int mode = 7,
	            std::string device = "default") throw (SnesException);
	~SoundSystem(void);


public:
	void processSound(void);
	void playSample(unsigned channel, struct SAPU *apu)
		{channels[channel].playSample(apu, playbackRate);}
	void reset(bool full);

	void setMute(bool m) {mute = m;}
	void setVolume(int left, int right);
	void setEchoVolume(int left, int right) {echoData->setVolume(left, right);}
	void setChannelVolume(unsigned c, int left, int right)
		{channels[c].setVolume(left, right);}

	void setEchoEnable(boost::uint8_t byte);
	void setEchoWriteEnable(bool byte);
	void setEchoFeedback(int f) {echoData->setFeedBack(f);}
	void setEchoDelay(int delay) {echoData->setDelay(delay, playbackRate);}
	void setFilterCoefficient (int tap, int value) {echoData->setFilterCoefficient (tap, value);}

	void setChannelType(unsigned c, SoundType t) {channels[c].setType(t);}
	SoundType getChannelType(unsigned c) const {return channels[c].getType();}
	SoundState getChannelState(unsigned c) const {return channels[c].getState();}
	signed short getChannelSample(unsigned c) const {return channels[c].getSample();}
	void setSoundKeyOff(unsigned c) {channels[c].setSoundKeyOff(playbackRate);}
	void fixEnvelope(unsigned channel,
	                 boost::uint8_t gain, boost::uint8_t adsr1, boost::uint8_t adsr2)
		{channels[channel].fixEnvelope(gain, adsr1, adsr2, playbackRate);}
	int getEnvelopeHeight(unsigned c) const {return channels[c].getEnvelopeHeight();}
	void setSoundFrequency(unsigned c, int hertz)
		{channels[c].setSoundFrequency(hertz, playbackRate);}
	void setAllChannelsNoiseFreq(int hertz);
	void setChannelFrequency(unsigned c, int hertz) {channels[c].setFrequency(hertz);}

	void setFrequencyModulationEnable(boost::uint8_t byte)
		{pitchMod = byte & ~1;}

private:
	void mixSamples(void);
	void mixStereo();

private:
	snd_pcm_t *playbackHandle;
	boost::int16_t *audioBuffer;
	boost::int16_t *mixBuffer;
	unsigned bufferSize;

	boost::thread *threadProcess;
	bool terminated;

	boost::shared_ptr<EchoData> echoData;

	bool mute;

#warning on dirait que les volumes left et right ne servent à rien
    short masterVolumeLeft;
    short masterVolumeRight;
    int masterVolume [2];

	int pitchMod;

	std::vector<Channel> channels;

	int soundSwitch;
	int noiseGen;
	int noiseHertz;
	unsigned playbackRate;

	std::vector<boost::uint32_t> dummy;

	boost::uint32_t errRate;
};

#endif
