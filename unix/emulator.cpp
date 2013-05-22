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
#include <boost/lexical_cast.hpp>

#include "emulator.hpp"
#include "snes9x.h"

Emulator::Emulator(const std::vector<std::string> arguments):
	sndSys(NULL),
	inputController(NULL),
	APUController(0),

	soundEnabled(true),
	soundSkipMethod(0),
	cyclesPercentage(100),
	disableHDMA(false),
	PAL(false),
	frameTimePAL(20000),
	frameTimeNTSC(16667),
	frameTime(frameTimeNTSC),
	skipFrames(AUTO_FRAMERATE),
	forceHiROM(false),
	forceLoROM(false),
	forceHeader(false),
	forceNoHeader(false),
	BS(false),
	supportHiRes(false),
	sixteenBit(true),
	displayFrameRate(false),
	swapJoypads(false),
	forceInterleaved(false),
	forceInterleaved2(false),
	forceNotInterleaved(false),
	disableIRQ(false),
	disableSoundEcho(false),
	soundPlaybackRate(7),
	soundEnvelopeHeightReading(false),
	disableSampleCaching(false),
	disableMasterVolume(false),
	soundSync(0),
	interpolatedSound(false),
	altSampleDecode(0),
	fixFrequency(false),
	snapshotFilename(""),
	vga(false),
	H_Max(SNES_CYCLES_PER_SCANLINE),
	HBlankStart((256 * Settings.H_Max) / SNES_HCOUNTER_MAX),
	controllerOption(0),
	autoSaveDelay(30),
	resolutionX(320),
	resolutionY(240),
	cl(0),
	cs(0),
	mfs(10)
{
	std::string romFilename(parseArgs(arguments));

	if (!Memory.Init ())
		throw SnesException("Out of memory");

	S9xInitSound (soundPlaybackRate, true,
	              8192);

	initDisplay();
}


Emulator::~Emulator()
{
    if (sndSys)
	    delete sndSys;

    if (inputController)
	    delete inputController;

    S9xDeinitDisplay ();
    Memory.SaveSRAM (S9xGetFilename (".srm"));

	Memory.DeInit();
}



void Emulator::initDisplay ()
{
	if (SDL_Init(SDL_INIT_VIDEO
	             | SDL_INIT_JOYSTICK
	             | (apu.getNextAPUEnabled() ? SDL_INIT_AUDIO : 0)) < 0 ) 
		throw SnesException("Unable to init SDL");

	atexit(SDL_Quit);
	screen = SDL_SetVideoMode(xs, ys, 16, SDL_SWSURFACE);
	SDL_ShowCursor(0); // rPi: we're not really interested in showing a mouse cursor


	if (screen == NULL)
		throw SnesException("Couldn't set video mode: " + SDL_GetError());

//Refaire GFX
	if (supportHiRes)
	{
		gfxscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, 512, 480, 16, 0, 0, 0, 0);
		GFX.Screen = (uint8 *)gfxscreen->pixels;
		GFX.Pitch = 512 * 2;
	} else
	{
		GFX.Screen = (uint8 *)screen->pixels + 64;
		GFX.Pitch = 320 * 2;
	}
	GFX.SubScreen = (uint8 *)malloc(512 * 480 * 2);
	GFX.ZBuffer = (uint8 *)malloc(512 * 480 * 2);
	GFX.SubZBuffer = (uint8 *)malloc(512 * 480 * 2);
}


