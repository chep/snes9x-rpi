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

#ifdef __DJGPP
//#include <allegro.h>
#undef TRUE
#endif

#include "snes9x.h"
#include "spc700.h"
#include "apu.hpp"
#include "soundux.h"
/*#include "cpuexec.h"*/
#include "port.h"

extern int NoiseFreq [32];
#ifdef DEBUGGER
void S9xTraceSoundDSP (const char *s, int i1 = 0, int i2 = 0, int i3 = 0,
		       int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0);
#endif


const uint8 APUROM [64] =
{
    0xCD,0xEF,0xBD,0xE8,0x00,0xC6,0x1D,0xD0,0xFC,0x8F,0xAA,0xF4,0x8F,
    0xBB,0xF5,0x78,0xCC,0xF4,0xD0,0xFB,0x2F,0x19,0xEB,0xF4,0xD0,0xFC,
    0x7E,0xF4,0xD0,0x0B,0xE4,0xF5,0xCB,0xF4,0xD7,0x00,0xFC,0xD0,0xF3,
    0xAB,0x01,0x10,0xEF,0x7E,0xF4,0x10,0xEB,0xBA,0xF6,0xDA,0x00,0xBA,
    0xF4,0xC4,0xF4,0xDD,0x5D,0xD0,0xDB,0x1F,0x00,0x00,0xC0,0xFF
};


// Raw SPC700 instruction cycle lengths
const int32 S9xAPUCycleLengths [256] = 
{
    /*        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, a, b, c, d, e, f, */
    /* 00 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 6, 8, 
    /* 10 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 4, 6, 
    /* 20 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 5, 4, 
    /* 30 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 3, 8, 
    /* 40 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 6, 6, 
    /* 50 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 4, 5, 2, 2, 4, 3, 
    /* 60 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 5, 5, 
    /* 70 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 6, 
    /* 80 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 2, 4, 5, 
    /* 90 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2,12, 5, 
    /* a0 */  3, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 2, 4, 4, 
    /* b0 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 4, 
    /* c0 */  3, 8, 4, 5, 4, 5, 4, 7, 2, 5, 6, 4, 5, 2, 4, 9, 
    /* d0 */  2, 8, 4, 5, 5, 6, 6, 7, 4, 5, 4, 5, 2, 2, 6, 3, 
    /* e0 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 4, 5, 3, 4, 3, 4, 3, 
    /* f0 */  2, 8, 4, 5, 4, 5, 5, 6, 3, 4, 5, 4, 2, 2, 4, 3
};

// Actual data used by CPU emulation, will be scaled by APUReset routine
// to be relative to the 65c816 instruction lengths.
const int32 S9xAPUDefaultCycles [256] =
{
    /*        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, a, b, c, d, e, f, */
    /* 00 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 6, 8, 
    /* 10 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 4, 6, 
    /* 20 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 5, 4, 
    /* 30 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 3, 8, 
    /* 40 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 6, 6, 
    /* 50 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 4, 5, 2, 2, 4, 3, 
    /* 60 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 5, 5, 
    /* 70 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 6, 
    /* 80 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 2, 4, 5, 
    /* 90 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2,12, 5, 
    /* a0 */  3, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 2, 4, 4, 
    /* b0 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 4, 
    /* c0 */  3, 8, 4, 5, 4, 5, 4, 7, 2, 5, 6, 4, 5, 2, 4, 9, 
    /* d0 */  2, 8, 4, 5, 5, 6, 6, 7, 4, 5, 4, 5, 2, 2, 6, 3, 
    /* e0 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 4, 5, 3, 4, 3, 4, 3, 
    /* f0 */  2, 8, 4, 5, 4, 5, 5, 6, 3, 4, 5, 4, 2, 2, 4, 3
};




/*******************************************************************************
 *************************************IAPU**************************************
 ******************************************************************************/


IAPU::IAPU(uint8 RAMInitialValue):
	RAM(0x10000),
	PC(RAM.begin()),
	directPage(RAM.begin()),
	APUExecuting(0),
	bit(0),
	address(0),
	waitAddress1(NULL),
	waitAddress2(NULL),
	waitCounter(0),
	shadowRAM(0x10000),
	cachedSamples(0x40000),
	carry(false),
	zero(false),
	overflow(false),
	timerErrorCounter(0),
    scanline(0),
    oneCycle(0),
	twoCycles(0),
	RAMInitialValue(RAMInitialValue)
{
}

