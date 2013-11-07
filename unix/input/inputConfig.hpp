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

#ifndef _INPUTCONFIG_HPP_
#define _INPUTCONFIG_HPP_

#include <string>
#include <vector>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/nvp.hpp>

#include "keyboard.hpp"
#include "joystick.hpp"
#include "exceptions.hpp"

#define INPUT_CONFIG_DEFAULT_FILE "input.config"

class InputConfig
{
public:
	InputConfig(const std::string &file = INPUT_CONFIG_DEFAULT_FILE) throw (SnesBadConfigFileException);
	InputConfig(bool unused); //create only default keymap and joystick

public:
	void save(const std::string &file = INPUT_CONFIG_DEFAULT_FILE) throw (SnesException);

	bool verify(void) {return !(kbdMaps.empty() || joysticks.empty());} //Config is invalid if no keyboard default map

	SDL_Scancode getGlobalKey(SNES_COMMON_KEY k) {return globalMap[k];}
	void setGlobalKey(SNES_COMMON_KEY k, SDL_Scancode sk) {globalMap[k] = sk;}

	AvailableJoystick* getJoystick(std::string name);
	AvailableJoystick* addJoystick(std::string name);

	unsigned getNbKbd(void) const {return kbdMaps.size();}
	KeyboardMapping* getKbdMap(unsigned index) throw (SnesException);
	KeyboardMapping* addKbdMap(void);
	void clearKbdMap() {while (kbdMaps.size() > 1) kbdMaps.pop_back();}

private:
	friend class boost::serialization::access;
	template<class archive>
	void serialize(archive& ar, const unsigned int version)
		{
			using boost::serialization::make_nvp;
			ar & make_nvp("globalMap", globalMap);
			ar & make_nvp("kbdMaps", kbdMaps);
			ar & make_nvp("joysticks", joysticks);
		}

private:
	GlobalKeyboardMapping globalMap;
	std::vector<KeyboardMapping> kbdMaps; //First element is default mapping
	std::vector<AvailableJoystick> joysticks; //First element is default mapping
};

#endif
