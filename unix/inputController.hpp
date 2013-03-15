#ifndef _INPUTCONTROLLER_HPP_
#define _INPUTCONTROLLER_HPP_

#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <SDL/SDL.h>

#include "joystick.hpp"
#include "keyboard.hpp"
#include "exceptions.hpp"


#define SNES_TR_MASK	    (1 << 4)
#define SNES_TL_MASK	    (1 << 5)
#define SNES_X_MASK	    (1 << 6)
#define SNES_A_MASK	    (1 << 7)
#define SNES_RIGHT_MASK	    (1 << 8)
#define SNES_LEFT_MASK	    (1 << 9)
#define SNES_DOWN_MASK	    (1 << 10)
#define SNES_UP_MASK	    (1 << 11)
#define SNES_START_MASK	    (1 << 12)
#define SNES_SELECT_MASK    (1 << 13)
#define SNES_Y_MASK	    (1 << 14)
#define SNES_B_MASK	    (1 << 15)

class InputController
{
public:
	InputController() throw (ExitException);
	virtual ~InputController();

public:
	boost::uint32_t getJoystickState(int index) throw (ExitException);


private:
	void loadJoystickMapping(void);
	void initSDLJoysticks(void);
	void process(void);

private:
	std::vector<boost::shared_ptr<AvailableJoystick> > availableJoysticks;
	std::vector<boost::shared_ptr<PluggedJoystick> > pluggedJoysticks;
	KeyboardMapping keyboardMapping;
	Uint8 *keyboardState;

	boost::thread *threadProcess;
	boost::mutex mutex;
};
#endif
