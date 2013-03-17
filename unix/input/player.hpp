#ifndef _PLAYER_HPP_
#define _PLAYER_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <SDL/SDL.h>

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

private:
	KeyboardMapping *kmap;
	boost::shared_ptr<PluggedJoystick> joystick;
};

#endif
