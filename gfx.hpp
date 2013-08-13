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


#ifndef _GFX_HPP_
#define _GFX_HPP_

#include <vector>
#include <boost/cstdint.hpp>

class GFX
{
public:
	GFX();
	virtual ~GFX();

public:
	void initGraphics (bool sixteenBit);

	/* Get and set */
	void setScreen (boost::uint8_t *s) {screen = s;}
	void setPitch(boost::uint32_t p) {pitch = p;}

private:
	boost::uint8_t *screen;
	std::vector<boost::uint8_t> subScreen;
	std::vector<boost::uint8_t> zBuffer;
	std::vector<boost::uint8_t> subZBuffer;
	boost::uint32_t pitch;

	// Setup in call to S9xGraphicsInit()
	int   delta;
	boost::uint16_t *X2;
	boost::uint16_t *ZERO_OR_X2;
	boost::uint16_t *ZERO;
	boost::uint32_t realPitch; /**< True pitch of Screen buffer.*/
	boost::uint32_t pitch2;    /**< Same as RealPitch except while using speed up hack for Glide.*/
	boost::uint32_t zPitch;    /**< Pitch of ZBuffer */
	boost::uint32_t PPL;       /**< Number of pixels on each of Screen buffer */
	boost::uint32_t PPLx2;
	boost::uint32_t pixSize;
	boost::uint8_t  *S;
	boost::uint8_t  *DB;
	boost::uint16_t *screenColors;
	boost::uint32_t depthDelta;
	boost::uint32_t z1;
	boost::uint32_t z2;
	boost::uint32_t fixedColour;
	const char *infoString;
	boost::uint32_t infoStringTimeout;
	boost::uint32_t startY;
	boost::uint32_t endY;
	struct ClipData *pCurrentClip;
	boost::uint32_t mode7Mask;
	boost::uint32_t mode7PriorityMask;
	int	   OBJList[129];
	boost::uint32_t sizes[129];
	int    vPositions[129];

	boost::uint8_t	r212c;
	boost::uint8_t	r212d;
	boost::uint8_t	r2130;
	boost::uint8_t	r2131;
	bool pseudo;
};

#endif