IAPU::~IAPU ()
{
}

void IAPU::reset()
{
	std::fill(RAM.begin(), RAM.end(), RAMInitialValue);
	std::fill(shadowRAM.begin(), shadowRAM.end(), RAMInitialValue);
	std::fill(cachedSamples.begin(), cachedSamples.end(), 0);
	resetROM();
	PC = RAM.begin() + RAM[0xfffe] + (RAM[0xffff] << 8);
	directPage = RAM.begin();

    waitAddress1 = NULL;
    waitAddress2 = NULL;
    waitCounter = 0;

    RAM[0xf1] = 0x80;
    twoCycles = oneCycle * 2;
}


void IAPU::resetROM()
{
	std::copy(APUROM, APUROM + sizeof(APUROM), RAM.begin() + 0xffc0);
}


void IAPU::fillRAM(unsigned offset, const std::vector<uint8>& src)
{
	std::copy(src.begin(), src.end(), RAM.begin() + offset);
}

/*******************************************************************************
 **************************************APU**************************************
 ******************************************************************************/


APU::APU():
	cycles(0),
	showROM(false),
	flags(0),
	keyedChannels(0),
	outPorts(4),
	DSP(0x80),
	extraRAM(64),
	timer(3),
	timerTarget(3),
	timerEnabled(3),
	timerValueWritten(3),
	vCycles(256)
{
	std::copy(S9xAPUDefaultCycles,
	          S9xAPUDefaultCycles + sizeof(S9xAPUDefaultCycles),
	          vCycles.begin());
}

void APU::reset()
{
	std::fill(outPorts.begin(), outPorts.end(), 0);
	std::copy(APUROM, APUROM + sizeof(APUROM), extraRAM.begin());
	cycles = 0;
    registers.YA.W = 0;
    registers.X = 0;
    registers.S = 0xff;
    registers.P = 0;
    registers.PC = 0;
    showROM = true;

    std::fill(timerEnabled.begin(), timerEnabled.end(), false);
    std::fill(timerValueWritten.begin(), timerValueWritten.end(), false);
    std::fill(timerTarget.begin(), timerTarget.end(), 0);
    std::fill(timer.begin(), timer.end(), 0);
    std::fill(DSP.begin(), DSP.end(), 0);

    DSP[APU_ENDX] = 0;
    DSP[APU_KOFF] = 0;
    DSP[APU_KON] = 0;
    DSP[APU_FLG] = APU_MUTE | APU_ECHO_DISABLED;
    keyedChannels = 0;
}


void APU::setCycles(int32 iapuOneCycle)
{
	for (unsigned i = 0; i < vCycles.size(); i++)
		vCycles[i] = S9xAPUCycleLengths [i] * iapuOneCycle;
}


