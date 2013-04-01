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

#include "player.hpp"

#include "snes9x.h"
extern struct SSettings Settings;


Player::Player(KeyboardMapping *kmap,
               SDL_Joystick *joy,
               AvailableJoystick *avblJ) throw (SnesException):
	kmap(kmap),
	joystick(new PluggedJoystick(joy, avblJ))
{
	if (!joystick)
		throw SnesException("Out of memory");
}


boost::uint32_t Player::getControllerState(const Uint8 *keyboardState) const throw (ExitException)
{
	boost::uint32_t state(0);

	if (kmap)
	{
		if (keyboardState[kmap->getSDLKey(KEY_L)] == SDL_PRESSED) state |= SNES_TL_MASK;
		if (keyboardState[kmap->getSDLKey(KEY_R)] == SDL_PRESSED) state |= SNES_TR_MASK;

		if (keyboardState[kmap->getSDLKey(KEY_X)] == SDL_PRESSED) state |= SNES_X_MASK;
		if (keyboardState[kmap->getSDLKey(KEY_Y)] == SDL_PRESSED) state |= SNES_Y_MASK;
		if (keyboardState[kmap->getSDLKey(KEY_B)] == SDL_PRESSED) state |= SNES_B_MASK;
		if (keyboardState[kmap->getSDLKey(KEY_A)] == SDL_PRESSED) state |= SNES_A_MASK;

		if (keyboardState[kmap->getSDLKey(KEY_START)] == SDL_PRESSED)  state |= SNES_START_MASK;
		if (keyboardState[kmap->getSDLKey(KEY_SELECT)] == SDL_PRESSED) state |= SNES_SELECT_MASK;

		if (keyboardState[kmap->getSDLKey(KEY_UP)] == SDL_PRESSED)    state |= SNES_UP_MASK;
		if (keyboardState[kmap->getSDLKey(KEY_DOWN)] == SDL_PRESSED)  state |= SNES_DOWN_MASK;
		if (keyboardState[kmap->getSDLKey(KEY_LEFT)] == SDL_PRESSED)  state |= SNES_LEFT_MASK;
		if (keyboardState[kmap->getSDLKey(KEY_RIGHT)] == SDL_PRESSED) state |= SNES_RIGHT_MASK;
	}
	else
	{
		PluggedJoystick &j = *joystick;
		static bool turbo(false);
		static int OldSkipFrame(0);

		if (j[JB_L]) state |= SNES_TL_MASK;
		if (j[JB_R]) state |= SNES_TR_MASK;

		if (j[JB_X]) state |= SNES_X_MASK;
		if (j[JB_Y]) state |= SNES_Y_MASK;
		if (j[JB_B]) state |= SNES_B_MASK;
		if (j[JB_A]) state |= SNES_A_MASK;

		if (j[JB_START]) state |= SNES_START_MASK;
		if (j[JB_SELECT]) state |= SNES_SELECT_MASK;

		if (j[JA_LR] == LEFT)  state |= SNES_LEFT_MASK;
		if (j[JA_LR] == RIGHT) state |= SNES_RIGHT_MASK;
		if (j[JA_UD] == UP)    state |= SNES_UP_MASK;
		if (j[JA_UD] == DOWN)  state |= SNES_DOWN_MASK;

		if (j[JB_QUIT])
			throw ExitException();

		if (!j[JB_ACCEL])
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

	return state;
}


bool Player::hasJoystick(int joystickIndex)
{
	if (joystick)
		return joystick->getSDLIndex() == joystickIndex;
	else
		return false;
}

