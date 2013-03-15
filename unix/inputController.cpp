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
	loadJoystickMapping();
	//If there is not enough plugged joysticks, we add default fake joysticks
	for (unsigned i = pluggedJoysticks.size(); i < MAX_KEYBOARD_PLAYERS; i++)
	{
		boost::shared_ptr<PluggedJoystick> pj(new PluggedJoystick(NULL,
		                                                          i,
		                                                          *availableJoysticks.begin()));
		pluggedJoysticks.push_back(pj);
	}

	initSDLJoysticks();

	threadProcess = new boost::thread(boost::bind(&InputController::process, this));
	if (!threadProcess)
		throw ExitException();
}



InputController::~InputController()
{
	if (threadProcess)
		delete threadProcess;
}


void InputController::loadJoystickMapping(void)
{
	/* default mapping: */
	boost::shared_ptr<AvailableJoystick> temp(new AvailableJoystick());
	availableJoysticks.push_back(temp);

	/* Load from file */
	std::ifstream file(JOYSTICK_BUTTONS_FILENAME);
	bool noError(true);
	while (file.is_open() && !file.bad() && noError)
	{
		try
		{
			boost::shared_ptr<AvailableJoystick> j(AvailableJoystick::load(file));
			availableJoysticks.push_back(j);
		}
		catch (...)
		{
			noError = false;
		}
	}
}


void InputController::initSDLJoysticks()
{
	int numJoysticks(0);
	numJoysticks = SDL_NumJoysticks();
	for (int i = 0; i < numJoysticks && i < NB_MAX_CONTROLLERS; ++i)
	{
		SDL_Joystick *joy = SDL_JoystickOpen(i);
		if(joy)
		{
			std::cout<<"Opened joystick "<<i<<std::endl;
			if(SDL_JoystickEventState(SDL_ENABLE) != SDL_ENABLE)
			{
				std::cerr<<"Could not set joystick event state "<<SDL_GetError()<<std::endl;
				throw -1;
			}
			/* mapping */
			std::string joyName(SDL_JoystickName(i));
			typedef std::vector<boost::shared_ptr<AvailableJoystick> >::iterator AJIt;
			AJIt it = std::find(availableJoysticks.begin(),
			                    availableJoysticks.end(),
			                    joyName);
			if (it == availableJoysticks.end())
			{
				std::cerr<<"Could not find joystick mapping for"<<joyName;
				std::cerr<<"Use default"<<std::endl;
				boost::shared_ptr<PluggedJoystick> pj(new PluggedJoystick(joy,
				                                                          i,
				                                                          *availableJoysticks.begin()));
				pluggedJoysticks.push_back(pj);
			}
			else
			{
				std::cout<<"Mapping for "<<SDL_JoystickName(i)<<" found!"<<std::endl;
				boost::shared_ptr<PluggedJoystick> pj(new PluggedJoystick(joy, i, *it));
				pluggedJoysticks.push_back(pj);
			}
		}
	}
}