void APU::setDSP (uint8 byte, IAPU& iapu)
{
    uint8 reg = iapu[0xf2];
    int i;

    switch (reg)
    {
    case APU_FLG:
	    if (byte & APU_SOFT_RESET)
	    {
		    DSP[reg] = APU_MUTE | APU_ECHO_DISABLED | (byte & 0x1f);
		    DSP[APU_ENDX] = 0;
		    DSP[APU_KOFF] = 0;
		    DSP[APU_KON] = 0;
		    S9xSetEchoWriteEnable (FALSE);
		    // Kill sound
		    S9xResetSound (FALSE);
	    }
	    else
	    {
		    S9xSetEchoWriteEnable (!(byte & APU_ECHO_DISABLED));
		    if (byte & APU_MUTE)
			    S9xSetSoundMute (TRUE);
		    else
			    S9xSetSoundMute (FALSE);

		    SoundData.noise_hertz = NoiseFreq [byte & 0x1f];
		    for (i = 0; i < 8; i++)
		    {
			    if (SoundData.channels [i].type == SOUND_NOISE)
				    S9xSetSoundFrequency (i, SoundData.noise_hertz);
		    }
	    }
	    break;

    case APU_NON:
	    if (byte != DSP[APU_NON])
	    {
		    uint8 mask = 1;
		    for (int c = 0; c < 8; c++, mask <<= 1)
		    {
			    int type;
			    if (byte & mask)
				    type = SOUND_NOISE;
			    else
				    type = SOUND_SAMPLE;
			    SoundData.channels[c].type = type;
		    }
	    }
	    break;
    case APU_MVOL_LEFT:
	    if (byte != DSP[APU_MVOL_LEFT])
	    {
		    SoundData.master_volume_left = (signed char) byte;
		    SoundData.master_volume_right = (signed char) DSP[APU_MVOL_RIGHT];
		    SoundData.master_volume [Settings.ReverseStereo] = (signed char) byte;
		    SoundData.master_volume [1 ^ Settings.ReverseStereo] = (signed char) DSP[APU_MVOL_RIGHT];
	    }
	    break;
    case APU_MVOL_RIGHT:
	    if (byte != DSP[APU_MVOL_RIGHT])
	    {
		    SoundData.master_volume_left = (signed char) DSP [APU_MVOL_LEFT];
		    SoundData.master_volume_right = (signed char) (signed char) byte;
		    SoundData.master_volume [Settings.ReverseStereo] = (signed char) DSP [APU_MVOL_LEFT];
		    SoundData.master_volume [1 ^ Settings.ReverseStereo] = (signed char) byte;
	    }
	    break;
    case APU_EVOL_LEFT:
	    if (byte != DSP [APU_EVOL_LEFT])
	    {
		    SoundData.echo_volume_left = (signed char) byte;
		    SoundData.echo_volume_right = (signed char) DSP [APU_EVOL_RIGHT];
		    SoundData.echo_volume [Settings.ReverseStereo] = (signed char) byte;
		    SoundData.echo_volume [1 ^ Settings.ReverseStereo] = (signed char) DSP [APU_EVOL_RIGHT];
	    }
	    break;
    case APU_EVOL_RIGHT:
	    if (byte != DSP [APU_EVOL_RIGHT])
	    {
		    SoundData.echo_volume_left = (signed char) DSP [APU_EVOL_LEFT];
		    SoundData.echo_volume_right = (signed char) byte;
		    SoundData.echo_volume [Settings.ReverseStereo] = (signed char) DSP [APU_EVOL_LEFT];
		    SoundData.echo_volume [1 ^ Settings.ReverseStereo] = (signed char) byte;
	    }
	    break;
    case APU_ENDX:
	    byte = 0;
	    break;

    case APU_KOFF:
	    if (byte)
	    {
		    uint8 mask = 1;
		    for (int c = 0; c < 8; c++, mask <<= 1)
		    {
			    if ((byte & mask) != 0)
			    {
				    if (keyedChannels & mask)
				    {
					    {
						    keyedChannels &= ~mask;
						    DSP[APU_KON] &= ~mask;
						    S9xSetSoundKeyOff (c);
					    }
				    }
			    }
		    }
	    }
	    DSP[APU_KOFF] = byte;
	    return;
    case APU_KON:
	    if (byte)
	    {
		    uint8 mask = 1;
		    for (int c = 0; c < 8; c++, mask <<= 1)
		    {
			    if ((byte & mask) != 0)
			    {
				    // Pac-In-Time requires that channels can be key-on
				    // regardeless of their current state.
				    keyedChannels |= mask;
				    DSP[APU_KON] |= mask;
				    DSP[APU_KOFF] &= ~mask;
				    DSP[APU_ENDX] &= ~mask;
				    #warning à décommenter
//				    S9xPlaySample (c, this);
			    }
		    }
	    }
	    return;
	
    case APU_VOL_LEFT + 0x00:
    case APU_VOL_LEFT + 0x10:
    case APU_VOL_LEFT + 0x20:
    case APU_VOL_LEFT + 0x30:
    case APU_VOL_LEFT + 0x40:
    case APU_VOL_LEFT + 0x50:
    case APU_VOL_LEFT + 0x60:
    case APU_VOL_LEFT + 0x70:
// At Shin Megami Tensei suggestion 6/11/00
//	if (byte != apu->DSP [reg])
    {
	    int ch = reg >> 4;
	    SoundData.channels[ch].volume_left = (signed char) byte;
	    SoundData.channels[ch].volume_right = (signed char) DSP[reg + 1];
	    SoundData.channels[ch].left_vol_level = (SoundData.channels[ch].envx * (signed char) byte) / 128;
	    SoundData.channels[ch].right_vol_level = (SoundData.channels[ch].envx * (signed char) DSP[reg + 1]) / 128;
    }
    break;
    case APU_VOL_RIGHT + 0x00:
    case APU_VOL_RIGHT + 0x10:
    case APU_VOL_RIGHT + 0x20:
    case APU_VOL_RIGHT + 0x30:
    case APU_VOL_RIGHT + 0x40:
    case APU_VOL_RIGHT + 0x50:
    case APU_VOL_RIGHT + 0x60:
    case APU_VOL_RIGHT + 0x70:
// At Shin Megami Tensei suggestion 6/11/00
//	if (byte != apu->DSP [reg])
    {
	    int ch = reg >> 4;
	    SoundData.channels[ch].volume_left = (signed char) DSP[reg - 1];
	    SoundData.channels[ch].volume_right = (signed char) byte;
	    SoundData.channels[ch].left_vol_level = (SoundData.channels[ch].envx
	                                             * (signed char) DSP[reg - 1]) / 128;
	    SoundData.channels[ch].right_vol_level = (SoundData.channels[ch].envx
	                                              * (signed char) byte) / 128;
    }
    break;

    case APU_P_LOW + 0x00:
    case APU_P_LOW + 0x10:
    case APU_P_LOW + 0x20:
    case APU_P_LOW + 0x30:
    case APU_P_LOW + 0x40:
    case APU_P_LOW + 0x50:
    case APU_P_LOW + 0x60:
    case APU_P_LOW + 0x70:
    {
	    int ch = reg >> 4;
	    int hertz = ((byte + (DSP [reg + 1] << 8)) & FREQUENCY_MASK) * 8;
	    SoundData.channels[ch].hertz = hertz;
	    S9xSetSoundFrequency (ch, hertz);break;
    }
    case APU_P_HIGH + 0x00:
    case APU_P_HIGH + 0x10:
    case APU_P_HIGH + 0x20:
    case APU_P_HIGH + 0x30:
    case APU_P_HIGH + 0x40:
    case APU_P_HIGH + 0x50:
    case APU_P_HIGH + 0x60:
    case APU_P_HIGH + 0x70:
    {
	    int ch = reg >> 4;
	    int hertz = (((byte << 8) + DSP[reg - 1]) & FREQUENCY_MASK) * 8;
	    SoundData.channels[ch].hertz = hertz;
	    S9xSetSoundFrequency(ch, hertz);
    }
    break;

    case APU_SRCN + 0x00:
    case APU_SRCN + 0x10:
    case APU_SRCN + 0x20:
    case APU_SRCN + 0x30:
    case APU_SRCN + 0x40:
    case APU_SRCN + 0x50:
    case APU_SRCN + 0x60:
    case APU_SRCN + 0x70:
	    if (byte != DSP [reg])
		    S9xSetSoundSample (reg >> 4, byte);
	    break;
	
    case APU_ADSR1 + 0x00:
    case APU_ADSR1 + 0x10:
    case APU_ADSR1 + 0x20:
    case APU_ADSR1 + 0x30:
    case APU_ADSR1 + 0x40:
    case APU_ADSR1 + 0x50:
    case APU_ADSR1 + 0x60:
    case APU_ADSR1 + 0x70:
	    if (byte != DSP[reg])
		    S9xFixEnvelope (reg >> 4, DSP[reg + 2], byte, 
		                    DSP[reg + 1]);
	    break;

    case APU_ADSR2 + 0x00:
    case APU_ADSR2 + 0x10:
    case APU_ADSR2 + 0x20:
    case APU_ADSR2 + 0x30:
    case APU_ADSR2 + 0x40:
    case APU_ADSR2 + 0x50:
    case APU_ADSR2 + 0x60:
    case APU_ADSR2 + 0x70:
	    if (byte != DSP[reg])
		    S9xFixEnvelope (reg >> 4, DSP[reg + 1], DSP[reg - 1],
		                    byte);
	    break;

    case APU_GAIN + 0x00:
    case APU_GAIN + 0x10:
    case APU_GAIN + 0x20:
    case APU_GAIN + 0x30:
    case APU_GAIN + 0x40:
    case APU_GAIN + 0x50:
    case APU_GAIN + 0x60:
    case APU_GAIN + 0x70:
	    if (byte != DSP[reg])
		    S9xFixEnvelope (reg >> 4, byte, DSP[reg - 2],
		                    DSP[reg - 1]);
	    break;

    case APU_ENVX + 0x00:
    case APU_ENVX + 0x10:
    case APU_ENVX + 0x20:
    case APU_ENVX + 0x30:
    case APU_ENVX + 0x40:
    case APU_ENVX + 0x50:
    case APU_ENVX + 0x60:
    case APU_ENVX + 0x70:
	    break;

    case APU_OUTX + 0x00:
    case APU_OUTX + 0x10:
    case APU_OUTX + 0x20:
    case APU_OUTX + 0x30:
    case APU_OUTX + 0x40:
    case APU_OUTX + 0x50:
    case APU_OUTX + 0x60:
    case APU_OUTX + 0x70:
	    break;
    
    case APU_DIR:
	    break;

    case APU_PMON:
	    if (byte != DSP[APU_PMON])
		    S9xSetFrequencyModulationEnable (byte);
	    break;

    case APU_EON:
	    if (byte != DSP[APU_EON])
		    S9xSetEchoEnable (byte);
	    break;

    case APU_EFB:
	    S9xSetEchoFeedback((signed char) byte);
	    break;

    case APU_ESA:
	    break;

    case APU_EDL:
	    S9xSetEchoDelay(byte & 0xf);
	    break;

    case APU_C0:
    case APU_C1:
    case APU_C2:
    case APU_C3:
    case APU_C4:
    case APU_C5:
    case APU_C6:
    case APU_C7:
	    S9xSetFilterCoefficient(reg >> 4, (signed char) byte);
	    break;
    default:
// XXX
//printf ("Write %02x to unknown APU register %02x\n", byte, reg);
	    break;
    }

    if (reg < 0x80)
	    DSP[reg] = byte;
}


