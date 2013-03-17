#ifndef _INPUTCONTROLLER_HPP_
#define _INPUTCONTROLLER_HPP_

#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <SDL/SDL.h>

#include "inputConfig.hpp"
#include "exceptions.hpp"
#include "player.hpp"

class InputController
{
public:
	InputController() throw (ExitException);
	virtual ~InputController();

public:
	boost::uint32_t getControllerState(unsigned player) throw (ExitException);
	void checkGlobal(void) throw (ExitException);

private:
	void process(void);

private:
	InputConfig config;
	Uint8 *keyboardState;

	std::vector<Player> players;

	boost::thread *threadProcess;
	boost::mutex mutex;
};
#endif