boost::uint32_t InputController::getJoystickState(int index) throw (ExitException)
{
	boost::uint32_t state(0x80000000);
	static bool turbo(false);
	static int OldSkipFrame(0);
		
	if (index < 0 || (index >= pluggedJoysticks.size() && index >= MAX_KEYBOARD_PLAYERS))
		return state;

	boost::mutex::scoped_lock lock(mutex);
	PluggedJoystick &j(*pluggedJoysticks[index]);

	if (keyboardState[keyboardMapping[index][KEY_L]] == SDL_PRESSED)
		state |= SNES_TL_MASK;
	if (keyboardState[keyboardMapping[index][KEY_R]] == SDL_PRESSED || j[JB_R])
		state |= SNES_TR_MASK;

	if (keyboardState[keyboardMapping[index][KEY_X]] == SDL_PRESSED || j[JB_X])
		state |= SNES_X_MASK;
	if (keyboardState[keyboardMapping[index][KEY_Y]] == SDL_PRESSED || j[JB_Y])
		state |= SNES_Y_MASK;
	if (keyboardState[keyboardMapping[index][KEY_B]] == SDL_PRESSED || j[JB_B])
		state |= SNES_B_MASK;
	if (keyboardState[keyboardMapping[index][KEY_A]] == SDL_PRESSED || j[JB_A])
		state |= SNES_A_MASK;
	if (keyboardState[keyboardMapping[index][KEY_START]] == SDL_PRESSED || j[JB_START])
		state |= SNES_START_MASK;
	if (keyboardState[keyboardMapping[index][KEY_SELECT]] == SDL_PRESSED || j[JB_SELECT])
		state |= SNES_SELECT_MASK;

	if (keyboardState[keyboardMapping[index][KEY_UP]] == SDL_PRESSED || j[JA_UD] == UP)
		state |= SNES_UP_MASK;
	if (keyboardState[keyboardMapping[index][KEY_DOWN]] == SDL_PRESSED || j[JA_UD] == DOWN)
		state |= SNES_DOWN_MASK;
	if (keyboardState[keyboardMapping[index][KEY_LEFT]] == SDL_PRESSED || j[JA_LR] == LEFT)
		state |= SNES_LEFT_MASK;
	if (keyboardState[keyboardMapping[index][KEY_RIGHT]] == SDL_PRESSED || j[JA_LR] == RIGHT)
		state |= SNES_RIGHT_MASK;

	if (j[JB_QUIT] || keyboardState[keyboardMapping[KEY_QUIT]])
		throw ExitException();

	if (!j[JB_ACCEL] && !keyboardState[keyboardMapping[KEY_ACCEL]])
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

	return state;
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
			case SDL_JOYBUTTONDOWN:
				if (event.jbutton.which < pluggedJoysticks.size())
					(*pluggedJoysticks[event.jbutton.which])[event.jbutton.button] = true;
				break;
			case SDL_JOYBUTTONUP:
				if (event.jbutton.which < pluggedJoysticks.size())
					(*pluggedJoysticks[event.jbutton.which])[event.jbutton.button] = false;
				break;
			case SDL_JOYAXISMOTION:
				if (event.jaxis.which < pluggedJoysticks.size())
				{
					PluggedJoystick &j(*pluggedJoysticks[event.jaxis.which]);
					switch(event.jaxis.axis)
					{
					case JA_LR:
						if(event.jaxis.value == 0)
							j[JA_LR] = CENTER;
						else if(event.jaxis.value > 0)
							j[JA_LR] = RIGHT;
						else
							j[JA_LR] = LEFT;
						break;
					case JA_UD:
						if(event.jaxis.value == 0)
							j[JA_UD] = CENTER;
						else if(event.jaxis.value > 0)
						j[JA_UD] = DOWN;
						else
							j[JA_UD] = UP;
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


#if 0
if (event.key.keysym.sym == sfc_key[QUIT])
	S9xExit();

if (keyboardState[sfc_key[ACCEL]] == SDL_PRESSED)
{
	if (!TURBO)
	{
		TURBO = TRUE;
		OldSkipFrame = Settings.SkipFrames;
		Settings.SkipFrames = 10;
	}
}
else
{
	if (TURBO)
	{
		TURBO = FALSE;
		Settings.SkipFrames = OldSkipFrame;
	}
}

Uint16 sfc_key[256];
void S9xInitInputDevices ()
{
	memset(sfc_key, 0, sizeof(sfc_key));
	sfc_key[QUIT] = SDLK_a;
	sfc_key[A_1] = RPI_KEY_A;
	sfc_key[B_1] = RPI_KEY_B;
	sfc_key[X_1] = RPI_KEY_X;
	sfc_key[Y_1] = RPI_KEY_Y;
	sfc_key[L_1] = RPI_KEY_L;
	sfc_key[R_1] = RPI_KEY_R;
	sfc_key[START_1] = RPI_KEY_START;
	sfc_key[SELECT_1] = RPI_KEY_SELECT;
	sfc_key[LEFT_1] = RPI_KEY_LEFT;
	sfc_key[RIGHT_1] = RPI_KEY_RIGHT;
	sfc_key[UP_1] = RPI_KEY_UP;
	sfc_key[DOWN_1] = RPI_KEY_DOWN;

	sfc_key[QUIT] = RPI_KEY_QUIT;
	sfc_key[ACCEL] = RPI_KEY_ACCEL;

/*	sfc_key[LEFT_2] = SDLK_4;
	sfc_key[RIGHT_2] = SDLK_6;
	sfc_key[UP_2] = SDLK_8;
	sfc_key[DOWN_2] = SDLK_2;
	sfc_key[LU_2] = SDLK_7;
	sfc_key[LD_2] = SDLK_1;
	sfc_key[RU_2] = SDLK_9;
	sfc_key[RD_2] = SDLK_3; */


	int i = 0;
	char *envp, *j;
	envp = j = getenv ("S9XKEYS");
	if (envp) {
		do {
			if (j = strchr(envp, ','))
				*j = 0;
			if (i == 0) sfc_key[QUIT] = atoi(envp);
			else if (i == 1)  sfc_key[A_1] = atoi(envp);
			else if (i == 2)  sfc_key[B_1] = atoi(envp);
			else if (i == 3)  sfc_key[X_1] = atoi(envp);
			else if (i == 4)  sfc_key[Y_1] = atoi(envp);
			else if (i == 5)  sfc_key[L_1] = atoi(envp);
			else if (i == 6)  sfc_key[R_1] = atoi(envp);
			else if (i == 7)  sfc_key[START_1] = atoi(envp);
			else if (i == 8)  sfc_key[SELECT_1] = atoi(envp);
/*			else if (i == 9)  sfc_key[LEFT_2] = atoi(envp);
			else if (i == 10) sfc_key[RIGHT_2] = atoi(envp);
			else if (i == 11) sfc_key[UP_2] = atoi(envp);
			else if (i == 12) sfc_key[DOWN_2] = atoi(envp);
			else if (i == 13) sfc_key[LU_2] = atoi(envp);
			else if (i == 14) sfc_key[LD_2] = atoi(envp);
			else if (i == 15) sfc_key[RU_2] = atoi(envp);
			else if (i == 16) sfc_key[RD_2] = atoi(envp); */
			envp = j + 1;
			++i;
		} while(j);
	}
}
#endif