void APU::setControl (uint8 byte, IAPU& iapu)
{
	if ((byte & 1) != 0 && !timerEnabled[0])
    {
        timer[0] = 0;
        iapu[0xfd] = 0;
        timerTarget[0] = iapu[0xfa] ? iapu[0xfa]:0x100;
    }
    if ((byte & 2) != 0 && !timerEnabled[1])
    {
        timer[1] = 0;
        iapu[0xfe] = 0;
        timerTarget[1] = iapu[0xfb] ? iapu[0xfb]:0x100;
    }
    if ((byte & 4) != 0 && !timerEnabled[2])
    {
        timer[2] = 0;
        iapu[0xff] = 0;
        timerTarget[2] = iapu[0xfc] ? iapu[0xfc]:0x100;
    }
    timerEnabled[0] = byte & 1;
    timerEnabled[1] = (byte & 2) >> 1;
    timerEnabled[2] = (byte & 4) >> 2;

    if (byte & 0x10)
	    iapu[0xF4] = iapu[0xF5] = 0;

    if (byte & 0x20)
        iapu[0xF6] = iapu[0xF7] = 0;

    if (byte & 0x80)
    {
        if (!showROM)
        {
	        iapu.resetROM();
            showROM = true;
        }
    }
    else
    {
        if (showROM)
        {
            showROM = false;
            iapu.fillRAM(0xffc0, extraRAM);
        }
    }
    iapu[0xf1] = byte;
}



