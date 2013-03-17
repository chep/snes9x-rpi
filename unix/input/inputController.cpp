#include <iostream>

#include "inputController.hpp"
#include "cpuexec.h"
#include "snapshot.h"
#include "display.h"

//TODO remove globals!
#include "snes9x.h"
extern struct SSettings Settings;
#include "ppu.h"
extern struct SPPU PPU;



InputController::InputController() throw (ExitException):
	threadProcess(NULL),
	keyboardState(SDL_GetKeyState(NULL))
{
	//Players initialisation
	//First we try joysticks. If no joystick are available, we try keyboard.
	//There must be at least one player.
	int numJoysticks(SDL_NumJoysticks());
	if(SDL_JoystickEventState(SDL_ENABLE) != SDL_ENABLE)
	{
		std::cerr<<"Could not set joystick event state "<<SDL_GetError()<<std::endl;
		numJoysticks = 0;
	}
	for (unsigned i = 0; i < numJoysticks; ++i)
	{
		SDL_Joystick *joy = SDL_JoystickOpen(i);
		if(joy)
			players.push_back(Player(NULL,
			                         joy,
			                         config.getJoystick(SDL_JoystickName(i))));
	}
	//"Joystick" players ok let's add "keyboard" players.
	unsigned numKbd(config.getNbKbd());
	for (unsigned i = 1; i < numKbd; ++i)
	{
		players.push_back(Player(config.getKbdMap(i),
		                         NULL,
		                         NULL));
	}

	//If no player, use default keyboard map.
	if (players.empty())
		players.push_back(Player(config.getKbdMap(0),
		                         NULL,
		                         NULL));

	threadProcess = new boost::thread(boost::bind(&InputController::process, this));
	if (!threadProcess)
		throw ExitException();
}



InputController::~InputController()
{
	if (threadProcess)
		delete threadProcess;
}



boost::uint32_t InputController::getControllerState(unsigned player) throw (ExitException)
{
	boost::uint32_t state(0x80000000);

	if (player >= players.size())
		return state;

	boost::mutex::scoped_lock lock(mutex);

	state |= players[player].getControllerState(keyboardState);
	return state;
}


void InputController::checkGlobal() throw (ExitException)
{
	static bool turbo(false);
	static int OldSkipFrame(0);

	if (keyboardState[config.getGlobalKey(KEY_QUIT)] == SDL_PRESSED)
		throw ExitException();

	if (keyboardState[config.getGlobalKey(KEY_ACCEL)] != SDL_PRESSED)
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


void InputController::process(void)
{
	while (true)
	{
		uint8 jbtn = 0;
		uint32 num = 0;
		static bool8_32 TURBO = FALSE;
		SDL_Event event;

		if (SDL_WaitEvent(&event))
		{
			boost::mutex::scoped_lock lock(mutex);
			switch(event.type)
			{
			case SDL_JOYBUTTONDOWN: //event.jbutton.which = player index
				if (event.jbutton.which < players.size())
					players[event.jbutton.which].setButton(event.jbutton.button, true);
				break;
			case SDL_JOYBUTTONUP:
				if (event.jbutton.which < players.size())
					players[event.jbutton.which].setButton(event.jbutton.button, false);
				break;
			case SDL_JOYAXISMOTION:
				if (event.jaxis.which < players.size())
				{
					switch(event.jaxis.axis)
					{
					case JA_LR:
						if(event.jaxis.value == 0)
							players[event.jaxis.which].setAxis(JA_LR, CENTER);
						else if(event.jaxis.value > 0)
							players[event.jaxis.which].setAxis(JA_LR, RIGHT);
						else
							players[event.jaxis.which].setAxis(JA_LR, LEFT);
						break;
					case JA_UD:
						if(event.jaxis.value == 0)
							players[event.jaxis.which].setAxis(JA_UD, CENTER);
						else if(event.jaxis.value > 0)
							players[event.jaxis.which].setAxis(JA_UD, DOWN);
						else
							players[event.jaxis.which].setAxis(JA_UD, UP);
						break;
					}
				}
			case SDL_KEYDOWN:
				keyboardState = SDL_GetKeyState(NULL);

				if (event.key.keysym.sym == SDLK_0)
					Settings.DisplayFrameRate = !Settings.DisplayFrameRate;
				else if (event.key.keysym.sym == SDLK_1)	PPU.BG_Forced ^= 1;
				else if (event.key.keysym.sym == SDLK_2)	PPU.BG_Forced ^= 2;
				else if (event.key.keysym.sym == SDLK_3)	PPU.BG_Forced ^= 4;
				else if (event.key.keysym.sym == SDLK_4)	PPU.BG_Forced ^= 8;
				else if (event.key.keysym.sym == SDLK_5)	PPU.BG_Forced ^= 16;
				else if (event.key.keysym.sym == SDLK_6)	num = 1;
				else if (event.key.keysym.sym == SDLK_7)	num = 2;
				else if (event.key.keysym.sym == SDLK_8)	num = 3;
				else if (event.key.keysym.sym == SDLK_9)	num = 4;
				else if (event.key.keysym.sym == SDLK_r)
				{
					if (event.key.keysym.mod & KMOD_SHIFT)
						S9xReset();
				}
				if (num)
				{
					char fname[256], ext[8];
					sprintf(ext, ".00%d", num - 1);
					strcpy(fname, S9xGetFilename (ext));
					if (event.key.keysym.mod & KMOD_SHIFT)
						S9xFreezeGame (fname);
					else
						S9xLoadSnapshot (fname);
				}
				break;
			case SDL_KEYUP:
				keyboardState = SDL_GetKeyState(NULL);
			}
		}
	}
}
