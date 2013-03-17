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
