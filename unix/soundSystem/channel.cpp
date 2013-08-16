#include "channel.hpp"
#include "soundConstants.hpp"

#include "snes9x.h"
#include "apu.h"
#include "memmap.h"


/**************************** Constants and macros ****************************/
#define ENVX_SHIFT 24

#define SOUND_BUFFER_SIZE (1024 * 16)

#define JUST_PLAYED_LAST_SAMPLE (samplePointer >= LAST_SAMPLE)
#define LAST_SAMPLE 0xffffff

// F is channel's current frequency and M is the 16-bit modulation waveform
// from the previous channel multiplied by the current envelope volume level.
#define PITCH_MOD(F,M) ((F) * ((((unsigned long) (M)) + 0x800000) >> 16) >> 7)

#define MAX_ENVELOPE_HEIGHT 127

/********************************** Globals ***********************************/
extern struct SSettings Settings;
EXTERN_C struct SIAPU IAPU;
extern int NoiseFreq [32];


/****************************** Static functions ******************************/
static inline void APUSetEndX(int ch)
{
	APU.DSP[APU_ENDX] |= 1 << ch;
}

static inline uint8 *getSampleAddress (int sampleNumber)
{
	uint32 addr = (((APU.DSP[APU_DIR] << 8) + (sampleNumber << 2)) & 0xffff);
	return (IAPU.RAM + addr);
}



/************************************ Code ************************************/
void Channel::decode(bool mod)
{
	unsigned long freq0 = frequency * 985 / 1000;

	if (needsDecode)
	{
		decodeBlock();
		needsDecode = false;
		sample = block[0];
		samplePointer = freq0 >> FIXED_POINT_SHIFT;
		if (samplePointer == 0)
			samplePointer = 1;
		if (samplePointer > SOUND_DECODE_LENGTH)
			samplePointer = SOUND_DECODE_LENGTH - 1;

		nextSample=block[samplePointer];
		interpolate = 0;

		if (Settings.InterpolatedSound && freq0 < FIXED_POINT && !mod)
			interpolate = ((nextSample - sample) *
			               (long) freq0) / (long) FIXED_POINT;
	}
}



void Channel::decodeBlock ()
{
	boost::int32_t out;
	unsigned char filter;
	unsigned char shift;
	signed char sample1, sample2;
	unsigned char i;
	bool invalid_header;
	signed char *compressed = (signed char *) &IAPU.RAM [blockPointer];

	if (Settings.AltSampleDecode)
	{
		altDecodeBlock();
		return;
	}

	if (blockPointer > 0x10000 - 9)
	{
		lastBlock = TRUE;
		loop = FALSE;
		block = decoded;
		return;
	}

	filter = *compressed;
	if ((lastBlock = filter & 1))
		loop = (filter & 2) != 0;

	// If enabled, results in 'tick' sound on some samples that repeat by
	// re-using part of the original sample but generate a slightly different
	// waveform.
	if (!Settings.DisableSampleCaching &&
	    memcmp ((uint8 *) compressed, &IAPU.ShadowRAM[blockPointer], 9) == 0)
	{
		block = (signed short *) (IAPU.CachedSamples + (blockPointer << 2));
		previous[0] = block[15];
		previous[1] = block[14];
	}
	else
	{
		if (!Settings.DisableSampleCaching)
			memcpy (&IAPU.ShadowRAM[blockPointer], (uint8 *) compressed, 9);
		compressed++;
		signed short *raw = block = decoded;

		// Seperate out the header parts used for decoding
		shift = filter >> 4;
		filter = filter&0x0c;

		// Header validity check: if range(shift) is over 12, ignore
		// all bits of the data for that block except for the sign bit of each one
		if ((filter&0xF0) < 0xD0)
		{
			invalid_header = false;
		}
		else
		{
			// fprintf(stderr,"invalid header! header: %02x\n", filter);
			invalid_header = true;
		}

		int32 prev0 = previous[0];
		int32 prev1 = previous[1];

		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			//Sample 2 = Bottom Nibble, Sign Extended.
			sample2 >>= 4;
			//Sample 1 = Top Nibble, shifted down and Sign Extended.
			sample1 >>= 4;

			if (invalid_header) { sample1>>=3; sample2>>=3; }

			for (int nybblesmp = 0; nybblesmp < 2; nybblesmp++)
			{
				out=(((nybblesmp) ? sample2 : sample1) << shift);
				out >>= 1;

				switch(filter)
				{
				case 0x00:
					// Method0 - [Smp]
					break;

				case 0x04:
					// Method1 - [Delta]+[Smp-1](15/16)
					out+=(prev0>>1)+((-prev0)>>5);
					break;

				case 0x08:
					// Method2 - [Delta]+[Smp-1](61/32)-[Smp-2](15/16)
					out+=(prev0)+((-(prev0 +(prev0>>1)))>>5)-(prev1>>1)+(prev1>>5);
					break;

				default:
					// Method3 - [Delta]+[Smp-1](115/64)-[Smp-2](13/16)
					out+=(prev0)+((-(prev0 + (prev0<<2) + (prev0<<3)))>>7)-(prev1>>1)+((prev1+(prev1>>1))>>4);
					break;

				}
				if (out > 32767) out = 32767;
				else if (out < -32768) out = -32768;

				*raw++ = (signed short)(out<<1);
				prev1=(signed short)prev0;
				prev0=(signed short)(out<<1);
			}
		}
		previous[0] = prev0;
		previous[1] = prev1;

		if (!Settings.DisableSampleCaching)
		{
			memcpy (IAPU.CachedSamples + (blockPointer << 2),
			        (uint8 *) decoded, 32);
		}
	}
	blockPointer += 9;
}


