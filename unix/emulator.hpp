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

#ifndef _EMULATOR_HPP_
#define _EMULATOR_HPP_

#include <vector>
#include <string>
#include <SDL/SDL.h>

#include "exceptions.hpp"
#include "memmap.h"
#include "apu.hpp"
#include "gfx.hpp"

/* Forward declaration */
class SoundSystem;
class InputController;


/**
   Main class.
 */
class Emulator
{
public:
	/** Constructor.
	    @param [in] arguments Command line arguments, without argv[0].
	*/
	Emulator(const std::vector<std::string> arguments);

	/** Destructor.
	    Destroys all initialized systems like video or sound.
	*/
	virtual ~Emulator();


private:
	/** Parses arguments given to emulator.
	    @param [in] arguments Command line arguments, without argv[0].
	    @return Rom file.
	*/
	std::string parseArgs(const std::vector<std::string> arguments);

	/** Prints usage and throws an ExitException
	 */
	void printUsage() throw (ExitException);

	/** Initializes SDL surfaces */
	void initDisplay();

private:
	SoundSystem *sndSys; /**< Alsa sound system. */
	InputController *inputController;/**< keyboard and joystick management. */
	CMemory memory;
	APUController apu;

	/* Display */
	SDL_Surface *screen;
	SDL_Surface *gfxscreen;
	GFX gfx;

	/* Settings: */
	bool soundEnabled; /**< True if sound are enabled. */
	unsigned soundSkipMethod; /**< Sound skip method. */
	unsigned long cyclesPercentage; /**< Cycles percentage. */
	bool disableHDMA; /**< True if HDMA is disabled. */
	bool PAL; /**< True if we use PAL. False if we use NTSC. */
	unsigned frameTimePAL; /**< Don't know. */
	unsigned frameTimeNTSC; /**< Don't know. */
	unsigned frameTime; /**< Don't know. */
	unsigned skipFrames; /**< Number of frames to skip. */
	bool forceHiROM; /**< Force HiROM. */
	bool forceLoROM; /**< Force LoROM. */
	bool forceHeader; /**< True if force rom header. */
	bool forceNoHeader; /**< True if force no rom header. */
	bool BS; /**< Rom BS. */
	bool supportHiRes; /**< High res support. */
	bool sixteenBit; /**< 16 bits. */
	bool displayFrameRate; /**< Frame rate display. */
	bool swapJoypads; /**< Joypad swap. */
	bool forceInterleaved; /**< Force interleaved. */
	bool forceInterleaved2; /**< Force interleaved 2. */
	bool forceNotInterleaved; /**< Force not interleaved. */
	bool disableIRQ; /**< IRQ deactivation. */
	bool disableSoundEcho; /**< Echo deactivation. */
	unsigned soundPlaybackRate; /**< Playback rate for sound system. */
	bool soundEnvelopeHeightReading; /**< Don't know. */
	bool disableSampleCaching; /**< Don't know. */
	bool disableMasterVolume; /**< Master volume deactivation. */
	unsigned soundSync; /**< Sound synchronisation. */
	bool interpolatedSound; /**< Sound interpolation. */
	int altSampleDecode; /**< Don't know. */
	bool fixFrequency;  /**< Don't know. */
	std::string snapshotFilename; /**< Snapshot to load. */
	bool vga; /**< VGA mode. */
	long H_Max; /**< Don't know. */
	long HBlankStart; /**< Don't know. */
	unsigned controllerOption; /**< Don't know. */
	unsigned autoSaveDelay; /**< Don't know. */

	unsigned resolutionX; /**< Screen width. Default is 320. */
	unsigned resolutionY; /**< Screen height. Default is 240. */ 
	unsigned cl; /**< Don't know. Default is 0. */
	unsigned cs; /**< Don't know. Default is 0. */
	unsigned mfs; /**< Don't know. Default is 10. */


};

#endif
