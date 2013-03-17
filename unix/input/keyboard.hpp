#ifndef _KEYBOARD_HPP_
#define _KEYBOARD_HPP_

#include <map>
#include <SDL/SDL.h> //For SDLKey type
#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>

#include "exceptions.hpp"

enum SNES_KEY {KEY_A,
               KEY_B,
               KEY_X,
               KEY_Y,
               KEY_L,
               KEY_R,
               KEY_START,
               KEY_SELECT,
               KEY_RIGHT,
               KEY_LEFT,
               KEY_UP,
               KEY_DOWN,
               KEY_NB_KEYS};

enum SNES_COMMON_KEY {KEY_ACCEL,
                      KEY_QUIT};


//This is global key map for global keys like QUIT or ACCEL
class GlobalKeyboardMapping
{
public:
	GlobalKeyboardMapping() {}
	SDLKey& operator[](SNES_COMMON_KEY k) {return mapping[k];}

private:
	friend class boost::serialization::access;
	template<class archive>
	void serialize(archive& ar, const unsigned int version)
		{
			using boost::serialization::make_nvp;
			ar & make_nvp("mapping", mapping);
		}
	
private:
	std::map<SNES_COMMON_KEY, SDLKey> mapping;
};


//This is the keyboard mapping for a specific player
class KeyboardMapping
{
public:
	KeyboardMapping() {}

public:
	SDLKey getSDLKey(SNES_KEY k) {return mapping[k];}
	void setSDLKey(SNES_KEY k, SDLKey sk) {mapping[k] = sk;}

	
private:
	friend class boost::serialization::access;
	template<class archive>
	void serialize(archive& ar, const unsigned int version)
		{
			using boost::serialization::make_nvp;
			ar & make_nvp("mapping", mapping);
		}

private:
	std::map<SNES_KEY, SDLKey> mapping;
};

#endif
