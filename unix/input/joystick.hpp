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

#ifndef _JOYSTICK_HPP_
#define _JOYSTICK_HPP_

#include <string>
#include <map>
#include <fstream>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <SDL2/SDL.h>

enum JOYSTICK_BUTTON {JB_A,
                      JB_B,
                      JB_X,
                      JB_Y,
                      JB_L,
                      JB_R,
                      JB_START,
                      JB_SELECT,
                      JB_ACCEL,
                      JB_QUIT,
                      JB_NB_BUTTONS};

enum JOYSTICK_AXIS {JA_LR,
                   JA_UD};

#define CENTER	0
#define LEFT	1
#define RIGHT	2
#define UP	1
#define DOWN	2

#define JOYSTICK_DEFAULT_NAME "Default"

class AvailableJoystick
{
public:
	AvailableJoystick(std::string name = JOYSTICK_DEFAULT_NAME);
	uint8_t& operator[](JOYSTICK_BUTTON i) {return mapping[i];}

	void setName(const std::string& n) {name = n;}
	std::string getName() const {return name;}

private:
	friend class boost::serialization::access;
	template<class archive>
	void serialize(archive& ar, const unsigned int version)
		{
			using boost::serialization::make_nvp;
			ar & make_nvp("name", name);
			ar & make_nvp("mapping", mapping);
		}

private:
	std::string name;
	std::map<JOYSTICK_BUTTON, uint8_t> mapping;
};


//Uncopiable because of SDL_Joystick:
class PluggedJoystick: boost::noncopyable 
{
public:
	PluggedJoystick(SDL_Joystick *sdlJoy,
	                AvailableJoystick *mapping);
	~PluggedJoystick() {if (sdlJoy) SDL_JoystickClose(sdlJoy);}

	bool& operator[](unsigned button) {return buttons[button];}
	int& operator[](JOYSTICK_AXIS axe) {return axes[axe];}
	bool& operator[](JOYSTICK_BUTTON button) {return buttons[(*mapping)[button]];}

	std::string getName() const {return mapping->getName();}

	int getSDLIndex() const {if(sdlJoy) return SDL_JoystickInstanceID(sdlJoy); else return -1;}

private:
	SDL_Joystick *sdlJoy;
	AvailableJoystick *mapping;
	std::map<unsigned, bool> buttons;
	std::map<unsigned, int> axes;
};


inline bool operator==(const AvailableJoystick &j, const std::string &name)
{
	return j.getName() == name;
}
inline bool operator==(boost::shared_ptr<AvailableJoystick> j, const std::string &name)
{
	return j->getName() == name;
}

#endif
