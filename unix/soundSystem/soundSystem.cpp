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

#include <iostream>
#include <fstream>

#include "soundSystem.hpp"
#include "soundConstants.hpp"
#include "apu.h"
#include "snes9x.h"

/********************************* Constants **********************************/
static const unsigned int audioFrequencies[8] =
{0, 8192, 11025, 16000, 22050, 29300, 36600, 44100};

static const unsigned int audioBufferSizes [8] =
{ 0, 256, 256, 256, 512, 512, 1024, 1024};

#define NUM_CHANNELS 8

/********************************** Globals ***********************************/
extern "C" struct SAPU APU;

/************************************ Code ************************************/
SoundSystem::SoundSystem(unsigned int mode, std::string device) throw (SnesException):
	playbackHandle(NULL),
	audioBuffer(NULL),
	threadProcess(NULL),
	terminated(false),
	channels(NUM_CHANNELS),
	filterTaps(8),
	dummy(3)
{
	int err;
	snd_pcm_hw_params_t *hw_params;
	unsigned int frequency;
	const int periods = 2;

	for(unsigned int i = 0; i < NUM_CHANNELS; ++i)
		channels[i].setChannelNumber(i);

	if (mode >= sizeof(audioFrequencies) / sizeof(audioFrequencies[0]))
		mode = (sizeof(audioFrequencies) / sizeof(audioFrequencies[0]))- 1;
	frequency = audioFrequencies[mode];
	bufferSize = audioBufferSizes[mode];

	err = snd_pcm_open(&playbackHandle,
	                   device.c_str(),
	                   SND_PCM_STREAM_PLAYBACK,
	                   0);

	if(err < 0)
		throw SnesException(std::string("Cannot open audio device: ") + snd_strerror(err));

	/* Initialize parameters */
	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0)
		throw SnesException(std::string("Cannot allocate hardware parameter structure: ") + snd_strerror(err));

	if ((err = snd_pcm_hw_params_any(playbackHandle, hw_params)) < 0)
		throw AlsaFreeParamsException(hw_params,
		                              std::string("Cannot initialize hardware parameter structure: ")
		                              + snd_strerror(err));

	/* Access type */
	err = snd_pcm_hw_params_set_access(playbackHandle,
	                                   hw_params,
	                                   SND_PCM_ACCESS_RW_INTERLEAVED);
	if (err < 0)
		throw AlsaFreeParamsException(hw_params,
		                              std::string("Cannot set access type: ")
		                              + snd_strerror(err));

	/* Sample format */
	err = snd_pcm_hw_params_set_format(playbackHandle,
	                                   hw_params,
	                                   SND_PCM_FORMAT_S16_LE);
	if (err < 0)
		throw AlsaFreeParamsException(hw_params,
		                              std::string("Cannot set sample format: ")
		                              + snd_strerror(err));

	/* Rate */
	err = snd_pcm_hw_params_set_rate_near(playbackHandle, hw_params, &frequency, 0);
	if (err < 0)
		throw AlsaFreeParamsException(hw_params,
		                              std::string("Cannot set sample rate: ")
		                              + snd_strerror (err));

	/* Channel count */
	err = snd_pcm_hw_params_set_channels(playbackHandle,
	                                     hw_params,
	                                     2);
	if (err < 0)
		throw AlsaFreeParamsException(hw_params,
		                              std::string("Cannot set channel count: ")
		                              + snd_strerror(err));

	/* Set number of periods. */
	if (snd_pcm_hw_params_set_periods(playbackHandle, hw_params, periods, 0) < 0)
	    throw AlsaFreeParamsException(hw_params,
	                                  std::string("Cannot set periods: ")
	                                  + snd_strerror(err));

	/* Set buffer size */
	snd_pcm_uframes_t frameSize = (bufferSize * periods);
	err = snd_pcm_hw_params_set_buffer_size_near(playbackHandle,
	                                             hw_params,
	                                             &frameSize);
	if (err < 0)
	    throw AlsaFreeParamsException(hw_params,
	                                  std::string("Cannot set buffer size: ")
	                                  + snd_strerror(err));

	/* Apply parameters */
	if ((err = snd_pcm_hw_params(playbackHandle, hw_params)) < 0)
		throw AlsaFreeParamsException(hw_params,
		                              std::string("Cannot set parameters: ")
		                              + snd_strerror(err));

	if ((err = snd_pcm_prepare(playbackHandle)) < 0)
		throw AlsaFreeParamsException(hw_params,
		                              std::string("Cannot prepare audio interface for use: ")
		                              + snd_strerror(err));

	snd_pcm_hw_params_free (hw_params);


	audioBuffer = new boost::int16_t[bufferSize];
	if (!audioBuffer)
		throw SnesException("Unable to allocate audio buffer.");
	std::fill(audioBuffer, audioBuffer + bufferSize, 0);

	mixBuffer = new boost::int16_t[bufferSize];
	if (!mixBuffer)
		throw SnesException("Unable to allocate mix buffer.");
	std::fill(mixBuffer, mixBuffer + bufferSize, 0);

	echoData = boost::shared_ptr<EchoData>(new EchoData(bufferSize));

	threadProcess = new boost::thread(boost::bind(&SoundSystem::processSound, this));

	return;
}

