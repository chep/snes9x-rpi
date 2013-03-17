#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <boost/shared_ptr.hpp>

#include "inputConfig.hpp"

#include "joystick.hpp"
#include "snes9x.h"
#include "keyboard.hpp"

struct SDLInfos
{
	TTF_Font *font;
	SDL_Surface *screen;
};


const std::string conversion[] = {"A",
                                  "B",
                                  "X",
                                  "Y",
                                  "L",
                                  "R",
                                  "START",
                                  "SELECT",
                                  "ACCEL",
                                  "QUIT"};

const std::string conversionKbd[] = {"A",
                                     "B",
                                     "X",
                                     "Y",
                                     "L",
                                     "R",
                                     "START",
                                     "SELECT",
                                     "RIGHT",
                                     "LEFT",
                                     "UP",
                                     "DOWN"};

static bool initSDL(uint32_t xs, uint32_t ys, SDLInfos *infos);
static bool displayMessage(SDLInfos &infos, int x, int y, std::string msg);
static void clearScreen(SDL_Surface *screen);
static void configureGlobals(SDLInfos &infos, boost::shared_ptr<InputConfig> conf);
static void configureJoysticks(SDLInfos &infos, boost::shared_ptr<InputConfig> conf);
static void configureKeyboard(SDLInfos &infos,
                              boost::shared_ptr<InputConfig> conf,
                              unsigned nbKeyboards);

int main(int argc, char **argv)
{
	boost::shared_ptr<InputConfig> conf;
	SDLInfos sdlInfos;
	unsigned nbKeyboards(0);

	if (argc < 2)
	{
		std::cerr<<"Usage: "<<argv[0]<<" nbKeyboardPlayers"<<std::endl;
		std::cerr<<"Use 0 to disable keyboard."<<std::endl;
		return 1;
	}
	std::istringstream iss(argv[1]);
	iss>>nbKeyboards;

	try
	{
		conf = boost::shared_ptr<InputConfig>(new InputConfig(std::string("../")
		                                                      + INPUT_CONFIG_DEFAULT_FILE));
	}
	catch (...)
	{
		std::cerr<<"Exception occurs. Creating a new config."<<std::endl;
		conf = boost::shared_ptr<InputConfig>(new InputConfig(true));
	}
	//Keyboard is always reconfigured:
	conf->clearKbdMap();

	if (!initSDL(500, 300, &sdlInfos))
	{
		std::cerr<<"Unable to init SDL. Exiting"<<std::endl;
		return 1;
	}

	if (nbKeyboards)
		configureGlobals(sdlInfos, conf);
	configureJoysticks(sdlInfos, conf);
	configureKeyboard(sdlInfos, conf, nbKeyboards);

	try
	{
		conf->save(std::string("../") + INPUT_CONFIG_DEFAULT_FILE);
	}
	catch (SnesException e)
	{
		std::cerr<<e.getName()<<std::endl;
	}

	SDL_FreeSurface(sdlInfos.screen);
	TTF_CloseFont(sdlInfos.font);
	TTF_Quit();
	return 0;
}



bool initSDL(uint32_t xs, uint32_t ys, SDLInfos *infos)
{
	if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_VIDEO))
	{
		std::cerr<<"Could not initialize SDL: "<<SDL_GetError()<<std::endl;
		return false;
	}
	atexit(SDL_Quit);
	infos->screen = SDL_SetVideoMode(xs, ys, 16, SDL_SWSURFACE);
	if (infos->screen == NULL)
	{
		std::cerr<<"Couldn't set video mode: "<<SDL_GetError()<<std::endl;
		return false;
	}

	SDL_ShowCursor(0); // rPi: we're not really interested in showing a mouse cursor

	if( TTF_Init() == -1 )
		return false;

	infos->font = TTF_OpenFont("FreeMono.ttf", 12);
	if( infos->font == NULL ) 
		return false;

	return true;
}

bool displayMessage(SDLInfos &infos, int x, int y, std::string msg)
{
	SDL_Surface *message;
	SDL_Color textColor = {255, 255, 255};
	SDL_Rect dst;
	message = TTF_RenderText_Solid(infos.font, msg.c_str(), textColor); 
	if(message == NULL)
		return false;
	dst.x = x;
	dst.y = y;
	SDL_BlitSurface(message, NULL, infos.screen, &dst);

	if(SDL_Flip(infos.screen) == -1)
		return false;
	SDL_FreeSurface(message);

	return true;
}