void Channel::altDecodeBlock ()
{
	if (blockPointer >= 0x10000 - 9)
	{
		lastBlock = TRUE;
		loop = FALSE;
		block = decoded;
		memset ((void *) decoded, 0, sizeof(decoded));
		return;
	}
	signed char *compressed = (signed char *) &IAPU.RAM [blockPointer];
	unsigned char filter = *compressed;
	if ((lastBlock = filter & 1))
		loop = (filter & 2) != 0;

	int32 out;
	unsigned char shift;
	signed char sample1, sample2;
	unsigned int i;
	signed short *raw = block = decoded;

	int32 prev0 = previous[0];
	int32 prev1 = previous[1];

	compressed++;
	shift = filter >> 4;

	switch ((filter >> 2) & 3)
	{
	case 0:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			sample2 >>= 4;
			sample1 >>= 4;
			*raw++ = ((int32) sample1 << shift);
			*raw++ = ((int32) sample2 << shift);
		}
		prev1 = *(raw - 2);
		prev0 = *(raw - 1);
		break;
	case 1:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			sample2 >>= 4;
			sample1 >>= 4;
			prev0 = (int16) prev0;
			*raw++ = prev1 = ((int32) sample1 << shift) + prev0 - (prev0 >> 4);
			prev1 = (int16) prev1;
			*raw++ = prev0 = ((int32) sample2 << shift) + prev1 - (prev1 >> 4);
		}
		break;
	case 2:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			sample2 >>= 4;
			sample1 >>= 4;

			out = (sample1 << shift) - prev1 + (prev1 >> 4);
			prev1 = (int16) prev0;
			prev0 &= ~3;
			*raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 5) - (prev0 >> 4);

			out = (sample2 << shift) - prev1 + (prev1 >> 4);
			prev1 = (int16) prev0;
			prev0 &= ~3;
			*raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 5) -
				(prev0 >> 4);
		}
		break;
	case 3:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			sample2 >>= 4;
			sample1 >>= 4;
			out = (sample1 << shift);

			out = out - prev1 + (prev1 >> 3) + (prev1 >> 4);
			prev1 = (int16) prev0;
			prev0 &= ~3;
			*raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 3) - (prev0 >> 4) - (prev1 >> 6);

			out = (sample2 << shift);
			out = out - prev1 + (prev1 >> 3) + (prev1 >> 4);
			prev1 = (int16) prev0;
			prev0 &= ~3;
			*raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 3) - (prev0 >> 4) - (prev1 >> 6);
		}
		break;
	}
	previous[0] = prev0;
	previous[1] = prev1;

	blockPointer += 9;
}



