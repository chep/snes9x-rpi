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

#ifndef _INPUTCONTROLLER_HPP_
#define _INPUTCONTROLLER_HPP_

#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <SDL2/SDL.h>

#include "inputConfig.hpp"
#include "exceptions.hpp"
#include "player.hpp"

class InputController
{
public:
	InputController() throw (SnesException);
	virtual ~InputController();

public:
	boost::uint32_t getControllerState(unsigned player) throw (ExitException);
	void checkGlobal(void) throw (ExitException);
	void process(void);
	Player* getPlayerByJoystick(int joystickIndex);

private:
	InputConfig config;
	const Uint8 *keyboardState;

	std::vector<Player> players;
};
#endif
