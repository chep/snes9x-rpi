#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <fstream>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <boost/shared_ptr.hpp>

#include "joystick.hpp"
#include "snes9x.h"

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

static bool initSDL(uint32_t xs, uint32_t ys, SDLInfos *infos);
static bool initJoysticks(std::vector<boost::shared_ptr<AvailableJoystick> > &availableJoysticks,
                          std::vector<boost::shared_ptr<PluggedJoystick> > &pluggedJoysticks);
static bool displayMessage(SDLInfos *infos, int x, int y, std::string msg);

int main()
{
	std::vector<boost::shared_ptr<AvailableJoystick> > availableJoysticks;
	std::vector<boost::shared_ptr<PluggedJoystick> > pluggedJoysticks;

	std::ifstream file(JOYSTICK_BUTTONS_FILENAME);
	bool noError(true);
	SDLInfos sdlInfos;


	while (file.is_open() && !file.bad() && noError)
	{
		boost::shared_ptr<AvailableJoystick> j;
		try
		{
			j = AvailableJoystick::load(file);
			availableJoysticks.push_back(j);
		}
		catch (...)
		{
			noError = false;
		}
	}
	if(file.is_open())
		file.close();

	if (!initSDL(500, 300, &sdlInfos))
		return 1;

	if (!initJoysticks(availableJoysticks, pluggedJoysticks))
		return 1;

	for (int i = 0; i < pluggedJoysticks.size(); ++i)
	{
		std::string msg;
		SDL_FillRect(sdlInfos.screen, NULL, SDL_MapRGB(sdlInfos.screen->format, 0, 0, 0));
		displayMessage(&sdlInfos, 0, 0,
		               "Configuration of: " + pluggedJoysticks[i]->getName());

		for (int b = JB_A; b < JB_NB_BUTTONS; b++)
		{
			SDL_Event event;
			bool ok = false;
			displayMessage(&sdlInfos, 0, b * 15 + 15,
			               "Press " + conversion[b] +" button, [Esc] to disable this button.");
			while(!ok)
			{
				if (!SDL_WaitEvent(&event))
					std::cerr<<"SDL_WaitEvent error: "<<SDL_GetError()<<std::endl;
				switch(event.type)
				{
				case SDL_JOYBUTTONDOWN:
					if (event.jbutton.which == pluggedJoysticks[i]->getIndex())
					{
						pluggedJoysticks[i]->setMapping((JOYSTICK_BUTTON)b, event.jbutton.button);
						std::cout<<conversion[b]<<" is assigned to "<<(int)event.jbutton.button<<std::endl;
						ok = true;
					}
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE)
					{
						pluggedJoysticks[i]->setMapping((JOYSTICK_BUTTON)b, 255);
						std::cout<<conversion[b]<<" is disabled "<<std::endl;
						ok = true;
					}
					break;
				}
			}
		}
	}

	std::ofstream ofile(JOYSTICK_BUTTONS_FILENAME);
	std::for_each(availableJoysticks.begin(),
	              availableJoysticks.end(),
	              std::bind1st(std::ptr_fun(AvailableJoystick::save), ofile));
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



bool initJoysticks(std::vector<boost::shared_ptr<AvailableJoystick> > &availableJoysticks,
                   std::vector<boost::shared_ptr<PluggedJoystick> > &pluggedJoysticks)
{
	int numJoysticks(0);

	numJoysticks = SDL_NumJoysticks();
	for (int i = 0; i < numJoysticks && i < NB_MAX_CONTROLLERS; ++i) 
	{
		SDL_Joystick *joy = SDL_JoystickOpen(i);
		if(joy)
		{
			std::string joyName(SDL_JoystickName(i));
			std::cout<<"Opened joystick "<<joyName<<std::endl;
			if(SDL_JoystickEventState(SDL_ENABLE) != SDL_ENABLE)
			{
				std::cerr<<"Could not set joystick event state"<<SDL_GetError()<<std::endl;
				return false;
			}
			typedef std::vector<boost::shared_ptr<AvailableJoystick> >::iterator AJIt;
			AJIt it = std::find(availableJoysticks.begin(),
			                    availableJoysticks.end(),
			                    joyName);
			if (it == availableJoysticks.end())
			{
				boost::shared_ptr<AvailableJoystick> j(new AvailableJoystick(joyName));
				availableJoysticks.push_back(j);
				boost::shared_ptr<PluggedJoystick> pj(new PluggedJoystick(joy,
				                                                          i,
				                                                          availableJoysticks.back()));
				pluggedJoysticks.push_back(pj);
			}
			else
			{
				boost::shared_ptr<PluggedJoystick> pj(new PluggedJoystick(joy, i, *it));
				pluggedJoysticks.push_back(pj);
			}
		}
	}
	return true;
}


bool displayMessage(SDLInfos *infos, int x, int y, std::string msg)
{
	SDL_Surface *message;
	SDL_Color textColor = {255, 255, 255};
	SDL_Rect dst;
	message = TTF_RenderText_Solid(infos->font, msg.c_str(), textColor); 
	if(message == NULL)
		return false;
	dst.x = x;
	dst.y = y;
	SDL_BlitSurface(message, NULL, infos->screen, &dst);

	if(SDL_Flip(infos->screen) == -1)
		return false;
	SDL_FreeSurface(message);

	return true;
}
