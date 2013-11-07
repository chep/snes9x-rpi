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

#ifndef _PLAYER_HPP_
#define _PLAYER_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <SDL2/SDL.h>

#include "keyboard.hpp"
#include "joystick.hpp"

#define SNES_TR_MASK	    (1 << 4)
#define SNES_TL_MASK	    (1 << 5)
#define SNES_X_MASK	    (1 << 6)
#define SNES_A_MASK	    (1 << 7)
#define SNES_RIGHT_MASK	    (1 << 8)
#define SNES_LEFT_MASK	    (1 << 9)
#define SNES_DOWN_MASK	    (1 << 10)
#define SNES_UP_MASK	    (1 << 11)
#define SNES_START_MASK	    (1 << 12)
#define SNES_SELECT_MASK    (1 << 13)
#define SNES_Y_MASK	    (1 << 14)
#define SNES_B_MASK	    (1 << 15)

class Player
{
public:
	Player(KeyboardMapping *kmap,
	       SDL_Joystick *joy,
	       AvailableJoystick *avblJ) throw (SnesException);

public:
	boost::uint32_t getControllerState(const Uint8 *keyboardState) const throw (ExitException);
	void setButton(unsigned button, bool state) {(*joystick)[button] = state;}
	void setAxis(JOYSTICK_AXIS axis, int state) {(*joystick)[axis] = state;}

	bool hasJoystick(int joystickIndex);

private:
	KeyboardMapping *kmap;
	boost::shared_ptr<PluggedJoystick> joystick;
};

#endif
