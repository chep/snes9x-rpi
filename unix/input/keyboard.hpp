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

#ifndef _KEYBOARD_HPP_
#define _KEYBOARD_HPP_

#include <map>
#include <SDL2/SDL.h> //For SDLKey type
#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>

#include "exceptions.hpp"

enum SNES_KEY {KEY_A,
               KEY_B,
               KEY_X,
               KEY_Y,
               KEY_L,
               KEY_R,
               KEY_START,
               KEY_SELECT,
               KEY_RIGHT,
               KEY_LEFT,
               KEY_UP,
               KEY_DOWN,
               KEY_NB_KEYS};

enum SNES_COMMON_KEY {KEY_ACCEL,
                      KEY_QUIT};


//This is global key map for global keys like QUIT or ACCEL
class GlobalKeyboardMapping
{
public:
	GlobalKeyboardMapping() {}
	SDL_Scancode& operator[](SNES_COMMON_KEY k) {return mapping[k];}

private:
	friend class boost::serialization::access;
	template<class archive>
	void serialize(archive& ar, const unsigned int version)
		{
			using boost::serialization::make_nvp;
			ar & make_nvp("mapping", mapping);
		}

private:
	std::map<SNES_COMMON_KEY, SDL_Scancode> mapping;
};


//This is the keyboard mapping for a specific player
class KeyboardMapping
{
public:
	KeyboardMapping() {}

public:
	SDL_Scancode getSDLKey(SNES_KEY k) {return mapping[k];}
	void setSDLKey(SNES_KEY k, SDL_Scancode sk) {mapping[k] = sk;}


private:
	friend class boost::serialization::access;
	template<class archive>
	void serialize(archive& ar, const unsigned int version)
		{
			using boost::serialization::make_nvp;
			ar & make_nvp("mapping", mapping);
		}

private:
	std::map<SNES_KEY, SDL_Scancode> mapping;
};

#endif
