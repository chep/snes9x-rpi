#ifndef _KEYBOARD_HPP_
#define _KEYBOARD_HPP_

#include <vector>
#include <map>
#include <SDL/SDL.h> //For SDLKey type
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
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

enum SNES_COMMON_KEYS {KEY_ACCEL,
                       KEY_QUIT};

#define KEYBOARD_MAPPING_DEFAULT_FILE "keyboard.mapping"
#define MAX_KEYBOARD_PLAYERS 2

class KeyboardMapping
{
public:
	KeyboardMapping(std::string file = KEYBOARD_MAPPING_DEFAULT_FILE);

	std::map<SNES_KEY, SDLKey>& operator[](unsigned player) {return players[player];}
	SDLKey& operator[](SNES_COMMON_KEYS key) {return common[key];}

public:
	void save(std::string file = KEYBOARD_MAPPING_DEFAULT_FILE) throw (SnesException);

private:
	friend class boost::serialization::access;
	template<class archive>
	void serialize(archive& ar, const unsigned int version)
		{
			using boost::serialization::make_nvp;
			ar & make_nvp("players", players);
			ar & make_nvp("common", common);
		}

private:
	std::vector<std::map<SNES_KEY, SDLKey> > players;
	std::map<SNES_COMMON_KEYS, SDLKey> common;
};

#endif
