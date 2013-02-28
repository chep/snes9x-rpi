#ifndef _JOYSTICK_HPP_
#define _JOYSTICK_HPP_

#include <string>
#include <map>
#include <fstream>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/shared_ptr.hpp>
#include <SDL/SDL.h>

#define JOYSTICK_BUTTONS_FILENAME "joysticks.buttons"

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

enum JOYSTICK_AXE {JA_LR,
                   JA_UD};

#define CENTER	0
#define LEFT	1
#define RIGHT	2
#define UP	1
#define DOWN	2

class AvailableJoystick
{
public:
	AvailableJoystick(std::string name="Default");
	uint8_t& operator[](JOYSTICK_BUTTON i) {return mapping[i];}

	static boost::shared_ptr<AvailableJoystick> load(std::ifstream &file);
	static void save(std::ofstream &file, boost::shared_ptr<AvailableJoystick> j);

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


class PluggedJoystick
{
public:
	PluggedJoystick(SDL_Joystick *sdlJoy, int index,
	                boost::shared_ptr<AvailableJoystick> mapping);
	PluggedJoystick(const PluggedJoystick& j);
	~PluggedJoystick() {SDL_JoystickClose(sdlJoy);}

	bool& operator[](unsigned button) {return buttons[button];}
	int& operator[](JOYSTICK_AXE axe) {return axes[axe];}

	bool& operator[](JOYSTICK_BUTTON button);

	int getIndex() const {return index;}
	std::string getName() const {return mapping->getName();}
	void setMapping(JOYSTICK_BUTTON b, uint8_t number) {(*mapping)[b] = number;}

private:
	SDL_Joystick *sdlJoy;
	int index;
	boost::shared_ptr<AvailableJoystick> mapping;
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
