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

#include "inputController.hpp"
#include "cpuexec.h"
#include "snapshot.h"
#include "display.h"

//TODO remove globals!
#include "snes9x.h"
extern struct SSettings Settings;
#include "ppu.h"
extern struct SPPU PPU;



InputController::InputController() throw (SnesException):
	config(std::string(S9xGetSnapshotDirectory()) + "/" + INPUT_CONFIG_DEFAULT_FILE),
	threadProcess(NULL),
	keyboardState(SDL_GetKeyState(NULL))
{
	//Players initialisation
	//First we try joysticks. If no joystick are available, we try keyboard.
	//There must be at least one player.
	int numJoysticks(SDL_NumJoysticks());
	if(SDL_JoystickEventState(SDL_ENABLE) != SDL_ENABLE)
	{
		std::cerr<<"Could not set joystick event state "<<SDL_GetError()<<std::endl;
		numJoysticks = 0;
	}
	for (unsigned i = 0; i < numJoysticks; ++i)
	{
		SDL_Joystick *joy = SDL_JoystickOpen(i);
		if(joy)
		{
			//Check if keyboard is not detected as a joystick
			if (SDL_JoystickNumAxes(joy) > 6)
				SDL_JoystickClose(joy);
			else
				players.push_back(Player(NULL,
				                         joy,
				                         config.getJoystick(SDL_JoystickName(i))));
		}
	}
	//"Joystick" players ok let's add "keyboard" players.
	unsigned numKbd(config.getNbKbd());
	for (unsigned i = 1; i < numKbd; ++i)
	{
		players.push_back(Player(config.getKbdMap(i),
		                         NULL,
		                         NULL));
	}

	//If no player, use default keyboard map.
	if (players.empty())
		players.push_back(Player(config.getKbdMap(0),
		                         NULL,
		                         NULL));

	threadProcess = new boost::thread(boost::bind(&InputController::process, this));
	if (!threadProcess)
		throw ExitException();
}



InputController::~InputController()
{
	if (threadProcess)
		delete threadProcess;
}



boost::uint32_t InputController::getControllerState(unsigned player) throw (ExitException)
{
	boost::uint32_t state(0x80000000);

	if (player >= players.size())
		return state;

	boost::mutex::scoped_lock lock(mutex);

	state |= players[player].getControllerState(keyboardState);
	return state;
}


void InputController::checkGlobal() throw (ExitException)
{
	static bool turbo(false);
	static int OldSkipFrame(0);

	if (keyboardState[config.getGlobalKey(KEY_QUIT)] == SDL_PRESSED)
		throw ExitException();

	if (keyboardState[config.getGlobalKey(KEY_ACCEL)] != SDL_PRESSED)
	{
		if (turbo)
		{
			turbo = false;
			Settings.SkipFrames = OldSkipFrame;
		}
	}
	else
	{
		if (!turbo)
		{
			turbo = true;
			OldSkipFrame = Settings.SkipFrames;
			Settings.SkipFrames = 10;
		}
	}
}


void InputController::process(void)
{
	while (true)
	{
		uint8 jbtn = 0;
		uint32 num = 0;
		static bool8_32 TURBO = FALSE;
		SDL_Event event;
		Player* player;

		if (SDL_WaitEvent(&event))
		{
			boost::mutex::scoped_lock lock(mutex);
			switch(event.type)
			{
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				player = getPlayerByJoystick(event.jbutton.which);
				if (player)
					player->setButton(event.jbutton.button,
					                  event.type == SDL_JOYBUTTONDOWN ? true:false);
				break;
			case SDL_JOYAXISMOTION:
				player = getPlayerByJoystick(event.jaxis.which);
				if (player)
				{
					switch(event.jaxis.axis)
					{
					case JA_LR:
						if(event.jaxis.value == 0)
							player->setAxis(JA_LR, CENTER);
						else if(event.jaxis.value > 0)
							player->setAxis(JA_LR, RIGHT);
						else
							player->setAxis(JA_LR, LEFT);
						break;
					case JA_UD:
						if(event.jaxis.value == 0)
							player->setAxis(JA_UD, CENTER);
						else if(event.jaxis.value > 0)
							player->setAxis(JA_UD, DOWN);
						else
							player->setAxis(JA_UD, UP);
						break;
					}
				}
			case SDL_KEYDOWN:
				keyboardState = SDL_GetKeyState(NULL);

				if (event.key.keysym.sym == SDLK_0)
					Settings.DisplayFrameRate = !Settings.DisplayFrameRate;
				else if (event.key.keysym.sym == SDLK_1)	PPU.BG_Forced ^= 1;
				else if (event.key.keysym.sym == SDLK_2)	PPU.BG_Forced ^= 2;
				else if (event.key.keysym.sym == SDLK_3)	PPU.BG_Forced ^= 4;
				else if (event.key.keysym.sym == SDLK_4)	PPU.BG_Forced ^= 8;
				else if (event.key.keysym.sym == SDLK_5)	PPU.BG_Forced ^= 16;
				else if (event.key.keysym.sym == SDLK_6)	num = 1;
				else if (event.key.keysym.sym == SDLK_7)	num = 2;
				else if (event.key.keysym.sym == SDLK_8)	num = 3;
				else if (event.key.keysym.sym == SDLK_9)	num = 4;
				else if (event.key.keysym.sym == SDLK_r)
				{
					if (event.key.keysym.mod & KMOD_SHIFT)
						S9xReset();
				}
				if (num)
				{
					char fname[256], ext[8];
					sprintf(ext, ".00%d", num - 1);
					strcpy(fname, S9xGetFilename (ext));
					if (event.key.keysym.mod & KMOD_SHIFT)
						S9xFreezeGame (fname);
					else
						S9xLoadSnapshot (fname);
				}
				break;
			case SDL_KEYUP:
				keyboardState = SDL_GetKeyState(NULL);
			}
		}
	}
}


Player* InputController::getPlayerByJoystick(int joystickIndex)
{
	if (joystickIndex < 0)
		return NULL;
	else
	{
		std::vector<Player>::iterator it(std::find_if(players.begin(),
		                                              players.end(),
		                                              boost::bind(&Player::hasJoystick,
		                                                          _1, joystickIndex)));
		if (it == players.end())
			return NULL;
		else
			return &(*it);
	}
}

