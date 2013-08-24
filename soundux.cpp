/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2003 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2003 Matthew Kendora and
                            Brad Jorsch (anomie@users.sourceforge.net)
 

                      
  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and
                            Nach (n-a-c-h@users.sourceforge.net)
                                          
  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2003 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman (jweidman@slip.net),
                            neviksti (neviksti@hotmail.com), and
                            Kris Bleakley (stinkfish@bigpond.com)
 
  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2003 zsKnight, pagefault (pagefault@zsnes.com)
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar and Gary Henderson.



 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/
// #ifdef __DJGPP__
// #include <allegro.h>
// #undef TRUE
// #endif

// #include <iostream>





// #include "snes9x.h"
// #include "apu.h"
// #include "soundux.h"
// #include "memmap.h"
// #include "cpuexec.h"

// extern int Echo [24000];
// extern int DummyEchoBuffer [SOUND_BUFFER_SIZE];
// extern int MixBuffer [SOUND_BUFFER_SIZE];
// extern int EchoBuffer [SOUND_BUFFER_SIZE];
// extern int FilterTaps [8];
// extern unsigned long Z;
// extern int Loop [16];

// extern long FilterValues[4][2];
// extern int NoiseFreq [32];

// #undef ABS
// #define ABS(a) ((a) < 0 ? -(a) : (a))


// #define VOL_DIV8  0x8000
// #define VOL_DIV16 0x0080
// #define ENVX_SHIFT 24







// void S9xFixSoundAfterSnapshotLoad ()
// {
//     SoundData.echo_write_enabled = !(APU.DSP [APU_FLG] & 0x20);
//     SoundData.echo_channel_enable = APU.DSP [APU_EON];
//     S9xSetEchoDelay (APU.DSP [APU_EDL] & 0xf);
//     S9xSetEchoFeedback ((signed char) APU.DSP [APU_EFB]);
	
//     S9xSetFilterCoefficient (0, (signed char) APU.DSP [APU_C0]);
//     S9xSetFilterCoefficient (1, (signed char) APU.DSP [APU_C1]);
//     S9xSetFilterCoefficient (2, (signed char) APU.DSP [APU_C2]);
//     S9xSetFilterCoefficient (3, (signed char) APU.DSP [APU_C3]);
//     S9xSetFilterCoefficient (4, (signed char) APU.DSP [APU_C4]);
//     S9xSetFilterCoefficient (5, (signed char) APU.DSP [APU_C5]);
//     S9xSetFilterCoefficient (6, (signed char) APU.DSP [APU_C6]);
//     S9xSetFilterCoefficient (7, (signed char) APU.DSP [APU_C7]);
//     for (int i = 0; i < 8; i++)
//     {
// 		SoundData.channels[i].needs_decode = TRUE;
// 		S9xSetSoundFrequency (i, SoundData.channels[i].hertz);
// 		SoundData.channels [i].envxx = SoundData.channels [i].envx << ENVX_SHIFT;
// 		SoundData.channels [i].next_sample = 0;
// 		SoundData.channels [i].interpolate = 0;
// 		SoundData.channels [i].previous [0] = (int32) SoundData.channels [i].previous16 [0];
// 		SoundData.channels [i].previous [1] = (int32) SoundData.channels [i].previous16 [1];
//     }
//     SoundData.master_volume [Settings.ReverseStereo] = SoundData.master_volume_left;
//     SoundData.master_volume [1 ^ Settings.ReverseStereo] = SoundData.master_volume_right;
//     SoundData.echo_volume [Settings.ReverseStereo] = SoundData.echo_volume_left;
//     SoundData.echo_volume [1 ^ Settings.ReverseStereo] = SoundData.echo_volume_right;
//     IAPU.Scanline = 0;
// }

// void S9xSetSoundADSR (int channel, int attack_rate, int decay_rate,
// 					  int sustain_rate, int sustain_level, int release_rate)
// {
//     SoundData.channels[channel].attack_rate = attack_rate;
//     SoundData.channels[channel].decay_rate = decay_rate;
//     SoundData.channels[channel].sustain_rate = sustain_rate;
//     SoundData.channels[channel].release_rate = release_rate;
//     SoundData.channels[channel].sustain_level = sustain_level + 1;
	
//     switch (SoundData.channels[channel].state)
//     {
//     case SOUND_ATTACK:
//     	S9xSetEnvRate (&SoundData.channels [channel], attack_rate, 1, 127);
// //		S9xSetEnvelopeRate (channel, attack_rate, 1, 127);
// 		break;
		
//     case SOUND_DECAY:
//     	S9xSetEnvRate (&SoundData.channels [channel], decay_rate, -1,
// 			(MAX_ENVELOPE_HEIGHT * (sustain_level + 1)) >> 3);
// //		S9xSetEnvelopeRate (channel, decay_rate, -1,
// //			(MAX_ENVELOPE_HEIGHT * (sustain_level + 1)) >> 3);
// 		break;
//     case SOUND_SUSTAIN:
//     	S9xSetEnvRate (&SoundData.channels [channel], sustain_rate, -1, 0);
// //		S9xSetEnvelopeRate (channel, sustain_rate, -1, 0);
// 		break;
//     }
// }





// bool8_32 S9xInitSound (int mode, bool8_32 stereo, int buffer_size)
// {
//     if (!S9xOpenSoundDevice (mode, stereo, buffer_size))
//     {
// 		S9xMessage (S9X_ERROR, S9X_SOUND_DEVICE_OPEN_FAILED,
// 			"Sound device open failed");
// 		return (0);
//     }
	
//     return (1);
// }