SoundSystem::~SoundSystem()
{
	terminated = true;
	if (threadProcess)
	{
		threadProcess->join();
		delete threadProcess;
	}

	if (playbackHandle)
		snd_pcm_close(playbackHandle);

	if (audioBuffer)
		delete [] audioBuffer;
	if (mixBuffer)
		delete [] mixBuffer;
}


void SoundSystem::processSound()
{
	while (!terminated)
	{
		mixSamples();

		snd_pcm_sframes_t err = snd_pcm_writei(playbackHandle,
		                                       audioBuffer,
		                                       bufferSize/2);
	}
}

void SoundSystem::mixSamples()
{
	if (mute)
	{
		std::fill(audioBuffer, audioBuffer + bufferSize, 0);
		return;
	}

	std::fill(mixBuffer, mixBuffer + bufferSize, 0);
	mixStereo();

	if(echoData->isEchoEnabled())
		echoData->mixSamples(audioBuffer, mixBuffer, bufferSize,
		                     masterVolume);
	else
	{
		int J;
		// 16-bit mono or stereo sound, no echo
		for (J = 0; J < bufferSize; J++)
		{
			int I((mixBuffer [J] *
			       masterVolume[J & 1]) / VOL_DIV16);
			CLIP16(I);
			audioBuffer[J] = I;
		}
	}
}


void SoundSystem::mixStereo()
{
    int pitchModLocal = pitchMod & ~APU.DSP[APU_NON];

    for (unsigned J = 0; J < NUM_CHANNELS; J++)
    {
		Channel *ch = &channels[J];

		if (ch->getState() == SOUND_SILENT
		    || !(soundSwitch & (1 << J)))
			continue;

		bool mod = pitchModLocal & (1 << J);

		ch->decode(mod);

		ch->mixStereo(mixBuffer, bufferSize,
		              mod,
		              pitchModLocal, &noiseGen, playbackRate);
    }
}


void SoundSystem::reset(bool full)
{
	std::for_each(channels.begin(), channels.end(),
	              std::mem_fun_ref(&Channel::reset));

    filterTaps[0] = 127;
    filterTaps[1] = 0;
    filterTaps[2] = 0;
    filterTaps[3] = 0;
    filterTaps[4] = 0;
    filterTaps[5] = 0;
    filterTaps[6] = 0;
    filterTaps[7] = 0;
    mute = true;
    noiseGen = 1;
    soundSwitch = 255;

    if (full)
    {
		masterVolumeLeft = 0;
		masterVolumeRight = 0;
		echoData->reset();

		pitchMod = 0;
		dummy[0] = 0;
		dummy[1] = 0;
		dummy[2] = 0;
		noiseHertz = 0;
    }

    masterVolumeLeft = 127;
    masterVolumeRight = 127;
    masterVolume[0] = masterVolume[1] = 127;
    if (playbackRate)
		errRate = (FIXED_POINT * SNES_SCANLINE_TIME / (1.0 / playbackRate));
    else
		errRate = 0;
}


void SoundSystem::setEchoEnable(boost::uint8_t byte)
{
	echoData->setEnable(byte);

	for (int i = 0; i < channels.size(); ++i)
    {
		if (byte & (1 << i))
			channels[i].setEchoBufPtr(echoData->getBuffer());
		else
			channels[i].setEchoBufPtr(echoData->getDummyBuffer());
    }
}