static void clearScreen(SDL_Surface *screen)
{
		SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
}


static void configureGlobals(SDLInfos &infos, boost::shared_ptr<InputConfig> conf)
{
	bool ok(false);
	SDL_Event event;

	clearScreen(infos.screen);
	displayMessage(infos, 0, 0, "Global keys configuration");

	displayMessage(infos, 0, 15,
	               "Press ACCEL key.");
	while(!ok)
	{
		if (!SDL_WaitEvent(&event))
			std::cerr<<"SDL_WaitEvent error: "<<SDL_GetError()<<std::endl;
		switch(event.type)
		{
		case SDL_KEYDOWN:
			conf->setGlobalKey(KEY_ACCEL, event.key.keysym.sym);
			ok = true;
			break;
		default:
			break;
		}
	}
	displayMessage(infos, 0, 30,
	               "Press QUIT key.");
	ok = false;
	while(!ok)
	{
		if (!SDL_WaitEvent(&event))
			std::cerr<<"SDL_WaitEvent error: "<<SDL_GetError()<<std::endl;
		switch(event.type)
		{
		case SDL_KEYDOWN:
			conf->setGlobalKey(KEY_QUIT, event.key.keysym.sym);
			ok = true;
			break;
		default:
			break;
		}
	}
}


static void configureJoysticks(SDLInfos &infos, boost::shared_ptr<InputConfig> conf)
{
	int numJoysticks(0);

	numJoysticks = SDL_NumJoysticks();
	for (int i = 0; i < numJoysticks; ++i) 
	{
		SDL_Joystick *joy = SDL_JoystickOpen(i);
		if (joy)
		{
			AvailableJoystick *avJ(conf->getJoystick(SDL_JoystickName(i)));
			if (avJ->getName() == JOYSTICK_DEFAULT_NAME) //Not found in config
				avJ = conf->addJoystick(SDL_JoystickName(i));

			clearScreen(infos.screen);
			displayMessage(infos, 0, 0,
			               std::string("Configuration of: ") + SDL_JoystickName(i));

			for (int b = JB_A; b < JB_NB_BUTTONS; b++)
			{
				SDL_Event event;
				bool ok(false);
				displayMessage(infos, 0, b * 15 + 15,
				               "Press " + conversion[b] +" button, [Esc] to disable this button.");
				while(!ok)
				{
					if (!SDL_WaitEvent(&event))
						std::cerr<<"SDL_WaitEvent error: "<<SDL_GetError()<<std::endl;
					switch(event.type)
					{
					case SDL_JOYBUTTONDOWN:
						if (event.jbutton.which == i)
						{
							(*avJ)[(JOYSTICK_BUTTON)b] = event.jbutton.button;
							std::cout<<conversion[b]<<" is assigned to "<<(int)event.jbutton.button<<std::endl;
							ok = true;
						}
						break;
					case SDL_KEYDOWN:
						if (event.key.keysym.sym == SDLK_ESCAPE)
						{
							(*avJ)[(JOYSTICK_BUTTON)b] = 255;
							std::cout<<conversion[b]<<" is disabled "<<std::endl;
							ok = true;
						}
						break;
					default:
						break;
					}
				}
			}
		}
	}
}


static void configureKeyboard(SDLInfos &infos,
                              boost::shared_ptr<InputConfig> conf,
                              unsigned nbKeyboards)
{
	for (unsigned i = 0; i < nbKeyboards; i++)
	{
		KeyboardMapping *kmap(conf->addKbdMap());
		std::ostringstream oss("Keyboard configuration number ");
		oss<<i;
		oss<<": ";
		clearScreen(infos.screen);
		displayMessage(infos, 0, 0, oss.str());

		for (unsigned k = 0; k < KEY_NB_KEYS; ++k)
		{
			bool ok(false);
			SDL_Event event;
			displayMessage(infos, 0, k * 15 + 15,
			               "Press " + conversionKbd[k] +" key.");
			while(!ok)
			{
				if (!SDL_WaitEvent(&event))
					std::cerr<<"SDL_WaitEvent error: "<<SDL_GetError()<<std::endl;
				switch(event.type)
				{
				case SDL_KEYDOWN:
					kmap->setSDLKey((SNES_KEY)k, event.key.keysym.sym);
					ok = true;
					break;
				default:
					break;
				}
			}
		}
	}
}
