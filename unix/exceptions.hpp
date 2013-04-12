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

#ifndef _EXCEPTIONS_HPP_
#define _EXCEPTIONS_HPP_

#include <string>
#include <alsa/asoundlib.h>

class SnesException
{
public:
	SnesException(const std::string name): name(name) {}
	virtual ~SnesException() {}

public:
	std::string getName() const {return name;}

private:
	std::string name;
};

inline std::ostream& operator<< (std::ostream& os, const SnesException &e)
{
	os << e.getName();
	return os;
}

class ExitException: public SnesException
{
public:
	ExitException() : SnesException("Exit") {}
	virtual ~ExitException() {}

private:
};

class SnesBadConfigFileException: public SnesException
{
public:
	SnesBadConfigFileException() : SnesException("Bad Config file") {}
	virtual ~SnesBadConfigFileException() {}

private:
};


class AlsaFreeParamsException: public SnesException
{
public:
	AlsaFreeParamsException(snd_pcm_hw_params_t *hw_params, const std::string name) : SnesException(name)
		{
			snd_pcm_hw_params_free (hw_params);
		}
	virtual ~AlsaFreeParamsException() {}

private:
};

#endif
