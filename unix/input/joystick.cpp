#include <iostream>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include "joystick.hpp"

#define DEFAULT_BUTTON_NUMBER 32


AvailableJoystick::AvailableJoystick(std::string name):
	name(name)
{
	mapping[JB_A] = 0;
	mapping[JB_B] = 1;
	mapping[JB_X] = 2;
	mapping[JB_Y] = 3;
	mapping[JB_L] = 4;
	mapping[JB_R] = 5;
	mapping[JB_START] = 6;
	mapping[JB_SELECT] = 7;
	mapping[JB_ACCEL] = 8;
	mapping[JB_QUIT] = 9;
}



PluggedJoystick::PluggedJoystick(SDL_Joystick *sdlJoy,
                                 AvailableJoystick *mapping):
	sdlJoy(sdlJoy),
	mapping(mapping)
{
	axes[JA_LR] = false;
	axes[JA_UD] = false;

	for (int i = 0; i < DEFAULT_BUTTON_NUMBER; ++i)
		buttons[i] = false;
}

PluggedJoystick::PluggedJoystick(const PluggedJoystick& j):
	sdlJoy(j.sdlJoy),
	mapping(j.mapping),
	buttons(j.buttons),
	axes(j.axes)
{
}