std::string Emulator::parseArgs(const std::vector<std::string> arguments)
{
	std::string romFilename("");

	if (arguments.empty())
		printUsage();

	for (std::vector<std::string>::const_iterator arg = arguments.begin();
	     arg != arguments.end();
	     arg++)
	{
		if (arg->at(0) == '-')
		{
			if (*arg == "-so"
			    || *arg == "-sound")
				soundEnabled = true;
			else if (*arg == "-ns"
			         || *arg == "-nosound")
				soundEnabled = false;

			else if (*arg == "-soundskip"
			         || *arg == "-sk")
			{
				arg++;
				if (arg != arguments.end())
				{
					try
					{
						soundSkipMethod = boost::lexical_cast<unsigned>(*arg);
					}
					catch( boost::bad_lexical_cast const& )
					{
						printUsage();
					}
					continue;
				}
				else
					printUsage();
			}

			else if (*arg == "-h"
			         || *arg == "-cycles")
			{
				arg++;
				if (arg != arguments.end())
				{
					try
					{
						cyclesPercentage = boost::lexical_cast<unsigned long>(*arg);
					}
					catch( boost::bad_lexical_cast const& )
					{
						printUsage();
					}
					if (cyclesPercentage > 200)
						cyclesPercentage = 200;
					continue;
				}
				else
					printUsage();
			}

			else if (*arg == "-nh"
			         || *arg == "-nohdma")
				disableHDMA = true;
			else if (*arg == "-ha"
			         || *arg == "-hdma")
				disableHDMA = false;

			else if (*arg == "-p"
			         || *arg ==  "-pal")
				PAL = true;
			else if (*arg == "-n"
			         || *arg == "-ntsc")
				PAL = false;

			else if (*arg == "-f"
			         || *arg == "-frameskip")
			{
				arg++;
				if (arg != arguments.end())
				{
					try
					{
						skipFrames = boost::lexical_cast<unsigned>(*arg) + 1;
					}
					catch( boost::bad_lexical_cast const& )
					{
						printUsage();
					}
					continue;
				}
				else
					printUsage();
			}

			else if (*arg == "-fh"
			         || *arg == "-hr"
			         || *arg == "-hirom")
				forceHiROM = true;
			else if (*arg == "-fl"
			         || *arg == "-lr"
			         || *arg == "-lorom")
				forceLoROM = true;

			else if (*arg == "-hd"
			         || *arg == "-header"
			         || *arg == "-he")
				forceHeader = true;

			else if (*arg =="-nhd"
			         || *arg == "-noheader")
				forceNoHeader = true;

			else if (*arg == "-bs")
				BS = true;

			else if (*arg == "-hi"
			         || *arg == "-hires")
				supportHiRes = true;

			else if (*arg == "-16"
			         || *arg == "-sixteen")
				sixteenBit = true;

			else if (*arg == "-displayframerate"
			         || *arg == "-dfr")
				displayFrameRate = true;

			else if (*arg == "-s"
			         || *arg == "-swapjoypads"
			         || *arg == "-sw")
				swapJoypads = true;

			else if (*arg == "-i"
			         || *arg == "-interleaved")
				forceInterleaved = true;
			else if (*arg == "-i2"
			         || *arg == "-interleaved2")
				forceInterleaved2 = true;
			else if (*arg == "-ni"
			         || *arg == "-nointerleave")
				forceNotInterleaved = true;

			else if (*arg == "-noirq")
				disableIRQ = true;

			else if (*arg == "-e"
			         || *arg == "-echo")
				disableSoundEcho = false;
			else if (*arg == "-ne"
			         || *arg == "-noecho")
				disableSoundEcho = true;
			else if (*arg == "-r" ||
			         *arg == "-soundquality" ||
			         *arg == "-sq")
			{
				arg++;
				if (arg != arguments.end())
				{
					try
					{
						soundPlaybackRate = boost::lexical_cast<unsigned>(*arg) & 7;
					}
					catch( boost::bad_lexical_cast const& )
					{
						printUsage();
					}
				}
				else
					printUsage();
			}

			else if (*arg == "-envx" ||
			         *arg == "-ex")
				soundEnvelopeHeightReading = true;
			else if (*arg == "-nosamplecaching" ||
			         *arg == "-nsc" ||
			         *arg == "-nc")
				disableSampleCaching = true;
			else if (*arg == "-nomastervolume" ||
			         *arg == "-nmv")
				disableMasterVolume = true;
			else if (*arg == "-soundsync" ||
			         *arg == "-sy")
			{
				soundSync = 1;
				soundEnvelopeHeightReading = true;
				interpolatedSound = true;
			}
			else if (*arg == "-soundsync2" ||
			         *arg == "-sy2")
			{
				soundSync = 2;
				soundEnvelopeHeightReading = true;
				interpolatedSound = true;
			}
			else if (*arg == "-interpolatedsound" ||
			         *arg == "-is")
				interpolatedSound = true;

			else if (*arg == "-alt" ||
			         *arg == "-altsampledecode")
				altSampleDecode = 1;
			else if (*arg == "-fix")
				fixFrequency = true;
			
			else if (*arg == "-l" ||
			         *arg == "-loadsnapshot")
			{
				arg++;
				if (arg != arguments.end())
					snapshotFilename = *arg;
				else
					printUsage();
			}

			else if (*arg == "-x2")
				vga = true;

			else if (*arg == "-xs")
			{
				try
				{
					resolutionX = boost::lexical_cast<unsigned>(*arg);
				}
				catch( boost::bad_lexical_cast const& )
				{
					printUsage();
				}
			}
			else if (*arg == "-ys")
			{
				try
				{
					resolutionY = boost::lexical_cast<unsigned>(*arg);
				}
				catch( boost::bad_lexical_cast const& )
				{
					printUsage();
				}
			}
			else if (*arg == "-cl")
			{
				try
				{
					cl = boost::lexical_cast<unsigned>(*arg);
				}
				catch( boost::bad_lexical_cast const& )
				{
					printUsage();
				}
			}
			else if (*arg == "-cs")
			{
				try
				{
					cs = boost::lexical_cast<unsigned>(*arg);
				}
				catch( boost::bad_lexical_cast const& )
				{
					printUsage();
				}
			}
			else if (*arg == "-mfs")
			{
				try
				{
					mfs = boost::lexical_cast<unsigned>(*arg);
				}
				catch( boost::bad_lexical_cast const& )
				{
					printUsage();
				}
			}
			else
				printUsage();

		}
		else
			romFilename = *arg;
	}

	return romFilename;
}


void Emulator::printUsage () throw(ExitException)
{
	std::cerr<<"Usage: snes9x <options> <rom image filename>"<<std::endl;
	throw ExitException();
}
