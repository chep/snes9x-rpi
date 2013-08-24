#include <iostream>

#include "echoData.hpp"
#include "soundConstants.hpp"
#include "snes9x.h"
#include "apu.h"


/********************************** Globals ***********************************/
extern struct SSettings Settings;
extern "C" struct SAPU APU;


/************************************ Code ************************************/
EchoData::EchoData(unsigned bufferSize) throw (SnesException):
	echo(NULL),
	volumeLeft(0),
	volumeRight(0),
	enable(false),
	feedback(0),
	ptr(0),
	bufferSize(bufferSize),
	echoBufferSize(0),
	writeEnabled(false),
	channelEnable(0),
	noFilter(true),
	filterTaps(8),
	Z(0)
{
	buffer = new boost::int16_t[bufferSize];
	dummyBuffer = new boost::int16_t[bufferSize];
	echo = new boost::int16_t[24000];
	if (!buffer || !dummyBuffer || !echo)
		throw SnesException("Unable to allocate echo buffer.");
	std::fill(buffer, buffer + bufferSize, 0);
	std::fill(echo, echo + sizeof(echo), 0);
	std::fill(loop, loop + ECHO_LOOP_SIZE, 0);
	volume[0] = volume[1] = 0;
}

EchoData::~EchoData()
{
	if (echo)
		delete [] echo;
	if (buffer)
		delete [] buffer;
	if (dummyBuffer)
		delete [] dummyBuffer;
}


void EchoData::mixSamples(boost::int16_t *audioBuffer,
                          boost::int16_t *mixBuffer,
                          unsigned audioBufferSize,
                          int masterVolume[2])
{
	int I;
	if (noFilter)
	{
		// ... but no filter defined.
		for (unsigned J = 0; J < audioBufferSize; J++)
		{
			int E = echo[ptr];
			echo[ptr] = (E * feedback) / 128 + buffer[J];
			if ((ptr += 1) >= echoBufferSize)
				ptr = 0;

			I = (mixBuffer[J] * masterVolume [J & 1] +
			     E * volume[J & 1]) / VOL_DIV16;

			CLIP16(I);
			audioBuffer[J] = I;
		}
	}
	else
	{
		// ... with filter defined.
		for (unsigned J = 0; J < audioBufferSize; J++)
		{
			int E = echo[ptr];
			loop[(Z - 0) & 15] = E;
			E =  E                    * filterTaps [0];
			E += loop [(Z -  2) & 15] * filterTaps [1];
			E += loop [(Z -  4) & 15] * filterTaps [2];
			E += loop [(Z -  6) & 15] * filterTaps [3];
			E += loop [(Z -  8) & 15] * filterTaps [4];
			E += loop [(Z - 10) & 15] * filterTaps [5];
			E += loop [(Z - 12) & 15] * filterTaps [6];
			E += loop [(Z - 14) & 15] * filterTaps [7];
			E /= 128;
			Z++;

			echo[ptr] = (E * feedback) / 128 + buffer[J];

			if ((ptr += 1) >= echoBufferSize)
				ptr = 0;

			I = (mixBuffer[J] * masterVolume[J & 1] +
			     E * volume[J & 1]) / VOL_DIV16;

			CLIP16(I);
			audioBuffer[J] = I;
		}
	}
}

void EchoData::reset()
{
	ptr = 0;
	feedback = 0;
	echoBufferSize = 1;
	volumeLeft = 0;
	volumeRight = 0;
	enable = 0;
	writeEnabled = 0;
	channelEnable = 0;
	volume[0] = 0;
	volume[1] = 0;
	noFilter = true;
	filterTaps[0] = 127;
    filterTaps[1] = 0;
    filterTaps[2] = 0;
    filterTaps[3] = 0;
    filterTaps[4] = 0;
    filterTaps[5] = 0;
    filterTaps[6] = 0;
    filterTaps[7] = 0;
}


void EchoData::setEnable(boost::uint8_t byte)
{
    channelEnable = byte;
    if (!writeEnabled || Settings.DisableSoundEcho)
		byte = 0;
    if (byte && !enable)
    {
	    memset (echo, 0, echoBufferSize * sizeof(boost::int16_t));
	    memset (loop, 0, sizeof(loop));
    }

    enable = byte;
}


void EchoData::setDelay(int delay, unsigned playbackRate) throw (SnesException)
{
    echoBufferSize = (512 * delay * playbackRate) / 32000;
    echoBufferSize <<= 1;
    if (echoBufferSize)
		ptr %= echoBufferSize;
    else
		ptr = 0;
    setEnable (APU.DSP[APU_EON]);
}


void EchoData::setVolume(int left, int right)
{
	#warning ajouter les volumes "inutiles" si besoin
	volume[Settings.ReverseStereo] = left;
	volume[1 ^ Settings.ReverseStereo] = right;
}

void EchoData::setFilterCoefficient (int tap, int value)
{
    filterTaps[tap & 7] = value;
    noFilter = (filterTaps[0] == 127 || filterTaps[0] == 0)
	           && filterTaps [1] == 0
		       && filterTaps [2] == 0
		       && filterTaps [3] == 0
		       && filterTaps [4] == 0
		       && filterTaps [5] == 0
	           && filterTaps [6] == 0
		       && filterTaps [7] == 0;
}