void APU::setTimer(uint16 address, uint8 byte, IAPU& iapu)
{
    iapu[address] = byte;

    switch (address)
    {
    case 0xfa:
        timerTarget[0] = iapu[0xfa] ? iapu[0xfa]:0x100;
	    timerValueWritten[0] = true;
	    break;
    case 0xfb:
        timerTarget[1] = iapu[0xfb] ? iapu[0xfb]:0x100;
	    timerValueWritten[1] = true;
	    break;
    case 0xfc:
        timerTarget[2] = iapu[0xfc] ? iapu[0xfc]:0x100;
	    timerValueWritten[2] = true;
	    break;
    }
}

/*******************************************************************************
 **********************************APUController********************************
 ******************************************************************************/

APUController::APUController(uint8 RAMInitialValue):
	iapu(RAMInitialValue),
	APUEnabled(true),
	nextAPUEnabled(true)
{
}


void APUController::reset ()
{
    APUEnabled = nextAPUEnabled;

    iapu.reset();
    apu.reset();
    
    unpackStatus ();

    iapu.setAPUExecuting(APUEnabled);
    apu.setCycles(iapu.getOneCycle());

    S9xResetSound (TRUE);
    S9xSetEchoEnable (0);
}


void APUController::unpackStatus()
{
	const uint8_32 P(apu.getP());
	iapu.setZero((P & Zero == 0) | (P & Negative));
	iapu.setCarry(P & Carry);
	iapu.setOverflow((P & Overflow) >> 6);
}