void Channel::mixStereo(boost::int16_t *mixBuffer, unsigned bufferSize,
                        bool mod,
                        int pitchMod, int *noiseGen, int playbackRate)
{
	static int wave[SOUND_BUFFER_SIZE] = {0};
	boost::int32_t VL, VR;
	unsigned long freq0 = frequency * 985 / 1000;

	VL = (sample * leftVolLevel) / 128;
	VR = (sample * rightVolLevel) / 128;

	for (unsigned I = 0; I < bufferSize; I += 2)
	{
		unsigned long freq = freq0;

		if (mod)
			freq = PITCH_MOD(freq, wave[I / 2]);

		envError += erate;
		if (envError >= FIXED_POINT)
		{
			uint32 step = envError >> FIXED_POINT_SHIFT;

			switch (state)
			{
			case SOUND_ATTACK:
				envError &= FIXED_POINT_REMAINDER;
				envx += step << 1;
				envxx = envx << ENVX_SHIFT;

				if (envx >= 126)
				{
					envx = 127;
					envxx = 127 << ENVX_SHIFT;
					state = SOUND_DECAY;
					if (sustainLevel != 8)
					{
						setEnvRate(decayRate, -1,
						           (MAX_ENVELOPE_HEIGHT * sustainLevel) >> 3,
						           playbackRate);
						break;
					}
					state = SOUND_SUSTAIN;
					setEnvRate (sustainRate, -1, 0,
					            playbackRate);
				}
				break;

			case SOUND_DECAY:
				while (envError >= FIXED_POINT)
				{
					envxx = (envxx >> 8) * 255;
					envError -= FIXED_POINT;
				}
				envx = envxx >> ENVX_SHIFT;
				if (envx <= envxTarget)
				{
					if (envx <= 0)
					{
						setEndOfSample();
						return;
					}
					state = SOUND_SUSTAIN;
					setEnvRate (sustainRate, -1, 0,
					            playbackRate);
				}
				break;

			case SOUND_SUSTAIN:
				while (envError >= FIXED_POINT)
				{
					envxx = (envxx >> 8) * 255;
					envError -= FIXED_POINT;
				}
				envx = envxx >> ENVX_SHIFT;
				if (envx <= 0)
				{
					setEndOfSample();
					return;
				}
				break;

			case SOUND_RELEASE:
				while (envError >= FIXED_POINT)
				{
					envxx -= (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
					envError -= FIXED_POINT;
				}
				envx = envxx >> ENVX_SHIFT;
				if (envx <= 0)
				{
					setEndOfSample();
					return;
				}
				break;

			case SOUND_INCREASE_LINEAR:
				envError &= FIXED_POINT_REMAINDER;
				envx += step << 1;
				envxx = envx << ENVX_SHIFT;

				if (envx >= 126)
				{
					envx = 127;
					envxx = 127 << ENVX_SHIFT;
					state = SOUND_GAIN;
					mode = MODE_GAIN;
					setEnvRate (0, -1, 0,
					            playbackRate);
				}
				break;

			case SOUND_INCREASE_BENT_LINE:
				if (envx >= (MAX_ENVELOPE_HEIGHT * 3) / 4)
				{
					while (envError >= FIXED_POINT)
					{
						envxx += (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
						envError -= FIXED_POINT;
					}
					envx = envxx >> ENVX_SHIFT;
				}
				else
				{
					envError &= FIXED_POINT_REMAINDER;
					envx += step << 1;
					envxx = envx << ENVX_SHIFT;
				}

				if (envx >= 126)
				{
					envx = 127;
					envxx = 127 << ENVX_SHIFT;
					state = SOUND_GAIN;
					mode = MODE_GAIN;
					setEnvRate (0, -1, 0,
					            playbackRate);
				}
				break;

			case SOUND_DECREASE_LINEAR:
				envError &= FIXED_POINT_REMAINDER;
				envx -= step << 1;
				envxx = envx << ENVX_SHIFT;
				if (envx <= 0)
				{
					setEndOfSample ();
					return;
				}
				break;

			case SOUND_DECREASE_EXPONENTIAL:
				while (envError >= FIXED_POINT)
				{
					envxx = (envxx >> 8) * 255;
					envError -= FIXED_POINT;
				}
				envx = envxx >> ENVX_SHIFT;
				if (envx <= 0)
				{
					setEndOfSample ();
					return;
				}
				break;

			case SOUND_GAIN:
				setEnvRate (0, -1, 0,
				            playbackRate);
				break;
			}
			leftVolLevel = (envx * volumeLeft) / 128;
			rightVolLevel = (envx * volumeRight) / 128;
			VL = (sample * leftVolLevel) / 128;
			VR = (sample * rightVolLevel) / 128;
		}

		count += freq;
		if (count >= FIXED_POINT)
		{
			VL = count >> FIXED_POINT_SHIFT;
			samplePointer += VL;
			count &= FIXED_POINT_REMAINDER;

			sample = nextSample;
			if (samplePointer >= SOUND_DECODE_LENGTH)
			{
				if (JUST_PLAYED_LAST_SAMPLE)
				{
					setEndOfSample ();
					return;
				}
				do
				{
					samplePointer -= SOUND_DECODE_LENGTH;
					if (lastBlock)
					{
						if (!loop)
						{
							samplePointer = LAST_SAMPLE;
							nextSample = sample;
							break;
						}
						else
						{
							APUSetEndX(channelNumber);
							lastBlock = FALSE;
							boost::uint8_t *dir = getSampleAddress (sampleNumber);
							blockPointer = READ_WORD(dir + 2);
						}
					}
					decodeBlock();
				} while (samplePointer >= SOUND_DECODE_LENGTH);
				if (!JUST_PLAYED_LAST_SAMPLE)
					nextSample = block[samplePointer];
			}
			else
				nextSample = block[samplePointer];

			if (type == SOUND_SAMPLE)
			{
				if (Settings.InterpolatedSound && freq < FIXED_POINT && !mod)
				{
					interpolate = ((nextSample - sample) *
					               (long) freq) / (long) FIXED_POINT;
					sample = (boost::int16_t) (sample + (((nextSample - sample) *
					                                      (long) (count)) / (long) FIXED_POINT));
				}
				else
					interpolate = 0;
			}
			else
			{
				for (;VL > 0; VL--)
					if (((*noiseGen) <<= 1) & 0x80000000L)
						(*noiseGen) ^= 0x0040001L;
				sample = ((*noiseGen) << 17) >> 17;
				interpolate = 0;
			}

			VL = (sample * leftVolLevel) / 128;
			VR = (sample * rightVolLevel) / 128;
		}
		else
		{
			if (interpolate)
			{
				boost::int32_t s = (boost::int32_t) sample + interpolate;
				CLIP16(s);
				sample = (boost::int16_t) s;
				VL = (sample * leftVolLevel) / 128;
				VR = (sample * rightVolLevel) / 128;
			}
		}

		if (pitchMod & (1 << (channelNumber + 1)))
			wave[I / 2] = sample * envx;

		mixBuffer[I      ^ Settings.ReverseStereo] += VL;
		mixBuffer[I + (1 ^ Settings.ReverseStereo)] += VR;
		echoBufPtr[I      ^ Settings.ReverseStereo] += VL;
		echoBufPtr[I + (1 ^ Settings.ReverseStereo)] += VR;
	}
}



void Channel::setEnvRate (unsigned long rate, int direction, int target,
                          int playbackRate)
{
	envxTarget = target;

	if (rate == ~0UL)
	{
		this->direction = 0;
		rate = 0;
	}
	else
		this->direction = direction;

	static int steps [] =
		{
			//	0, 64, 1238, 1238, 256, 1, 64, 109, 64, 1238
			0, 64, 619, 619, 128, 1, 64, 55, 64, 619
		};

	if (rate == 0 || playbackRate == 0)
		erate = 0;
	else
	{
		erate = (unsigned long) (((int64) FIXED_POINT * 1000 * steps[state]) /
		                         (rate * playbackRate));
	}
}



void Channel::setEndOfSample ()
{
	state = SOUND_SILENT;
	mode = MODE_NONE;
	APU.DSP [APU_ENDX] |= 1 << channelNumber;
	APU.DSP [APU_KON] &= ~(1 << channelNumber);
	APU.DSP [APU_KOFF] &= ~(1 << channelNumber);
	APU.KeyedChannels &= ~(1 << channelNumber);
}



void Channel::playSample(struct SAPU *apu, int playbackRate)
{
	state = SOUND_SILENT;
	mode = MODE_NONE;
	envx = 0;
	envxx = 0;

	fixEnvelope(apu->DSP [APU_GAIN  + (channelNumber << 4)],
	            apu->DSP [APU_ADSR1 + (channelNumber << 4)],
	            apu->DSP [APU_ADSR2 + (channelNumber << 4)],
	            playbackRate);

	sampleNumber = apu->DSP[APU_SRCN + channelNumber * 0x10];
	if (apu->DSP[APU_NON] & (1 << channelNumber))
		type = SOUND_NOISE;
	else
		type = SOUND_SAMPLE;

	setSoundFrequency (hertz, playbackRate);
	loop = false;
	needsDecode = true;
	lastBlock = false;
	previous[0] = previous[1] = 0;
	boost::uint8_t *dir = getSampleAddress (sampleNumber);
	blockPointer = READ_WORD (dir);
	samplePointer = 0;
	envError = 0;
	nextSample = 0;
	interpolate = 0;
	switch (mode)
	{
	case MODE_ADSR:
		if (attackRate == 0)
		{
			if (decayRate == 0 || sustainLevel == 8)
			{
				state = SOUND_SUSTAIN;
				envx = (MAX_ENVELOPE_HEIGHT * sustainLevel) >> 3;
				setEnvRate (sustainRate, -1, 0, playbackRate);
			}
			else
			{
				state = SOUND_DECAY;
				envx = MAX_ENVELOPE_HEIGHT;
				setEnvRate (decayRate, -1,
				            (MAX_ENVELOPE_HEIGHT * sustainLevel) >> 3,
				            playbackRate);
			}
			leftVolLevel = (envx * volumeLeft) / 128;
			rightVolLevel = (envx * volumeRight) / 128;
		}
		else
		{
			state = SOUND_ATTACK;
			envx = 0;
			leftVolLevel = 0;
			rightVolLevel = 0;
			setEnvRate (attackRate, 1, MAX_ENVELOPE_HEIGHT, playbackRate);
		}
		envxx = envx << ENVX_SHIFT;
		break;

	case MODE_GAIN:
		state = SOUND_GAIN;
		break;

	case MODE_INCREASE_LINEAR:
		state = SOUND_INCREASE_LINEAR;
		break;

	case MODE_INCREASE_BENT_LINE:
		state = SOUND_INCREASE_BENT_LINE;
		break;

	case MODE_DECREASE_LINEAR:
		state = SOUND_DECREASE_LINEAR;
		break;

	case MODE_DECREASE_EXPONENTIAL:
		state = SOUND_DECREASE_EXPONENTIAL;
		break;

	default:
		break;
	}

	fixEnvelope(apu->DSP [APU_GAIN  + (channelNumber << 4)],
	            apu->DSP [APU_ADSR1 + (channelNumber << 4)],
	            apu->DSP [APU_ADSR2 + (channelNumber << 4)],
	            playbackRate);
}


void Channel::fixEnvelope (boost::uint8_t gain, boost::uint8_t adsr1, boost::uint8_t adsr2,
                           int playbackRate)
{
	if (adsr1 & 0x80)
	{
		// ADSR mode
		static unsigned long AttackRate[16] = {
			4100, 2600, 1500, 1000, 640, 380, 260, 160,
			96, 64, 40, 24, 16, 10, 6, 1
		};
		static unsigned long DecayRate[8] = {
			1200, 740, 440, 290, 180, 110, 74, 37
		};
		static unsigned long SustainRate[32] = {
			~0, 38000, 28000, 24000, 19000, 14000, 12000, 9400,
			7100, 5900, 4700, 3500, 2900, 2400, 1800, 1500,
			1200, 880, 740, 590, 440, 370, 290, 220,
			180, 150, 110, 92, 74, 55, 37, 18
		};
		// XXX: can DSP be switched to ADSR mode directly from GAIN/INCREASE/
		// DECREASE mode? And if so, what stage of the sequence does it start
		// at?
		if (setSoundMode (MODE_ADSR))
		{
			// Hack for ROMs that use a very short attack rate, key on a
			// channel, then switch to decay mode. e.g. Final Fantasy II.

			int attack = AttackRate[adsr1 & 0xf];
			if (attack == 1 && (!Settings.SoundSync))
				attack = 0;
			setSoundADSR (attack,
			              DecayRate[(adsr1 >> 4) & 7],
			              SustainRate[adsr2 & 0x1f],
			              (adsr2 >> 5) & 7, 8,
			              playbackRate);
		}
	}
	else
	{
		// Gain mode
		if ((gain & 0x80) == 0)
		{
			if (setSoundMode (MODE_GAIN))
			{
				setEnvRate (0, 0, gain & 0x7f, playbackRate);
				setEnvelopeHeight (gain & 0x7f);
			}
		}
		else
		{
			static unsigned long IncreaseRate [32] = {
				~0, 4100, 3100, 2600, 2000, 1500, 1300, 1000,
				770, 640, 510, 380, 320, 260, 190, 160,
				130, 96, 80, 64, 48, 40, 32, 24,
				20, 16, 12, 10, 8, 6, 4, 2
			};
			static unsigned long DecreaseRateExp [32] = {
				~0, 38000, 28000, 24000, 19000, 14000, 12000, 9400,
				7100, 5900, 4700, 3500, 2900, 2400, 1800, 1500,
				1200, 880, 740, 590, 440, 370, 290, 220,
				180, 150, 110, 92, 74, 55, 37, 18
			};
			if (gain & 0x40)
			{
				// Increase mode
				if (setSoundMode ((gain & 0x20) ?
				                  MODE_INCREASE_BENT_LINE :
				                  MODE_INCREASE_LINEAR))
					setEnvRate (IncreaseRate [gain & 0x1f],
					            1, 127, playbackRate);
			}
			else
			{
				uint32 rate = (gain & 0x20) ?
					          DecreaseRateExp [gain & 0x1f] / 2 :
					          IncreaseRate [gain & 0x1f];
				int newMode = (gain & 0x20) ?
					          MODE_DECREASE_EXPONENTIAL
					          : MODE_DECREASE_LINEAR;

				if (setSoundMode (mode))
					setEnvRate (rate, -1, 0, playbackRate);
			}
		}
	}
}


bool Channel::setSoundMode (SoundMode newMode)
{
	switch (newMode)
	{
	case MODE_RELEASE:
		if (mode != MODE_NONE)
		{
			mode = MODE_RELEASE;
			return true;
		}
		break;

	case MODE_DECREASE_LINEAR:
	case MODE_DECREASE_EXPONENTIAL:
	case MODE_GAIN:
		if (mode != MODE_RELEASE)
		{
			mode = newMode;
			if (state != SOUND_SILENT)
				state = SoundState(newMode);
			return true;
		}
		break;

	case MODE_INCREASE_LINEAR:
	case MODE_INCREASE_BENT_LINE:
		if (mode != MODE_RELEASE)
		{
			mode = newMode;
			if (state != SOUND_SILENT)
				state = SoundState(newMode);
			return true;
		}
		break;

	case MODE_ADSR:
		if (mode == MODE_NONE || mode == MODE_ADSR)
		{
			mode = newMode;
			return true;
		}
	}
	return (false);
}

void Channel::setSoundADSR (int attack_rate, int decay_rate,
                            int sustain_rate, int sustain_level, int release_rate,
                            int playbackRate)
{
	attackRate = attack_rate;
	decayRate = decay_rate;
	sustainRate = sustain_rate;
	releaseRate = release_rate;
	sustainLevel = sustain_level + 1;

	switch (state)
	{
	case SOUND_ATTACK:
		setEnvRate (attack_rate, 1, 127, playbackRate);
		break;

	case SOUND_DECAY:
		setEnvRate (decay_rate, -1,
		            (MAX_ENVELOPE_HEIGHT * (sustain_level + 1)) >> 3,
		            playbackRate);
		break;
	case SOUND_SUSTAIN:
		setEnvRate (sustain_rate, -1, 0, playbackRate);
		break;
	}
}


void Channel::setEnvelopeHeight (int level)
{
    envx = level;
    envxx = level << ENVX_SHIFT;

    leftVolLevel = (level * volumeLeft) / 128;
    rightVolLevel = (level * volumeRight) / 128;

    if (envx == 0 && state != SOUND_SILENT && state != SOUND_GAIN)
		setEndOfSample ();
}


void Channel::setSoundFrequency (int hertz, int playbackRate)
{
    if (playbackRate)
    {
		if (type == SOUND_NOISE)
			hertz = NoiseFreq [APU.DSP [APU_FLG] & 0x1f];
		frequency = ((boost::int64_t(hertz) * FIXED_POINT) / playbackRate);
		if (Settings.FixFrequency)
			frequency =	double(frequency) * 0.980;
    }
}


void Channel::reset()
{
	state = SOUND_SILENT;
	mode = MODE_NONE;
	type = SOUND_SAMPLE;
	volumeLeft = 0;
	volumeRight = 0;
	hertz = 0;
	count = 0;
	loop = FALSE;
	envxTarget = 0;
	envError = 0;
	erate = 0;
	envx = 0;
	envxx = 0;
	leftVolLevel = 0;
	rightVolLevel = 0;
	direction = 0;
	attackRate = 0;
	decayRate = 0;
	sustainRate = 0;
	releaseRate = 0;
	sustainLevel = 0;
}
