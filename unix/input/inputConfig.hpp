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
	InputConfig(const std::string &file = INPUT_CONFIG_DEFAULT_FILE) throw (ExitException);
	InputConfig(bool unused); //create only default keymap and joystick

public:
	void save(const std::string &file = INPUT_CONFIG_DEFAULT_FILE) throw (SnesException);

	bool verify(void) {return !(kbdMaps.empty() || joysticks.empty());} //Config is invalid if no keyboard default map

	SDLKey getGlobalKey(SNES_COMMON_KEY k) {return globalMap[k];}
	void setGlobalKey(SNES_COMMON_KEY k, SDLKey sk) {globalMap[k] = sk;}

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