void S9xFixEnvelope (int channel, uint8 gain, uint8 adsr1, uint8 adsr2)
{
    if (adsr1 & 0x80)
    {
	// ADSR mode
	static unsigned long AttackRate [16] = {
	    4100, 2600, 1500, 1000, 640, 380, 260, 160,
	    96, 64, 40, 24, 16, 10, 6, 1
	};
	static unsigned long DecayRate [8] = {
	    1200, 740, 440, 290, 180, 110, 74, 37
	};
	static unsigned long SustainRate [32] = {
	    ~0, 38000, 28000, 24000, 19000, 14000, 12000, 9400,
	    7100, 5900, 4700, 3500, 2900, 2400, 1800, 1500,
	    1200, 880, 740, 590, 440, 370, 290, 220,
	    180, 150, 110, 92, 74, 55, 37, 18
	};
	// XXX: can DSP be switched to ADSR mode directly from GAIN/INCREASE/
	// DECREASE mode? And if so, what stage of the sequence does it start
	// at?
	if (S9xSetSoundMode (channel, MODE_ADSR))
	{
	    // Hack for ROMs that use a very short attack rate, key on a 
	    // channel, then switch to decay mode. e.g. Final Fantasy II.

	    int attack = AttackRate [adsr1 & 0xf];

	    if (attack == 1 && (!Settings.SoundSync
#ifdef __WIN32__
                || Settings.SoundDriver != WIN_SNES9X_DIRECT_SOUND_DRIVER
#endif
                ))
		attack = 0;

	    S9xSetSoundADSR (channel, attack,
			     DecayRate [(adsr1 >> 4) & 7],
			     SustainRate [adsr2 & 0x1f],
			     (adsr2 >> 5) & 7, 8);
	}
    }
    else
    {
	// Gain mode
	if ((gain & 0x80) == 0)
	{
	    if (S9xSetSoundMode (channel, MODE_GAIN))
	    {
		S9xSetEnvelopeRate (channel, 0, 0, gain & 0x7f);
		S9xSetEnvelopeHeight (channel, gain & 0x7f);
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
		if (S9xSetSoundMode (channel, (gain & 0x20) ?
					  MODE_INCREASE_BENT_LINE :
					  MODE_INCREASE_LINEAR))
		{
		    S9xSetEnvelopeRate (channel, IncreaseRate [gain & 0x1f],
					1, 127);
		}
	    }
	    else
	    {
		uint32 rate = (gain & 0x20) ? DecreaseRateExp [gain & 0x1f] / 2 :
					      IncreaseRate [gain & 0x1f];
		int mode = (gain & 0x20) ? MODE_DECREASE_EXPONENTIAL
					 : MODE_DECREASE_LINEAR;

		if (S9xSetSoundMode (channel, mode))
		    S9xSetEnvelopeRate (channel, rate, -1, 0);
	    }
	}
    }
}

void APUController::setAPUControl (uint8 byte)
{
	apu.setControl(byte, iapu);
}


void APUController::setAPUTimer (uint16 address, uint8 byte)
{
	apu.setTimer(address, byte, iapu);
}


uint8 APUController::getAPUDSP ()
{
	uint8 reg = iapu[0xf2] & 0x7f;
	uint8 byte = apu.getDSP(reg);

	switch (reg)
	{
	case APU_KON:
		break;
	case APU_KOFF:
		break;
	case APU_OUTX + 0x00:
	case APU_OUTX + 0x10:
	case APU_OUTX + 0x20:
	case APU_OUTX + 0x30:
	case APU_OUTX + 0x40:
	case APU_OUTX + 0x50:
	case APU_OUTX + 0x60:
	case APU_OUTX + 0x70:
		if (SoundData.channels [reg >> 4].state == SOUND_SILENT)
			return (0);
		return ((SoundData.channels [reg >> 4].sample >> 8) |
		        (SoundData.channels [reg >> 4].sample & 0xff));

	case APU_ENVX + 0x00:
	case APU_ENVX + 0x10:
	case APU_ENVX + 0x20:
	case APU_ENVX + 0x30:
	case APU_ENVX + 0x40:
	case APU_ENVX + 0x50:
	case APU_ENVX + 0x60:
	case APU_ENVX + 0x70:
		return ((uint8) S9xGetEnvelopeHeight (reg >> 4));

	case APU_ENDX:
// To fix speech in Magical Drop 2 6/11/00
//	APU.DSP [APU_ENDX] = 0;
		break;
	default:
		break;
	}
	return (byte);
}
