#include <iostream>
#include <fstream>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include "inputConfig.hpp"


InputConfig::InputConfig(const std::string &file) throw (ExitException)
{
	std::ifstream f(file.c_str());
	if (!f.is_open() || f.bad())
		throw ExitException();

	try
	{
		boost::archive::xml_iarchive xml(f);
		xml >> boost::serialization::make_nvp("InputConfig", *this);
	}
	catch (...)
	{
		throw ExitException();
	}

	if (!verify())
		throw ExitException();
}


InputConfig::InputConfig(bool unused)
{
	(void) unused;

	globalMap[KEY_ACCEL] = SDLK_BACKSPACE;
	globalMap[KEY_QUIT] = SDLK_ESCAPE;

	KeyboardMapping m;
	m.setSDLKey(KEY_A, SDLK_z);
	m.setSDLKey(KEY_B, SDLK_x);
	m.setSDLKey(KEY_X, SDLK_a);
	m.setSDLKey(KEY_Y, SDLK_s);
	m.setSDLKey(KEY_L, SDLK_q);
	m.setSDLKey(KEY_R, SDLK_w);
	m.setSDLKey(KEY_START, SDLK_RETURN);
	m.setSDLKey(KEY_SELECT, SDLK_LCTRL);
	m.setSDLKey(KEY_RIGHT, SDLK_RIGHT);
	m.setSDLKey(KEY_LEFT, SDLK_LEFT);
	m.setSDLKey(KEY_UP, SDLK_UP);
	m.setSDLKey(KEY_DOWN, SDLK_DOWN);
	kbdMaps.push_back(m);

	AvailableJoystick j;
	j[JB_A] = 0;
	j[JB_B] = 1;
	j[JB_X] = 2;
	j[JB_Y] = 3;
	j[JB_L] = 4;
	j[JB_R] = 5;
	j[JB_START] = 6;
	j[JB_SELECT] = 7;
	j[JB_ACCEL] = 8;
	j[JB_QUIT] = 9;

	joysticks.push_back(j);
}


void InputConfig::save(const std::string &file) throw (SnesException)
{
	std::ofstream f(file.c_str());
	if (!f.is_open() || f.bad())
		throw SnesException("Unable to open file for writting");

	try
	{
		boost::archive::xml_oarchive xml(f);
		xml << boost::serialization::make_nvp("InputConfig", *this);
	}
	catch (...)
	{
		throw SnesException("Unable to save config");
	}
}


KeyboardMapping* InputConfig::getKbdMap(unsigned index) throw (SnesException)
{
	if (index >= kbdMaps.size())
		throw SnesException("Invalid index");

	return &(kbdMaps[index]);
}

AvailableJoystick* InputConfig::getJoystick(std::string name)
{
	std::vector<AvailableJoystick>::iterator it;
	it = std::find(joysticks.begin(), joysticks.end(), name);
	if (it == joysticks.end())
	{
		std::cerr<<"Could not find joystick mapping for"<<name;
		std::cerr<<", use default"<<std::endl;
		return &(joysticks.front());
	}

	std::cout<<"Mapping for "<<name<<" found!"<<std::endl;
	return &(*it);
}

AvailableJoystick* InputConfig::addJoystick(std::string name)
{
	joysticks.push_back(AvailableJoystick(name));
	return &(joysticks.back());
}


KeyboardMapping* InputConfig::addKbdMap(void)
{
	kbdMaps.push_back(KeyboardMapping());
	return &(kbdMaps.back());
}
